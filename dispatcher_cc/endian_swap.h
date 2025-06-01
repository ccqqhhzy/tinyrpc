// Copyright (c) 2025 <York Zeng> All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#ifndef SERIALIZE_ENDIAN_SWAP_H
#define SERIALIZE_ENDIAN_SWAP_H

#include <arpa/inet.h>

namespace tinyrpc {
namespace cc {

// 16bit整型大小端互换
#define ENDIAN_SWAP_16(x) \
    ((((x) & 0xff00u) >> 8) | (((x) & 0x00ffu) << 8))

// 32bit整型大小端互换
#define ENDIAN_SWAP_32(x) \
    ((((x) & 0xff000000u) >> 24) | (((x) & 0x00ff0000u) >> 8) | \
     (((x) & 0x0000ff00u) <<  8) | (((x) & 0x000000ffu) << 24))

// 64bit整型大小端互换
#define ENDIAN_SWAP_64(x) \
    (((uint64_t)ENDIAN_SWAP_32((((x) & 0xffffffff00000000u) >> 32))) | \
     ((uint64_t)ENDIAN_SWAP_32(((uint32_t)((x) & 0x00000000ffffffffu))) << 32))

/*
 * 小端机器，需要转为网络字节序
 */
# if defined(__i386__) || defined(__alpha__) \
        || defined(__ia64) || defined(__ia64__) \
        || defined(_M_IX86) || defined(_M_IA64) \
        || defined(_M_ALPHA) || defined(__amd64) \
        || defined(__amd64__) || defined(_M_AMD64) \
        || defined(__x86_64) || defined(__x86_64__) \
        || defined(_M_X64) || defined(WIN32) || defined(_WIN64)
    # define XNTOH64(x)    ENDIAN_SWAP_64 (x)
    # define XNTOH32(x)    ENDIAN_SWAP_32 (x)
    # define XNTOH16(x)    ENDIAN_SWAP_16 (x)
    # define XHTON64(x)    ENDIAN_SWAP_64 (x)
    # define XHTON32(x)    ENDIAN_SWAP_32 (x)
    # define XHTON16(x)    ENDIAN_SWAP_16 (x)
# else
    # define XNTOH64(x)    (x)
    # define XNTOH32(x)    (x)
    # define XNTOH16(x)    (x)
    # define XHTON64(x)    (x)
    # define XHTON32(x)    (x)
    # define XHTON16(x)    (x)
# endif

/*
 * 判断本机大端还是小端
 * return: true 大端；false 小端
 */
inline bool isBigEndian() {
    union {
        unsigned long int i;
        unsigned char s[4];
    } c;
    c.i = 0x12345678;
    return (0x12 == c.s[0]);
}

/*
 * 转为本机字节序
 * 若本机为大端，与网络字节序同，直接返回；若本机为小端，转换成大端再返回
 * 弊端：多了 isBigEndian 判断逻辑，影响效率
 */
inline uint32_t my_ntohl(uint32_t h) { 
    return isBigEndian() ? h : ENDIAN_SWAP_32(h);
}
inline uint16_t my_ntohs(uint16_t h) { 
    return isBigEndian() ? h : ENDIAN_SWAP_16(h);
}

/*
 * 转为网络字节序
 * 若本机为大端，与网络字节序同，直接返回；若本机为小端，转换成大端再返回
 * 弊端：多了 isBigEndian 判断逻辑，影响效率
 */
inline uint32_t my_htonl(uint32_t h) { 
    return isBigEndian() ? h : ENDIAN_SWAP_32(h);
}
inline uint16_t my_htons(uint16_t h) { 
    return isBigEndian() ? h : ENDIAN_SWAP_16(h);
}

// # if __BYTE_ORDER == __BIG_ENDIAN
// /* The host byte order is the same as network byte order,
//    so these functions are all just identity.  */
// # define ntohl(x)    (x)
// # define ntohs(x)    (x)
// # define htonl(x)    (x)
// # define htons(x)    (x)
// # else
// #  if __BYTE_ORDER == __LITTLE_ENDIAN
// #   define ntohl(x)    __bswap_32 (x)
// #   define ntohs(x)    __bswap_16 (x)
// #   define htonl(x)    __bswap_32 (x)
// #   define htons(x)    __bswap_16 (x)
// #  endif
// # endif
 /* Swap bytes in 32 bit value.  */
/*
#define __bswap_constant_32(x) \
      ((((x) & 0xff000000u) >> 24) | (((x) & 0x00ff0000u) >>  8) |         \
       (((x) & 0x0000ff00u) <<  8) | (((x) & 0x000000ffu) << 24))
*/

} // namespace cc
} // namespace tinyrpc

#endif // SERIALIZE_ENDIAN_SWAP_H