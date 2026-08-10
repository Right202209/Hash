package engine

import "time"

type Options struct {
	BufferSize int
	Progress   func(bytesRead int64)
}

type Result struct {
	Path      string
	Size      int64
	Modified  time.Time
	Digests   map[string]string
	Order     []string
	BytesRead int64
	Reads     int
	Changed   bool
}
