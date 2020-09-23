#pragma once

#include <byteswap.h>
#include <stdint.h>

namespace svher {
    template <class T>
    // 模板仅当 sizeof(T) == sizeof(uint64_t) 时启用
    typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
    byteswap(T value) {
        return (T)bswap_64((uint64_t)value);
    }

    template <class T>
    typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
    byteswap(T value) {
        return (T)bswap_32((uint32_t)value);
    }

    template <class T>
    typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
    byteswap(T value) {
        return (T)bswap_16((uint16_t)value);
    }

    // 如果本来是大端则不需要做转化，否则做 byteswap
#if BYTE_ORDER == BIG_ENDIAN
    template <class T>
    T byteswap_t(T t) {
        return t;
    }
#else
    template <class T>
    T byteswap_t(T t) {
        return byteswap(t);
    }
#endif
}