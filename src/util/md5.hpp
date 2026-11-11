#pragma once

/*
 * MD5 message-digest algorithm (RFC 1321, Ronald Rivest).
 *
 * Interface is intentionally compatible with libmd
 * (https://www.hadrons.org/software/libmd/) so that code written against
 * Jankatay's vendored "src/libmd" on the experiments branch works
 * unchanged with this header. The algorithm in md5.cpp is a from-scratch
 * implementation following RFC 1321; no external code was copied.
 *
 * External citations:
 *   - RFC 1321: https://www.rfc-editor.org/rfc/rfc1321
 *   - libmd API: https://www.hadrons.org/software/libmd/
 */

#include <stddef.h>
#include <stdint.h>

#define MD5_BLOCK_LENGTH 64
#define MD5_DIGEST_LENGTH 16
#define MD5_DIGEST_STRING_LENGTH (MD5_DIGEST_LENGTH * 2 + 1)

typedef struct MD5Context
{
    uint32_t state[4];                /* state (ABCD) */
    uint64_t count;                   /* number of bits, mod 2^64 */
    uint8_t buffer[MD5_BLOCK_LENGTH]; /* input buffer */
} MD5_CTX;

#ifdef __cplusplus
extern "C"
{
#endif

    void MD5Init(MD5_CTX*);
    void MD5Update(MD5_CTX*, const uint8_t*, size_t);
    void MD5Pad(MD5_CTX*);
    void MD5Final(uint8_t[MD5_DIGEST_LENGTH], MD5_CTX*);
    void MD5Transform(uint32_t[4], const uint8_t[MD5_BLOCK_LENGTH]);

    /* The following return buf on success, nullptr on failure.            */
    /* buf must have room for MD5_DIGEST_STRING_LENGTH characters.         */
    char* MD5End(MD5_CTX*, char* buf);
    char* MD5Data(const uint8_t* data, size_t len, char* buf);
    char* MD5File(const char* filename, char* buf);

#ifdef __cplusplus
}
#endif
