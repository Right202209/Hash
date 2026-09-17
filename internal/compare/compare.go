// Package compare parses digest listings and compares an expected listing with a
// freshly computed one, reporting matches, mismatches and set differences.
package compare

import (
	"bufio"
	"fmt"
	"strconv"
	"strings"

	"hash/internal/engine"
	"hash/internal/pathutil"
)

const maxRecords = 100_000

// Record is one algorithm, digest and path tuple read from a listing.
type Record struct {
	Algorithm string
	Digest    string
	Path      string
}

// Summary counts how an actual listing compares with an expected one. Exact is
// true only when every expected record matched and neither side had extras or
// duplicates.
type Summary struct {
	Matches    int
	Mismatches int
	Missing    int
	Unexpected int
	Duplicates int
	Exact      bool
}

// RecordsFromResults flattens successful hash results into comparison records,
// skipping files that failed.
func RecordsFromResults(results []engine.FileResult) []Record {
	records := make([]Record, 0)
	for _, result := range results {
		if result.Err != nil {
			continue
		}
		for _, algorithm := range result.Result.Order {
			digest := result.Result.Digests[algorithm]
			if digest == "" {
				continue
			}
			records = append(records, Record{Algorithm: algorithm, Digest: digest, Path: result.Result.Path})
		}
	}
	return records
}

// ParseText reads a text or TSV digest listing, tolerating a BOM, header rows
// and error rows.
func ParseText(text string) ([]Record, error) {
	text = strings.TrimPrefix(text, string(rune(0xFEFF)))
	if strings.TrimSpace(text) == "" {
		return nil, fmt.Errorf("clipboard is empty")
	}

	records := make([]Record, 0)
	scanner := bufio.NewScanner(strings.NewReader(text))
	scanner.Buffer(make([]byte, 4*1024), 16<<20)
	for lineNumber := 1; scanner.Scan(); lineNumber++ {
		line := strings.TrimSpace(scanner.Text())
		if line == "" || strings.HasPrefix(line, "WARNING:") || strings.HasPrefix(line, "algorithm\tdigest\tpath") {
			continue
		}
		fields := strings.SplitN(line, "\t", 4)
		if len(fields) < 3 {
			return nil, fmt.Errorf("line %d has too few fields", lineNumber)
		}
		algorithm := strings.ToLower(strings.TrimSpace(fields[0]))
		if strings.EqualFold(algorithm, "error") {
			continue
		}
		digest := strings.ToLower(strings.TrimSpace(fields[1]))
		if algorithm == "" || digest == "" {
			return nil, fmt.Errorf("line %d is not a digest record", lineNumber)
		}
		path, err := parsePath(fields[2])
		if err != nil {
			return nil, fmt.Errorf("line %d path: %w", lineNumber, err)
		}
		records = append(records, Record{Algorithm: algorithm, Digest: digest, Path: path})
		if len(records) > maxRecords {
			return nil, fmt.Errorf("clipboard contains more than %d digest records", maxRecords)
		}
	}
	if err := scanner.Err(); err != nil {
		return nil, fmt.Errorf("scan clipboard: %w", err)
	}
	if len(records) == 0 {
		return nil, fmt.Errorf("clipboard contains no digest records")
	}
	return records, nil
}

func parsePath(value string) (string, error) {
	path := strings.TrimSpace(value)
	if strings.HasPrefix(path, "\"") {
		decoded, err := strconv.Unquote(path)
		if err != nil {
			return "", err
		}
		path = decoded
	} else {
		decoded, err := unescapeTSV(path)
		if err != nil {
			return "", err
		}
		path = removeFormulaPrefix(decoded)
	}
	if path == "" {
		return "", fmt.Errorf("has empty path")
	}
	return path, nil
}

func removeFormulaPrefix(value string) string {
	if len(value) > 1 && value[0] == '\'' {
		trimmed := strings.TrimLeft(value[1:], " \t\r\n")
		if trimmed != "" && strings.ContainsRune("=+-@", rune(trimmed[0])) {
			return value[1:]
		}
	}
	return value
}

func unescapeTSV(value string) (string, error) {
	var builder strings.Builder
	builder.Grow(len(value))
	for index := 0; index < len(value); index++ {
		if value[index] != '\\' {
			builder.WriteByte(value[index])
			continue
		}
		if index+1 >= len(value) {
			return "", fmt.Errorf("has an incomplete escape")
		}
		index++
		switch value[index] {
		case '\\':
			builder.WriteByte('\\')
		case 't':
			builder.WriteByte('\t')
		case 'r':
			builder.WriteByte('\r')
		case 'n':
			builder.WriteByte('\n')
		default:
			return "", fmt.Errorf("has unsupported escape \\%c", value[index])
		}
	}
	return builder.String(), nil
}

// Compare indexes both listings by algorithm and normalized path, then counts
// matches, mismatches, missing, unexpected and duplicate records.
func Compare(expected, actual []Record) Summary {
	expectedMap, expectedDuplicates := index(expected)
	actualMap, actualDuplicates := index(actual)
	summary := Summary{Duplicates: expectedDuplicates + actualDuplicates}
	for key, expectedDigest := range expectedMap {
		actualDigest, ok := actualMap[key]
		if !ok {
			summary.Missing++
			continue
		}
		if expectedDigest == actualDigest {
			summary.Matches++
		} else {
			summary.Mismatches++
		}
	}
	for key := range actualMap {
		if _, ok := expectedMap[key]; !ok {
			summary.Unexpected++
		}
	}
	summary.Exact = summary.Mismatches == 0 && summary.Missing == 0 && summary.Unexpected == 0 && summary.Duplicates == 0
	return summary
}

func index(records []Record) (map[string]string, int) {
	indexed := make(map[string]string, len(records))
	duplicates := 0
	for _, record := range records {
		key := strings.ToLower(strings.TrimSpace(record.Algorithm)) + "\x00" + normalizePath(record.Path)
		if _, exists := indexed[key]; exists {
			duplicates++
			continue
		}
		indexed[key] = strings.ToLower(strings.TrimSpace(record.Digest))
	}
	return indexed, duplicates
}

func normalizePath(path string) string {
	return pathutil.Key(strings.TrimSpace(path))
}
