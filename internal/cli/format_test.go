package cli

import "testing"

func TestTSVValueEscapesFormulaAfterLeadingWhitespace(t *testing.T) {
	for _, value := range []string{"=1+1", " =1+1", "\t@SUM(A:A)"} {
		got := tsvValue(value)
		if got[0] != '\'' {
			t.Fatalf("tsvValue(%q) = %q, want a leading apostrophe", value, got)
		}
	}
}
