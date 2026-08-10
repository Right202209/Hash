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

func WriteResults(writer io.Writer, results []engine.FileResult, algorithms []string, format string, showSize, showModified, utc bool) error {
	switch strings.ToLower(format) {
	case "text":
		return writeText(writer, results, algorithms, showSize, showModified, utc)
	case "tsv":
		return writeTSV(writer, results, algorithms, showSize, showModified, utc)
	case "json":
		return writeJSON(writer, results, algorithms, showSize, showModified, utc)
	default:
		return fmt.Errorf("unsupported output format %q", format)
	}
}

func writeText(writer io.Writer, results []engine.FileResult, algorithms []string, showSize, showModified, utc bool) error {
	for _, item := range results {
		if item.Err != nil {
			if _, err := fmt.Fprintf(writer, "ERROR\t%s\t%s\n", sanitize(item.Request.Path), sanitize(item.Err.Error())); err != nil {
				return err
			}
			continue
		}
		for _, algorithm := range algorithms {
			if showSize || showModified {
				if _, err := fmt.Fprintf(writer, "%s\t%s\t%s", algorithm, item.Result.Digests[algorithm], sanitize(item.Result.Path)); err != nil {
					return err
				}
				if showSize {
					if _, err := fmt.Fprintf(writer, "\tsize=%d", item.Result.Size); err != nil {
						return err
					}
				}
				if showModified {
					if _, err := fmt.Fprintf(writer, "\tmodified=%s", formatTime(item.Result.Modified, utc)); err != nil {
						return err
					}
				}
				if _, err := fmt.Fprintln(writer); err != nil {
					return err
				}
				continue
			}
			if _, err := fmt.Fprintf(writer, "%s\t%s\t%s\n", algorithm, item.Result.Digests[algorithm], sanitize(item.Result.Path)); err != nil {
				return err
			}
		}
	}
	return nil
}

func writeTSV(writer io.Writer, results []engine.FileResult, algorithms []string, showSize, showModified, utc bool) error {
	columns := []string{"algorithm", "digest", "path", "error"}
	if showSize {
		columns = append(columns, "size")
	}
	if showModified {
		columns = append(columns, "modified")
	}
	if _, err := fmt.Fprintln(writer, strings.Join(columns, "\t")); err != nil {
		return err
	}
	for _, item := range results {
		if item.Err != nil {
			values := []string{"ERROR", "", item.Request.Path, item.Err.Error()}
			if showSize {
				values = append(values, "-")
			}
			if showModified {
				values = append(values, "-")
			}
			for index := range values {
				values[index] = tsvValue(values[index])
			}
			if _, err := fmt.Fprintln(writer, strings.Join(values, "\t")); err != nil {
				return err
			}
			continue
		}
		for _, algorithm := range algorithms {
			values := []string{algorithm, item.Result.Digests[algorithm], item.Result.Path, ""}
			if showSize {
				values = append(values, strconv.FormatInt(item.Result.Size, 10))
			}
			if showModified {
				values = append(values, formatTime(item.Result.Modified, utc))
			}
			for index := range values {
				values[index] = tsvValue(values[index])
			}
			if _, err := fmt.Fprintln(writer, strings.Join(values, "\t")); err != nil {
				return err
			}
		}
	}
	return nil
}

func writeJSON(writer io.Writer, results []engine.FileResult, algorithms []string, showSize, showModified, utc bool) error {
	output := make([]jsonResult, 0, len(results))
	for _, item := range results {
		entry := jsonResult{Path: item.Request.Path, Digests: item.Result.Digests, Error: errorString(item.Err)}
		if showSize && item.Err == nil {
			entry.Size = &item.Result.Size
		}
		if showModified && item.Err == nil {
			modified := item.Result.Modified
			if utc {
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
