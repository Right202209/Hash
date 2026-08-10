package registry

import (
	"crypto/md5"
	"crypto/sha1"
	"crypto/sha256"
	"crypto/sha512"
	"hash"
	"hash/adler32"
	"hash/crc32"
	"hash/crc64"
	"hash/fnv"
)

func newHashMD5() hash.Hash       { return md5.New() }
func newHashSHA1() hash.Hash      { return sha1.New() }
func newHashSHA224() hash.Hash    { return sha256.New224() }
func newHashSHA256() hash.Hash    { return sha256.New() }
func newHashSHA384() hash.Hash    { return sha512.New384() }
func newHashSHA512() hash.Hash    { return sha512.New() }
func newHashSHA512224() hash.Hash { return sha512.New512_224() }
func newHashSHA512256() hash.Hash { return sha512.New512_256() }
func newAdler32() hash.Hash       { return adler32.New() }
func newCRC32IEEE() hash.Hash     { return crc32.New(crc32.MakeTable(crc32.IEEE)) }
func newCRC32Castagnoli() hash.Hash {
	return crc32.New(crc32.MakeTable(crc32.Castagnoli))
}
func newCRC32Koopman() hash.Hash { return crc32.New(crc32.MakeTable(crc32.Koopman)) }
func newCRC64ECMA() hash.Hash    { return crc64.New(crc64.MakeTable(crc64.ECMA)) }
func newCRC64ISO() hash.Hash     { return crc64.New(crc64.MakeTable(crc64.ISO)) }
func newFNV132() hash.Hash       { return fnv.New32() }
func newFNV1a32() hash.Hash      { return fnv.New32a() }
func newFNV164() hash.Hash       { return fnv.New64() }
func newFNV1a64() hash.Hash      { return fnv.New64a() }
func newFNV1128() hash.Hash      { return fnv.New128() }
func newFNV1a128() hash.Hash     { return fnv.New128a() }
