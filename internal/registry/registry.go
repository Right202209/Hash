// Package registry is the single source of truth for the hash algorithms the
// tool can compute, including their display labels and security categories.
package registry

import "hash"

// Category groups algorithms by their intended use.
type Category string

// Algorithm categories reported by Spec.Category.
const (
	CategoryCryptographic Category = "cryptographic"
	CategoryLegacy        Category = "legacy"
	CategoryChecksum      Category = "checksum"
	CategoryNonCrypto     Category = "non-cryptographic"
)

// Spec describes one algorithm. Name is the lower-case registry key; New
// returns a fresh hash.Hash for each file so digests never share state.
type Spec struct {
	Name     string
	Label    string
	Category Category
	Secure   bool
	New      func() hash.Hash
}

var specs = []Spec{
	{Name: "sha224", Label: "SHA-224", Category: CategoryCryptographic, Secure: true, New: newHashSHA224},
	{Name: "sha256", Label: "SHA-256", Category: CategoryCryptographic, Secure: true, New: newHashSHA256},
	{Name: "sha384", Label: "SHA-384", Category: CategoryCryptographic, Secure: true, New: newHashSHA384},
	{Name: "sha512", Label: "SHA-512", Category: CategoryCryptographic, Secure: true, New: newHashSHA512},
	{Name: "sha512-224", Label: "SHA-512/224", Category: CategoryCryptographic, Secure: true, New: newHashSHA512224},
	{Name: "sha512-256", Label: "SHA-512/256", Category: CategoryCryptographic, Secure: true, New: newHashSHA512256},
	{Name: "md5", Label: "MD5", Category: CategoryLegacy, Secure: false, New: newHashMD5},
	{Name: "sha1", Label: "SHA-1", Category: CategoryLegacy, Secure: false, New: newHashSHA1},
	{Name: "adler32", Label: "Adler-32", Category: CategoryChecksum, Secure: false, New: newAdler32},
	{Name: "crc32-ieee", Label: "CRC-32/IEEE", Category: CategoryChecksum, Secure: false, New: newCRC32IEEE},
	{Name: "crc32-castagnoli", Label: "CRC-32C/Castagnoli", Category: CategoryChecksum, Secure: false, New: newCRC32Castagnoli},
	{Name: "crc32-koopman", Label: "CRC-32/Koopman", Category: CategoryChecksum, Secure: false, New: newCRC32Koopman},
	{Name: "crc64-ecma", Label: "CRC-64/ECMA", Category: CategoryChecksum, Secure: false, New: newCRC64ECMA},
	{Name: "crc64-iso", Label: "CRC-64/ISO", Category: CategoryChecksum, Secure: false, New: newCRC64ISO},
	{Name: "fnv1-32", Label: "FNV-1 32", Category: CategoryNonCrypto, Secure: false, New: newFNV132},
	{Name: "fnv1a-32", Label: "FNV-1a 32", Category: CategoryNonCrypto, Secure: false, New: newFNV1a32},
	{Name: "fnv1-64", Label: "FNV-1 64", Category: CategoryNonCrypto, Secure: false, New: newFNV164},
	{Name: "fnv1a-64", Label: "FNV-1a 64", Category: CategoryNonCrypto, Secure: false, New: newFNV1a64},
	{Name: "fnv1-128", Label: "FNV-1 128", Category: CategoryNonCrypto, Secure: false, New: newFNV1128},
	{Name: "fnv1a-128", Label: "FNV-1a 128", Category: CategoryNonCrypto, Secure: false, New: newFNV1a128},
}

var byName = buildIndex()

// Algorithms returns a copy of every registered algorithm, in display order.
func Algorithms() []Spec {
	return append([]Spec(nil), specs...)
}

// Lookup returns the algorithm registered under name.
func Lookup(name string) (Spec, bool) {
	spec, ok := byName[name]
	return spec, ok
}

func buildIndex() map[string]Spec {
	index := make(map[string]Spec, len(specs))
	for _, spec := range specs {
		index[spec.Name] = spec
	}
	return index
}
