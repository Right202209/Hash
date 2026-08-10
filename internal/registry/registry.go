package registry

import "hash"

type Category string

const (
	CategoryCryptographic Category = "cryptographic"
	CategoryLegacy        Category = "legacy"
	CategoryChecksum      Category = "checksum"
	CategoryNonCrypto     Category = "non-cryptographic"
)

type Spec struct {
	Name     string
	Label    string
	Category Category
	Secure   bool
	New      func() hash.Hash
}

var specs = []Spec{
	{Name: "sha224", Label: "SHA-224", Category: CategoryCryptographic, Secure: true, New: func() hash.Hash { return newHashSHA224() }},
	{Name: "sha256", Label: "SHA-256", Category: CategoryCryptographic, Secure: true, New: func() hash.Hash { return newHashSHA256() }},
	{Name: "sha384", Label: "SHA-384", Category: CategoryCryptographic, Secure: true, New: func() hash.Hash { return newHashSHA384() }},
	{Name: "sha512", Label: "SHA-512", Category: CategoryCryptographic, Secure: true, New: func() hash.Hash { return newHashSHA512() }},
	{Name: "sha512-224", Label: "SHA-512/224", Category: CategoryCryptographic, Secure: true, New: func() hash.Hash { return newHashSHA512224() }},
	{Name: "sha512-256", Label: "SHA-512/256", Category: CategoryCryptographic, Secure: true, New: func() hash.Hash { return newHashSHA512256() }},
	{Name: "md5", Label: "MD5", Category: CategoryLegacy, Secure: false, New: func() hash.Hash { return newHashMD5() }},
	{Name: "sha1", Label: "SHA-1", Category: CategoryLegacy, Secure: false, New: func() hash.Hash { return newHashSHA1() }},
	{Name: "adler32", Label: "Adler-32", Category: CategoryChecksum, Secure: false, New: func() hash.Hash { return newAdler32() }},
	{Name: "crc32-ieee", Label: "CRC-32/IEEE", Category: CategoryChecksum, Secure: false, New: func() hash.Hash { return newCRC32IEEE() }},
	{Name: "crc32-castagnoli", Label: "CRC-32C/Castagnoli", Category: CategoryChecksum, Secure: false, New: func() hash.Hash { return newCRC32Castagnoli() }},
	{Name: "crc32-koopman", Label: "CRC-32/Koopman", Category: CategoryChecksum, Secure: false, New: func() hash.Hash { return newCRC32Koopman() }},
	{Name: "crc64-ecma", Label: "CRC-64/ECMA", Category: CategoryChecksum, Secure: false, New: func() hash.Hash { return newCRC64ECMA() }},
	{Name: "crc64-iso", Label: "CRC-64/ISO", Category: CategoryChecksum, Secure: false, New: func() hash.Hash { return newCRC64ISO() }},
	{Name: "fnv1-32", Label: "FNV-1 32", Category: CategoryNonCrypto, Secure: false, New: func() hash.Hash { return newFNV132() }},
	{Name: "fnv1a-32", Label: "FNV-1a 32", Category: CategoryNonCrypto, Secure: false, New: func() hash.Hash { return newFNV1a32() }},
	{Name: "fnv1-64", Label: "FNV-1 64", Category: CategoryNonCrypto, Secure: false, New: func() hash.Hash { return newFNV164() }},
	{Name: "fnv1a-64", Label: "FNV-1a 64", Category: CategoryNonCrypto, Secure: false, New: func() hash.Hash { return newFNV1a64() }},
	{Name: "fnv1-128", Label: "FNV-1 128", Category: CategoryNonCrypto, Secure: false, New: func() hash.Hash { return newFNV1128() }},
	{Name: "fnv1a-128", Label: "FNV-1a 128", Category: CategoryNonCrypto, Secure: false, New: func() hash.Hash { return newFNV1a128() }},
}

var byName = buildIndex()

func Algorithms() []Spec {
	return append([]Spec(nil), specs...)
}

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
