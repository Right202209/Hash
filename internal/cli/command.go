package cli

import (
	"context"
	"errors"
	"flag"
	"fmt"
	"io"
	"os"
	"os/signal"
	"path/filepath"
	"strings"
	"syscall"

	"hash/internal/engine"
	"hash/internal/pathutil"
	"hash/internal/registry"
)

type Config struct {
	Algorithms   []string
	All          bool
	ShowSize     bool
	ShowModified bool
	UTC          bool
	Workers      int
	Format       string
	Output       string
	Recursive    bool
	Literal      bool
	FailFast     bool
	Paths        []string
}

func Run(args []string, stdout, stderr io.Writer) int {
	config, err := Parse(args, stderr)
	if err != nil {
		return 2
	}
	algorithms, err := SelectAlgorithms(config)
	if err != nil {
		fmt.Fprintln(stderr, err)
		return 2
	}
	paths, err := ExpandPaths(config.Paths, config.Recursive, config.Literal)
	if err != nil {
		fmt.Fprintln(stderr, err)
		return 2
	}
	requests := make([]engine.FileRequest, len(paths))
	for index, path := range paths {
		requests[index] = engine.FileRequest{Path: path, Algorithms: algorithms}
	}
	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt, syscall.SIGTERM)
	defer stop()
	results := engine.HashFiles(ctx, requests, engine.BatchOptions{Workers: config.Workers, FailFast: config.FailFast})
	writer := stdout
	var output *os.File
	var tempOutput string
	if config.Output != "" {
		output, tempOutput, err = prepareOutput(config.Output, paths)
		if err != nil {
			fmt.Fprintln(stderr, err)
			return 1
		}
		writer = output
	}
	if err := WriteResults(writer, results, algorithms, config.Format, config.ShowSize, config.ShowModified, config.UTC); err != nil {
		if output != nil {
			_ = output.Close()
			_ = os.Remove(tempOutput)
		}
		fmt.Fprintln(stderr, "write results:", err)
		return 1
	}
	if output != nil {
		if err := output.Close(); err != nil {
			_ = os.Remove(tempOutput)
			fmt.Fprintln(stderr, "close output:", err)
			return 1
		}
		if err := commitOutput(tempOutput, config.Output, paths); err != nil {
			_ = os.Remove(tempOutput)
			fmt.Fprintln(stderr, "commit output:", err)
			return 1
		}
	}
	for _, result := range results {
		if result.Err != nil {
			if config.FailFast || errors.Is(result.Err, context.Canceled) {
				return 3
			}
			return 1
		}
	}
	return 0
}

func Parse(args []string, usage io.Writer) (Config, error) {
	flags := flag.NewFlagSet("hash", flag.ContinueOnError)
	flags.SetOutput(usage)
	algorithmList := flags.String("algorithm", "sha256", "comma-separated algorithms")
	flags.StringVar(algorithmList, "a", "sha256", "comma-separated algorithms")
	all := flags.Bool("all", false, "select every available algorithm")
	showSize := flags.Bool("size", false, "show file size in bytes")
	showModified := flags.Bool("modified", false, "show modification time")
	utc := flags.Bool("utc", false, "show times in UTC")
	workers := flags.Int("workers", 0, "number of files processed concurrently")
	format := flags.String("format", "text", "output format: text, tsv, or json")
	output := flags.String("output", "", "write output to a file")
	flags.StringVar(output, "o", "", "write output to a file")
	recursive := flags.Bool("recursive", false, "hash files under directory paths recursively")
	literal := flags.Bool("literal", false, "treat wildcard characters as literal")
	failFast := flags.Bool("fail-fast", false, "stop scheduling after cancellation or first failed result")
	if err := flags.Parse(args); err != nil {
		return Config{}, err
	}
	if *workers < 0 {
		return Config{}, errors.New("workers cannot be negative")
	}
	if *format != "text" && *format != "tsv" && *format != "json" {
		return Config{}, fmt.Errorf("unsupported output format %q", *format)
	}
	return Config{
		Algorithms:   splitAlgorithms(*algorithmList),
		All:          *all,
		ShowSize:     *showSize,
		ShowModified: *showModified,
		UTC:          *utc,
		Workers:      *workers,
		Format:       *format,
		Output:       *output,
		Recursive:    *recursive,
		Literal:      *literal,
		FailFast:     *failFast,
		Paths:        flags.Args(),
	}, nil
}

func SelectAlgorithms(config Config) ([]string, error) {
	if config.All {
		algorithms := registry.Algorithms()
		selected := make([]string, len(algorithms))
		for index, algorithm := range algorithms {
			selected[index] = algorithm.Name
		}
		return selected, nil
	}
	if len(config.Algorithms) == 0 {
		return nil, errors.New("at least one algorithm is required")
	}
	selected := make([]string, 0, len(config.Algorithms))
	seen := make(map[string]struct{}, len(config.Algorithms))
	for _, name := range config.Algorithms {
		name = strings.ToLower(strings.TrimSpace(name))
		if name == "" {
			return nil, errors.New("algorithm name cannot be empty")
		}
		if _, ok := registry.Lookup(name); !ok {
			return nil, fmt.Errorf("unknown algorithm %q", name)
		}
		if _, ok := seen[name]; ok {
			return nil, fmt.Errorf("algorithm %q was selected more than once", name)
		}
		seen[name] = struct{}{}
		selected = append(selected, name)
	}
	return selected, nil
}

func splitAlgorithms(value string) []string {
	parts := strings.Split(value, ",")
	for index := range parts {
		parts[index] = strings.ToLower(strings.TrimSpace(parts[index]))
	}
	return parts
}

func WriteFileAtomic(path string, inputs []string, content []byte) error {
	file, tempPath, err := prepareOutput(path, inputs)
	if err != nil {
		return err
	}
	if _, err := file.Write(content); err != nil {
		_ = file.Close()
		_ = os.Remove(tempPath)
		return fmt.Errorf("write temporary output: %w", err)
	}
	if err := file.Close(); err != nil {
		_ = os.Remove(tempPath)
		return fmt.Errorf("close temporary output: %w", err)
	}
	if err := commitOutput(tempPath, path, inputs); err != nil {
		_ = os.Remove(tempPath)
		return err
	}
	return nil
}

func validateOutputPath(output string, inputs []string) error {
	outputAbs, err := filepath.Abs(output)
	if err != nil {
		return fmt.Errorf("resolve output path: %w", err)
	}
	outputInfo, err := os.Lstat(output)
	if err == nil && outputInfo.Mode()&os.ModeSymlink != 0 {
		return fmt.Errorf("refusing to write through symbolic link %q", output)
	}
	if err != nil && !os.IsNotExist(err) {
		return fmt.Errorf("inspect output path: %w", err)
	}
	if err == nil {
		outputInfo, statErr := os.Stat(output)
		if statErr != nil {
			return fmt.Errorf("stat output path: %w", statErr)
		}
		for _, input := range inputs {
			inputInfo, inputErr := os.Stat(input)
			if inputErr == nil && os.SameFile(outputInfo, inputInfo) {
				return fmt.Errorf("output path must not refer to input %q", input)
			}
		}
	}
	for _, input := range inputs {
		inputAbs, absErr := filepath.Abs(input)
		if absErr == nil && pathutil.Key(inputAbs) == pathutil.Key(outputAbs) {
			return fmt.Errorf("output path must differ from input %q", input)
		}
	}
	return nil
}

func prepareOutput(output string, inputs []string) (*os.File, string, error) {
	if err := validateOutputPath(output, inputs); err != nil {
		return nil, "", err
	}
	file, err := os.CreateTemp(filepath.Dir(output), ".hash-output-*")
	if err != nil {
		return nil, "", fmt.Errorf("create temporary output: %w", err)
	}
	return file, file.Name(), nil
}

func commitOutput(tempPath, output string, inputs []string) error {
	if err := validateOutputPath(output, inputs); err != nil {
		return err
	}
	if err := replacePath(tempPath, output); err != nil {
		return fmt.Errorf("rename temporary output: %w", err)
	}
	return nil
}

func Main() {
	code := Run(os.Args[1:], os.Stdout, os.Stderr)
	os.Exit(code)
}
