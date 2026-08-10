package compare

import (
	"testing"

	"hash/internal/engine"
)

func TestParseTextAndCompare(t *testing.T) {
	text := "sha256\tABC123\t\"C:\\\\Data\\\\a.bin\"\tsize=4\n"
	actual, err := ParseText(text)
	if err != nil {
		t.Fatal(err)
	}
	expected := []Record{{Algorithm: "SHA256", Digest: "abc123", Path: "c:\\data\\a.bin"}}
	got := Compare(expected, actual)
	if !got.Exact || got.Matches != 1 {
		t.Fatalf("summary = %+v", got)
	}
}

func TestParseTextSupportsTSVEscapesAndUnicode(t *testing.T) {
	text := "algorithm\tdigest\tpath\terror\nSHA256\tABC123\tC:\\\\资料\\\\文件\\\\a.bin\t\n"
	records, err := ParseText(text)
	if err != nil {
		t.Fatal(err)
	}
	if len(records) != 1 || records[0].Path != "C:\\资料\\文件\\a.bin" {
		t.Fatalf("records = %+v", records)
	}
}

func TestParseTextSkipsErrorRows(t *testing.T) {
	text := "algorithm\tdigest\tpath\terror\nSHA256\tabc\tC:\\\\ok.bin\t\nERROR\t\tmissing.bin\tfile missing\n"
	records, err := ParseText(text)
	if err != nil {
		t.Fatal(err)
	}
	if len(records) != 1 || records[0].Path != "C:\\ok.bin" {
		t.Fatalf("records = %+v", records)
	}
}

func TestParseTextRemovesTSVFormulaSafetyPrefix(t *testing.T) {
	records, err := ParseText("sha256\tabc\t'=report.bin\n")
	if err != nil {
		t.Fatal(err)
	}
	if records[0].Path != "=report.bin" {
		t.Fatalf("path = %q", records[0].Path)
	}
}

func TestParseTextAcceptsByteOrderMark(t *testing.T) {
	records, err := ParseText("\uFEFFsha256\tabc\t\"C:\\\\a.bin\"\n")
	if err != nil {
		t.Fatal(err)
	}
	if len(records) != 1 {
		t.Fatalf("records = %+v", records)
	}
}
func TestCompareMissingUnexpectedMismatchAndDuplicate(t *testing.T) {
	expected := []Record{{"sha256", "aaa", "a"}, {"md5", "bbb", "b"}, {"sha1", "ccc", "c"}}
	actual := []Record{{"sha256", "aaa", "a"}, {"md5", "wrong", "b"}, {"sha512", "ddd", "d"}, {"sha512", "eee", "d"}}
	got := Compare(expected, actual)
	if got.Matches != 1 || got.Mismatches != 1 || got.Missing != 1 || got.Unexpected != 1 || got.Duplicates != 1 || got.Exact {
		t.Fatalf("summary = %+v", got)
	}
}

func TestRecordsFromResultsSkipsErrors(t *testing.T) {
	results := []engine.FileResult{{Result: engine.Result{Path: "a", Order: []string{"sha256"}, Digests: map[string]string{"sha256": "abc"}}}, {Err: testingError{}}}
	records := RecordsFromResults(results)
	if len(records) != 1 || records[0].Digest != "abc" {
		t.Fatalf("records = %+v", records)
	}
}

type testingError struct{}

func (testingError) Error() string { return "error" }

func TestParseTextRejectsInvalidLines(t *testing.T) {
	if _, err := ParseText("not-a-record"); err == nil {
		t.Fatal("invalid clipboard text was accepted")
	}
}
