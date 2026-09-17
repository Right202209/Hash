package cli

import (
	"bytes"
	"context"
	"errors"
	"flag"
	"fmt"
	"io"
	"os"
	"os/signal"
	"strings"
	"syscall"

	"hash/internal/atomicfile"
	"hash/internal/engine"
	"hash/internal/registry"
)

// Exit codes returned by Run. They are part of the command-line contract.
const (
	ExitSuccess  = 0
	ExitFailure  = 1
	ExitUsage    = 2
	ExitCanceled = 3
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

// Run parses args and executes one command. Cancellation is carried by ctx;
// Main installs the signal handler, which keeps Run testable.
func Run(ctx context.Context, args []string, stdout, stderr io.Writer) int {
	config, err := Parse(args, stderr)
	if err != nil {
		return ExitUsage
	}
	return run(ctx, config, stdout, stderr)
}

func run(ctx context.Context, config Config, stdout, stderr io.Writer) int {
	algorithms, err := SelectAlgorithms(config)
	if err != nil {
		fmt.Fprintln(stderr, err)
		return ExitUsage
	}
	paths, err := ExpandPaths(config.Paths, config.Recursive, config.Literal)
	if err != nil {
		fmt.Fprintln(stderr, err)
		return ExitUsage
	}
	requests := make([]engine.FileRequest, len(paths))
	for index, path := range paths {
		requests[index] = engine.FileRequest{Path: path, Algorithms: algorithms}
	}
	results := engine.HashFiles(ctx, requests, engine.BatchOptions{Workers: config.Workers, FailFast: config.FailFast})
	if err := writeResults(stdout, config, paths, results, algorithms); err != nil {
		fmt.Fprintln(stderr, err)
		return ExitFailure
	}
	return exitCodeForResults(results, config.FailFast)
}

// writeResults renders results to stdout, or atomically to config.Output. The
// output file is only touched once formatting has succeeded, so a formatting
// failure cannot truncate an existing file.
func writeResults(stdout io.Writer, config Config, paths []string, results []engine.FileResult, algorithms []string) error {
	if config.Output == "" {
		if err := WriteResults(stdout, results, algorithms, config.Format, config.ShowSize, config.ShowModified, config.UTC); err != nil {
			return fmt.Errorf("write results: %w", err)
		}
		return nil
	}
	var buffer bytes.Buffer
	if err := WriteResults(&buffer, results, algorithms, config.Format, config.ShowSize, config.ShowModified, config.UTC); err != nil {
		return fmt.Errorf("write results: %w", err)
	}
	if err := atomicfile.Write(config.Output, paths, buffer.Bytes()); err != nil {
		return fmt.Errorf("write output: %w", err)
	}
	return nil
}

func exitCodeForResults(results []engine.FileResult, failFast bool) int {
	for _, result := range results {
		if result.Err == nil {
			continue
		}
		if failFast || errors.Is(result.Err, context.Canceled) {
			return ExitCanceled
		}
		return ExitFailure
	}
	return ExitSuccess
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

func Main() {
	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt, syscall.SIGTERM)
	code := Run(ctx, os.Args[1:], os.Stdout, os.Stderr)
	stop()
	os.Exit(code)
}
