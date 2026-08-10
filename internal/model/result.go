package model

import "time"

type FileEntry struct {
	Path     string
	Size     int64
	Modified time.Time
}

type HashResult struct {
	Path     string            `json:"path"`
	Size     int64             `json:"size,omitempty"`
	Modified time.Time         `json:"modified,omitempty"`
	Digests  map[string]string `json:"digests"`
	Changed  bool              `json:"changed,omitempty"`
	Error    string            `json:"error,omitempty"`
}
