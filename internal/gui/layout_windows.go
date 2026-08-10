//go:build windows

package gui

func scale(value, dpi int32) int32 {
	if dpi <= 0 {
		dpi = 96
	}
	return value * dpi / 96
}

type uiLayout struct {
	algorithmPanel     rect
	queuePanel         rect
	resultPanel        rect
	addButton          rect
	clearButton        rect
	startButton        rect
	copyButton         rect
	saveButton         rect
	pasteCompareButton rect
	clearCompareButton rect
	cancelButton       rect
	title              rect
	subtitle           rect
	algorithmTitle     rect
	algorithmSummary   rect
	queueTitle         rect
	queueInfo          rect
	queue              rect
	resultTitle        rect
	resultSummary      rect
	compareStatus      rect
	result             rect
	status             rect
	progress           rect
}

func calculateLayout(width, height, dpi int32, algorithmCount int) uiLayout {
	if width < scale(900, dpi) {
		width = scale(900, dpi)
	}
	if height < scale(640, dpi) {
		height = scale(640, dpi)
	}
	margin := scale(24, dpi)
	gap := scale(16, dpi)
	headerHeight := scale(62, dpi)
	bottomHeight := scale(48, dpi)
	leftWidth := scale(272, dpi)
	bodyTop := headerHeight + margin
	bodyBottom := height - bottomHeight - margin
	if bodyBottom < bodyTop+scale(220, dpi) {
		bodyBottom = bodyTop + scale(220, dpi)
	}
	rightX := margin + leftWidth + gap
	rightWidth := width - rightX - margin
	if rightWidth < scale(420, dpi) {
		rightWidth = scale(420, dpi)
	}
	rightEdge := rightX + rightWidth
	bodyHeight := bodyBottom - bodyTop
	queueHeight := bodyHeight * 42 / 100
	if queueHeight < scale(180, dpi) {
		queueHeight = scale(180, dpi)
	}
	resultTop := bodyTop + queueHeight + gap
	inner := scale(16, dpi)
	buttonHeight := scale(32, dpi)
	buttonWidth := scale(94, dpi)
	buttonGap := scale(8, dpi)
	topButtonsWidth := buttonWidth*3 + buttonGap*2
	topButtonsX := width - margin - topButtonsWidth
	resultButtonWidth := scale(78, dpi)
	resultButtonX := width - margin - resultButtonWidth*4 - buttonGap*3
	statusHeight := scale(32, dpi)
	statusTop := height - margin - statusHeight
	return uiLayout{
		algorithmPanel:     rect{margin, bodyTop, margin + leftWidth, bodyBottom},
		queuePanel:         rect{rightX, bodyTop, rightEdge, bodyTop + queueHeight},
		resultPanel:        rect{rightX, resultTop, rightEdge, bodyBottom},
		addButton:          rect{topButtonsX, scale(18, dpi), topButtonsX + buttonWidth, scale(18, dpi) + buttonHeight},
		clearButton:        rect{topButtonsX + buttonWidth + buttonGap, scale(18, dpi), topButtonsX + buttonWidth*2 + buttonGap, scale(18, dpi) + buttonHeight},
		startButton:        rect{topButtonsX + buttonWidth*2 + buttonGap*2, scale(18, dpi), width - margin, scale(18, dpi) + buttonHeight},
		copyButton:         rect{resultButtonX, resultTop + scale(10, dpi), resultButtonX + resultButtonWidth, resultTop + scale(10, dpi) + buttonHeight},
		saveButton:         rect{resultButtonX + resultButtonWidth + buttonGap, resultTop + scale(10, dpi), resultButtonX + resultButtonWidth*2 + buttonGap, resultTop + scale(10, dpi) + buttonHeight},
		pasteCompareButton: rect{resultButtonX + (resultButtonWidth+buttonGap)*2, resultTop + scale(10, dpi), resultButtonX + resultButtonWidth*3 + buttonGap*2, resultTop + scale(10, dpi) + buttonHeight},
		clearCompareButton: rect{resultButtonX + (resultButtonWidth+buttonGap)*3, resultTop + scale(10, dpi), width - margin, resultTop + scale(10, dpi) + buttonHeight},
		cancelButton:       rect{width - margin - scale(78, dpi), statusTop, width - margin, statusTop + statusHeight},
		title:              rect{margin, scale(16, dpi), rightX, scale(40, dpi)},
		subtitle:           rect{margin, scale(42, dpi), rightX, scale(62, dpi)},
		algorithmTitle:     rect{margin + inner, bodyTop + inner, margin + leftWidth - inner, bodyTop + inner + scale(24, dpi)},
		algorithmSummary:   rect{margin + inner, bodyBottom - inner - scale(24, dpi), margin + leftWidth - inner, bodyBottom - inner},
		queueTitle:         rect{rightX + inner, bodyTop + inner, rightEdge - inner, bodyTop + inner + scale(24, dpi)},
		queueInfo:          rect{rightX + inner, bodyTop + inner + scale(22, dpi), rightEdge - inner, bodyTop + inner + scale(42, dpi)},
		queue:              rect{rightX + inner, bodyTop + inner + scale(48, dpi), rightEdge - inner, bodyTop + queueHeight - inner},
		resultTitle:        rect{rightX + inner, resultTop + inner, resultButtonX - gap, resultTop + inner + scale(24, dpi)},
		resultSummary:      rect{rightX + inner, resultTop + inner + scale(34, dpi), rightEdge - inner, resultTop + inner + scale(54, dpi)},
		compareStatus:      rect{rightX + inner, resultTop + inner + scale(56, dpi), rightEdge - inner, resultTop + inner + scale(76, dpi)},
		result:             rect{rightX + inner, resultTop + inner + scale(84, dpi), rightEdge - inner, bodyBottom - inner},
		status:             rect{margin, statusTop, margin + scale(244, dpi), statusTop + statusHeight},
		progress:           rect{margin + scale(260, dpi), statusTop + scale(8, dpi), width - margin - scale(190, dpi), statusTop + statusHeight - scale(8, dpi)},
	}
}
