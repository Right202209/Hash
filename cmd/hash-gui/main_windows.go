//go:build windows

// Command hash-gui is the native Windows GUI entry point for the hashing tool.
package main

import "hash/internal/gui"

func main() {
	gui.Run()
}
