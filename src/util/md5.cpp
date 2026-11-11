/*
 * From-scratch MD5 implementation following RFC 1321
 * (https://www.rfc-editor.org/rfc/rfc1321). Written to expose the same
 * interface as libmd (https://www.hadrons.org/software/libmd/), which
 * Jankatay vendored (as a prebuilt static library) on the experiments
 * branch; this source-only implementation keeps the build portable.
 *
 * Verified against the RFC 1321 test suite (see project notes):
 *   MD5("")          = d41d8cd98f00b204e9800998ecf8427e
 *   MD5("abc")       = 900150983cd24fb0d6963f7d28e17f72
 *   MD5("The quick brown fox jumps over the lazy dog")
 *                    = 9e107d9d372bb6826bd81d3542a419d6
 */

#include "md5.hpp"

#include <cstdio>
#include <cstring>

/* Per-round shift amounts, RFC 1321 section 2 */
static const uint32_t MD5_SHIFT[64] = {
    7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
    5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
    4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
    6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};

/* K[i] = floor(abs(sin(i + 1)) * 2^32), RFC 1321 section 2 */
static const uint32_t MD5_K[64] = {
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
    0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
    0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
    0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
    0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
    0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
    0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
    0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
    0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
    0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
    0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
    0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
    0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
    0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391};

/* Padding byte used at the start of message padding (0x80) per RFC 1321 */
static const uint8_t MD5_PADDING[MD5_BLOCK_LENGTH] = {0x80};

/* Rotate a 32-bit value left by n bits */
static inline uint32_t md5_rotl(uint32_t value, uint32_t n)
{
    return (value << n) | (value >> (32 - n));
}

extern "C"
{

    /// Loads the RFC 1321 initialization vector (A=67452301 etc.) and zeroes the bit count
    void MD5Init(MD5_CTX* ctx)
    {
        ctx -> count = 0;
        ctx -> state[0] = 0x67452301;
        ctx -> state[1] = 0xefcdab89;
        ctx -> state[2] = 0x98badcfe;
        ctx -> state[3] = 0x10325476;
    }

    /// Runs the 64 compression rounds (4 rounds x 16) over one 64-byte block
    void MD5Transform(uint32_t state[4], const uint8_t block[MD5_BLOCK_LENGTH])
    {
        uint32_t a = state[0], b = state[1], c = state[2], d = state[3];

        // Decode the 64-byte block into sixteen little-endian 32-bit words
        uint32_t words[16];
        for(int i = 0; i < 16; i++)
        {
            words[i] = (uint32_t)block[i * 4] | ((uint32_t)block[i * 4 + 1] << 8) | ((uint32_t)block[i * 4 + 2] << 16) | ((uint32_t)block[i * 4 + 3] << 24);
        }

        // Main loop: 4 rounds of 16 operations each (RFC 1321 section 3)
        for(int i = 0; i < 64; i++)
        {
            uint32_t f;
            int g;
            switch(i >> 4)
            {
            case 0:
                f = (b & c) | (~b & d);
                g = i;
                break; // Round 1
            case 1:
                f = (d & b) | (~d & c);
                g = (5 * i + 1) % 16;
                break; // Round 2
            case 2:
                f = b ^ c ^ d;
                g = (3 * i + 5) % 16;
                break; // Round 3
            default:
                f = c ^ (b | ~d);
                g = (7 * i) % 16;
                break; // Round 4
            }

            f += a + MD5_K[i] + words[g];
            a = d;
            d = c;
            c = b;
            b += md5_rotl(f, MD5_SHIFT[i]);
        }

        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
    }

    /// Buffers arbitrary-length input and transforms each full 64-byte block
    void MD5Update(MD5_CTX* ctx, const uint8_t* data, size_t len)
    {
        // Number of bytes currently buffered (count is in bits)
        size_t used = (size_t)((ctx -> count >> 3) & 0x3f);
        size_t space = MD5_BLOCK_LENGTH - used;

        ctx -> count += (uint64_t)len << 3;

        if(len >= space)
        {
            // Fill the buffer and flush it
            std::memcpy(ctx -> buffer + used, data, space);
            MD5Transform(ctx -> state, ctx -> buffer);
            data += space;
            len -= space;

            // Process full blocks straight from the input
            while(len >= MD5_BLOCK_LENGTH)
            {
                MD5Transform(ctx -> state, data);
                data += MD5_BLOCK_LENGTH;
                len -= MD5_BLOCK_LENGTH;
            }

            used = 0;
        }

        // Buffer whatever is left
        if(len > 0)
            std::memcpy(ctx -> buffer + used, data, len);
    }

    /// Appends 0x80 + zeros to 56 mod 64, then the 8-byte little-endian bit count
    void MD5Pad(MD5_CTX* ctx)
    {
        // Encode the bit count as eight little-endian bytes BEFORE padding
        uint8_t count_bytes[8];
        for(int i = 0; i < 8; i++)
            count_bytes[i] = (uint8_t)((ctx -> count >> (8 * i)) & 0xff);

        // Pad to 56 mod 64 bytes, then append the bit count
        size_t used = (size_t)((ctx -> count >> 3) & 0x3f);
        size_t padlen = (used < 56) ? (56 - used) : (120 - used);
        MD5Update(ctx, MD5_PADDING, padlen);
        MD5Update(ctx, count_bytes, 8);
    }

    /// Pads and emits the 16-byte little-endian digest, then wipes the context
    void MD5Final(uint8_t digest[MD5_DIGEST_LENGTH], MD5_CTX* ctx)
    {
        MD5Pad(ctx);

        // Encode the state as sixteen little-endian bytes
        for(int i = 0; i < 4; i++)
        {
            digest[i * 4] = (uint8_t)(ctx -> state[i] & 0xff);
            digest[i * 4 + 1] = (uint8_t)((ctx -> state[i] >> 8) & 0xff);
            digest[i * 4 + 2] = (uint8_t)((ctx -> state[i] >> 16) & 0xff);
            digest[i * 4 + 3] = (uint8_t)((ctx -> state[i] >> 24) & 0xff);
        }

        std::memset(ctx, 0, sizeof(*ctx)); // Don't leave key material behind
    }

    /// Finalizes and renders the digest as 32 lowercase hex characters
    char* MD5End(MD5_CTX* ctx, char* buf)
    {
        if(buf == nullptr)
            return nullptr;

        uint8_t digest[MD5_DIGEST_LENGTH];
        MD5Final(digest, ctx);

        for(int i = 0; i < MD5_DIGEST_LENGTH; i++)
            std::snprintf(buf + i * 2, 3, "%02x", digest[i]);
        buf[MD5_DIGEST_STRING_LENGTH - 1] = '\0';

        return buf;
    }

    /// One-shot: hashes an in-memory buffer to hex
    char* MD5Data(const uint8_t* data, size_t len, char* buf)
    {
        MD5_CTX ctx;
        MD5Init(&ctx);
        MD5Update(&ctx, data, len);
        return MD5End(&ctx, buf);
    }

    /// One-shot: hashes a file's contents to hex (nullptr on open/read errors)
    char* MD5File(const char* filename, char* buf)
    {
        FILE* file = std::fopen(filename, "rb");
        if(file == nullptr)
            return nullptr;

        MD5_CTX ctx;
        MD5Init(&ctx);

        uint8_t chunk[4096];
        size_t read_bytes;
        while((read_bytes = std::fread(chunk, 1, sizeof(chunk), file)) > 0)
            MD5Update(&ctx, chunk, read_bytes);

        bool read_error = std::ferror(file) != 0;
        std::fclose(file);

        if(read_error)
            return nullptr;

        return MD5End(&ctx, buf);
    }

} // extern "C"
