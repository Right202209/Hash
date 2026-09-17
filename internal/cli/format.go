package cli

import (
	"encoding/json"
	"fmt"
	"io"
	"strconv"
	"strings"
	"time"

	"hash/internal/engine"
)

// FormatOptions selects the output encoding and which optional columns are
// included. It replaces the boolean parameter list that used to make call
// sites unreadable.
type FormatOptions struct {
	Format       string
	ShowSize     bool
	ShowModified bool
	UTC          bool
}

// row is the flattened, per-algorithm view used by the text and TSV encoders.
// A failed file produces a single row with Err set.
type row struct {
	Algorithm string
	Digest    string
	Path      string
	Err       error
	Size      int64
	Modified  time.Time
}

// WriteResults renders results using options. The text and TSV encoders emit one
// row per algorithm; JSON emits one object per file.
func WriteResults(writer io.Writer, results []engine.FileResult, algorithms []string, options FormatOptions) error {
	switch strings.ToLower(options.Format) {
	case "text":
		return writeText(writer, rowsFromResults(results, algorithms), options)
	case "tsv":
		return writeTSV(writer, rowsFromResults(results, algorithms), options)
	case "json":
		return writeJSON(writer, results, options)
	default:
		return fmt.Errorf("%w %q", ErrUnsupportedFormat, options.Format)
	}
}

func rowsFromResults(results []engine.FileResult, algorithms []string) []row {
	rows := make([]row, 0, len(results))
	for _, item := range results {
		if item.Err != nil {
			rows = append(rows, row{Path: item.Request.Path, Err: item.Err})
			continue
		}
		for _, algorithm := range algorithms {
			rows = append(rows, row{
				Algorithm: algorithm,
				Digest:    item.Result.Digests[algorithm],
				Path:      item.Result.Path,
				Size:      item.Result.Size,
				Modified:  item.Result.Modified,
			})
		}
	}
	return rows
}

func writeText(writer io.Writer, rows []row, options FormatOptions) error {
	for _, item := range rows {
		if item.Err != nil {
			if _, err := fmt.Fprintf(writer, "ERROR\t%s\t%s\n", sanitize(item.Path), sanitize(item.Err.Error())); err != nil {
				return err
			}
			continue
		}
		if _, err := fmt.Fprintf(writer, "%s\t%s\t%s", item.Algorithm, item.Digest, sanitize(item.Path)); err != nil {
			return err
		}
		if options.ShowSize {
			if _, err := fmt.Fprintf(writer, "\tsize=%d", item.Size); err != nil {
				return err
			}
		}
		if options.ShowModified {
			if _, err := fmt.Fprintf(writer, "\tmodified=%s", formatTime(item.Modified, options.UTC)); err != nil {
				return err
			}
		}
		if _, err := fmt.Fprintln(writer); err != nil {
			return err
		}
	}
	return nil
}

func writeTSV(writer io.Writer, rows []row, options FormatOptions) error {
	columns := []string{"algorithm", "digest", "path", "error"}
	if options.ShowSize {
		columns = append(columns, "size")
	}
	if options.ShowModified {
		columns = append(columns, "modified")
	}
	if _, err := fmt.Fprintln(writer, strings.Join(columns, "\t")); err != nil {
		return err
	}
	for _, item := range rows {
		values := make([]string, 0, len(columns))
		if item.Err != nil {
			values = append(values, "ERROR", "", item.Path, item.Err.Error())
			if options.ShowSize {
				values = append(values, "-")
			}
			if options.ShowModified {
				values = append(values, "-")
			}
		} else {
			values = append(values, item.Algorithm, item.Digest, item.Path, "")
			if options.ShowSize {
				values = append(values, strconv.FormatInt(item.Size, 10))
			}
			if options.ShowModified {
				values = append(values, formatTime(item.Modified, options.UTC))
			}
		}
		for index := range values {
			values[index] = tsvValue(values[index])
		}
		if _, err := fmt.Fprintln(writer, strings.Join(values, "\t")); err != nil {
			return err
		}
	}
	return nil
}

func writeJSON(writer io.Writer, results []engine.FileResult, options FormatOptions) error {
	output := make([]jsonResult, 0, len(results))
	for _, item := range results {
		entry := jsonResult{Path: item.Request.Path, Digests: item.Result.Digests, Error: errorString(item.Err)}
		if options.ShowSize && item.Err == nil {
			entry.Size = &item.Result.Size
		}
		if options.ShowModified && item.Err == nil {
			modified := item.Result.Modified
			if options.UTC {
				modified = modified.UTC()
			}
			entry.Modified = &modified
		}
		output = append(output, entry)
	}
	encoder := json.NewEncoder(writer)
	encoder.SetIndent("", "  ")
	return encoder.Encode(output)
}

type jsonResult struct {
	Path     string            `json:"path"`
	Size     *int64            `json:"size,omitempty"`
	Modified *time.Time        `json:"modified,omitempty"`
	Digests  map[string]string `json:"digests,omitempty"`
	Error    string            `json:"error,omitempty"`
}

func errorString(err error) string {
	if err == nil {
		return ""
	}
	return err.Error()
}

func formatTime(value time.Time, utc bool) string {
	if utc {
		value = value.UTC()
	}
	return value.Format(time.RFC3339)
}

func sanitize(value string) string {
	return strconv.Quote(value)
}

func tsvValue(value string) string {
	trimmed := strings.TrimLeft(value, " \t\r\n")
	value = strings.NewReplacer("\\", "\\\\", "\t", "\\t", "\r", "\\r", "\n", "\\n").Replace(value)
	if trimmed != "" && strings.ContainsRune("=+-@", rune(trimmed[0])) {
		return "'" + value
	}
	return value
}
