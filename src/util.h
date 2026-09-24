#ifndef UTIL_H
#define UTIL_H

#include <cstdint>

// MD5 outputs 128 bytes computed by splitting into 4*32.
struct MD5 {
  uint32_t A, B, C, D; // From the most significant 'A' to the least significant bytes 'D'
};


// get MD5 hash for a 
MD5 md5(){
}

#endif
