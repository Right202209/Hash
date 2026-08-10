//go:build windows

package gui

import (
	"testing"

	"hash/internal/engine"
)

func TestCalculateLayoutKeepsPanelsInsideWindow(t *testing.T) {
	layout := calculateLayout(1080, 760, 96, 20)
	panels := []rect{layout.algorithmPanel, layout.queuePanel, layout.resultPanel}
	for _, panel := range panels {
		if panel.left < 0 || panel.top < 0 || panel.right > 1080 || panel.bottom > 760 {
			t.Fatalf("panel %+v escapes window", panel)
		}
		if panel.right <= panel.left || panel.bottom <= panel.top {
			t.Fatalf("invalid panel %+v", panel)
		}
	}
	if layout.queuePanel.bottom >= layout.resultPanel.top {
		t.Fatalf("queue and result panels overlap: queue=%+v result=%+v", layout.queuePanel, layout.resultPanel)
	}
}

func TestCalculateLayoutScalesWithDPI(t *testing.T) {
	base := calculateLayout(1080, 760, 96, 20)
	high := calculateLayout(1350, 950, 120, 20)
	if high.algorithmPanel.right <= base.algorithmPanel.right {
		t.Fatalf("algorithm panel did not scale: base=%+v high=%+v", base.algorithmPanel, high.algorithmPanel)
	}
	if high.result.bottom <= base.result.bottom {
		t.Fatalf("result panel did not scale: base=%+v high=%+v", base.result, high.result)
	}
}

func TestCalculateLayoutKeepsResultActionsAndStatusSeparate(t *testing.T) {
	for _, dpi := range []int32{96, 120, 144, 192} {
		layout := calculateLayout(scale(1080, dpi), scale(760, dpi), dpi, 20)
		controls := []rect{
			layout.copyButton,
			layout.saveButton,
			layout.pasteCompareButton,
			layout.clearCompareButton,
			layout.resultSummary,
			layout.compareStatus,
			layout.result,
		}
		for _, control := range controls {
			if control.left < layout.resultPanel.left || control.right > layout.resultPanel.right || control.top < layout.resultPanel.top || control.bottom > layout.resultPanel.bottom {
				t.Fatalf("dpi %d control %+v escapes result panel %+v", dpi, control, layout.resultPanel)
			}
		}
		buttons := controls[:4]
		for left := range buttons {
			for right := left + 1; right < len(buttons); right++ {
				if rectanglesOverlap(buttons[left], buttons[right]) {
					t.Fatalf("dpi %d buttons overlap: %+v and %+v", dpi, buttons[left], buttons[right])
				}
			}
		}
		if layout.result.top < layout.compareStatus.bottom {
			t.Fatalf("dpi %d result overlaps comparison status", dpi)
		}
	}
}

func rectanglesOverlap(left, right rect) bool {
	return left.left < right.right && right.left < left.right && left.top < right.bottom && right.top < left.bottom
}

func TestProgressPercentHandlesZeroByteFiles(t *testing.T) {
	if got := progressPercent(engine.Progress{TotalFiles: 3, CompletedFiles: 0}); got != 0 {
		t.Fatalf("initial zero-byte progress = %d", got)
	}
	if got := progressPercent(engine.Progress{TotalFiles: 3, CompletedFiles: 3}); got != 100 {
		t.Fatalf("completed zero-byte progress = %d", got)
	}
	if got := progressPercent(engine.Progress{TotalBytes: 100, CompletedBytes: 60, CurrentBytes: 20}); got != 80 {
		t.Fatalf("byte progress = %d", got)
	}
	if got := progressPercent(engine.Progress{TotalBytes: 100, CompletedBytes: 120}); got != 100 {
		t.Fatalf("clamped progress = %d", got)
	}
	if got := progressPercent(engine.Progress{TotalBytes: 1<<62 + 1, CompletedBytes: 1 << 62}); got != 99 {
		t.Fatalf("large byte progress = %d", got)
	}
}
