package registry_test

import (
	"crypto/md5"
	"crypto/sha1"
	"crypto/sha256"
	"crypto/sha512"
	"encoding/hex"
	"hash"
	"testing"

	"hash/internal/registry"
)

func TestAlgorithms_StandardDigestVectors(t *testing.T) {
	t.Parallel()

	tests := []struct {
		name        string
		newHash     func() hash.Hash
		expectation map[string]string
	}{
		{name: "md5", newHash: md5.New, expectation: map[string]string{
			"": "d41d8cd98f00b204e9800998ecf8427e", "abc": "900150983cd24fb0d6963f7d28e17f72",
			"The quick brown fox jumps over the lazy dog": "9e107d9d372bb6826bd81d3542a419d6",
		}},
		{name: "sha1", newHash: sha1.New, expectation: map[string]string{
			"": "da39a3ee5e6b4b0d3255bfef95601890afd80709", "abc": "a9993e364706816aba3e25717850c26c9cd0d89d",
			"The quick brown fox jumps over the lazy dog": "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12",
		}},
		{name: "sha224", newHash: sha256.New224, expectation: map[string]string{
			"": "d14a028c2a3a2bc9476102bb288234c415a2b01f828ea62ac5b3e42f", "abc": "23097d223405d8228642a477bda255b32aadbce4bda0b3f7e36c9da7",
			"The quick brown fox jumps over the lazy dog": "730e109bd7a8a32b1cb9d9a09aa2325d2430587ddbc0c38bad911525",
		}},
		{name: "sha256", newHash: sha256.New, expectation: map[string]string{
			"": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", "abc": "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
			"The quick brown fox jumps over the lazy dog": "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592",
		}},
		{name: "sha384", newHash: sha512.New384, expectation: map[string]string{
			"": "38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da274edebfe76f65fbd51ad2f14898b95b", "abc": "cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed8086072ba1e7cc2358baeca134c825a7",
			"The quick brown fox jumps over the lazy dog": "ca737f1014a48f4c0b6dd43cb177b0afd9e5169367544c494011e3317dbf9a509cb1e5dc1e85a941bbee3d7f2afbc9b1",
		}},
		{name: "sha512", newHash: sha512.New, expectation: map[string]string{
			"": "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e", "abc": "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f",
			"The quick brown fox jumps over the lazy dog": "07e547d9586f6a73f73fbac0435ed76951218fb7d0c8d788a309d785436bbb642e93a252a954f23912547d1e8a3b5ed6e1bfd7097821233fa0538f3db854fee6",
		}},
	}

	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			t.Parallel()
			spec, ok := registry.Lookup(test.name)
			if !ok || spec.New == nil {
				t.Fatalf("Lookup(%q) returned an unavailable algorithm", test.name)
			}
			for input, expected := range test.expectation {
				referenceHash := test.newHash()
				if _, err := referenceHash.Write([]byte(input)); err != nil {
					t.Fatalf("standard %q Write(%q): %v", test.name, input, err)
				}
				standard := hex.EncodeToString(referenceHash.Sum(nil))
				if standard != expected {
					t.Fatalf("test vector for standard %q digest(%q) = %s, want %s", test.name, input, standard, expected)
				}

				gotHash := spec.New()
				if _, err := gotHash.Write([]byte(input)); err != nil {
					t.Fatalf("%q Write(%q): %v", test.name, input, err)
				}
				if got := hex.EncodeToString(gotHash.Sum(nil)); got != standard {
					t.Errorf("%q digest(%q) = %s, want standard-library result %s", test.name, input, got, standard)
				}
			}
		})
	}
}

func TestAlgorithms_ClassificationAndSecurityMarkers(t *testing.T) {
	t.Parallel()
	want := map[string]struct {
		category registry.Category
		secure   bool
	}{
		"md5": {registry.CategoryLegacy, false}, "sha1": {registry.CategoryLegacy, false},
		"sha224": {registry.CategoryCryptographic, true}, "sha256": {registry.CategoryCryptographic, true},
		"sha384": {registry.CategoryCryptographic, true}, "sha512": {registry.CategoryCryptographic, true},
	}
	algorithms := registry.Algorithms()
	if len(algorithms) < len(want) {
		t.Fatalf("Algorithms() returned %d entries, want at least %d", len(algorithms), len(want))
	}
	seen := make(map[string]bool, len(algorithms))
	for _, algorithm := range algorithms {
		if seen[algorithm.Name] {
			t.Errorf("Algorithms() returned duplicate name %q", algorithm.Name)
		}
		seen[algorithm.Name] = true
		expected, ok := want[algorithm.Name]
		if !ok {
			continue
		}
		if algorithm.Category != expected.category {
			t.Errorf("%q category = %q, want %q", algorithm.Name, algorithm.Category, expected.category)
		}
		if algorithm.Secure != expected.secure {
			t.Errorf("%q Secure = %t, want %t", algorithm.Name, algorithm.Secure, expected.secure)
		}
		if algorithm.New == nil {
			t.Errorf("%q has nil constructor", algorithm.Name)
		}
	}
}

func TestLookup_UnknownAlgorithm(t *testing.T) {
	t.Parallel()
	if _, ok := registry.Lookup("sha3-256"); ok {
		t.Fatal(`Lookup("sha3-256") unexpectedly found an unregistered algorithm`)
	}
}
