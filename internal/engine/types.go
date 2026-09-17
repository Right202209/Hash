package engine

import "time"

// Options tunes a single-file hash operation.
type Options struct {
	BufferSize int
	Progress   func(bytesRead int64)
}

// Result is the outcome of hashing one file. Digests is keyed by algorithm name
// and Order preserves the caller's requested order.
type Result struct {
	Path      string
	Size      int64
	Modified  time.Time
	Digests   map[string]string
	Order     []string
	BytesRead int64
	Changed   bool
}
