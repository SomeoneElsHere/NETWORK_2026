#ifndef UTIL_H
#define UTIL_H

#include <cstdint>
#include <string>
#include <bitset>
#include <cmath>

using namespace std;

// MD5 outputs 128 bytes computed by splitting into 4*32.
struct MD5 {
  uint32_t A, B, C, D; // From the most significant 'A' to the least significant bytes 'D'
};


// Pad a string with 'pad_byte' up to some given bytes
// (new_len is counted WITHOUT the null-byte)
// Returns false on error and true on success
bool string_pad(string& str, char pad_byte, size_t new_len) {
  size_t old_len = str.length();
  if(old_len > new_len) return false; // pad would have to truncate
  str.append(new_len - old_len, pad_byte);
  return true;
}

// Calculate a power a^b at compile-time
// May change with the faster power algorithm later.
constexpr ssize_t pow_constexpr(size_t a, size_t b) {
  size_t ret = 1;
  for(int i = 0; i < b; i++) ret *= a;
  return ret;
}

// Pad a string for md5 specifically
// Returns false on error and true on success
template <size_t old_len>
bool md5_pad(bitset<old_len>& bits) {
  // init
  string str = bits.to_string();

  // append 1
  str.push_back('1');

  // pad with 0s to up to some 512x-64
  const size_t add_len = (old_len + 1) % (512 - 64); // +1 for the appended '1'
  const size_t new_len = old_len + add_len; // new_len WITHOUT the last 64 bits
  if(!string_pad(str, '\0', new_len)) return false;

  // append the old_len in bits modulo 2^64
  constexpr ssize_t CONST_PAD_LEN_VAL = pow_constexpr(2, 64);
  static_assert(CONST_PAD_LEN_VAL != 0, "util.h: CONST_PAD_LEN_VAL is somehow 0");
  string len = to_string(old_len % CONST_PAD_LEN_VAL);
  //TODO: finish rest of the padding
  return true;
}

// get MD5 hash for a 
MD5 md5(){
  return MD5{0, 0, 0, 0};
}

#endif
