#include <cstring>
#include <fstream>
#include <iomanip>
#include "bytearray.h"
#include "endian.h"
#include "log.h"

namespace svher {

    static Logger::ptr g_logger = LOG_NAME("sys");

    ByteArray::Node::Node(size_t s)
        : ptr(new char[s]), next(nullptr) {
    }

    ByteArray::Node::~Node() {
        delete[] ptr;
    }

    ByteArray::ByteArray(size_t base_size)
        : m_blockSize(base_size), m_capacity(base_size),
          m_endian((uint8_t)BYTE_ORDER), m_root(new Node(base_size)), m_cur(m_root) {

    }

    ByteArray::~ByteArray() {
        Node* tmp = m_root;
        while (tmp) {
            m_cur = tmp;
            tmp = tmp->next;
            delete m_cur;
        }
    }

    void ByteArray::writeFint8(const int8_t &value) {
        write(&value, 1);
    }

    void ByteArray::writeFuint8(const uint8_t &value) {
        write(&value, 1);
    }

    void ByteArray::writeFint16(const int16_t &value) {
        int16_t val = byteswap_t(value);
        write(&val, 2);
    }

    void ByteArray::writeFuint16(const uint16_t &value) {
        uint16_t val = byteswap_t(value);
        write(&val, 2);

    }

    void ByteArray::writeFint32(const int32_t &value) {
        int32_t val = byteswap_t(value);
        write(&val, 4);
    }

    void ByteArray::writeFuint32(const uint32_t &value) {
        uint32_t val = byteswap_t(value);
        write(&val, 4);

    }

    void ByteArray::writeFint64(const int64_t &value) {
        int64_t val = byteswap_t(value);
        write(&val, 8);

    }

    void ByteArray::writeFuint64(const uint64_t &value) {
        uint64_t val = byteswap_t(value);
        write(&val, 8);
    }

    void ByteArray::writeInt32(const int32_t &value) {
        writeUint32(EncodeZigzag32(value));
    }

    void ByteArray::writeUint32(const uint32_t &value) {
        uint32_t val = value;
        uint8_t tmp[5];
        uint8_t i = 0;
        while (val >= 0x80) {
            tmp[i++] = (val & 0x7Fu) | 0x80u;
            val >>= 7u;
        }
        tmp[i++]=val;
        write(tmp, i);
    }

    void ByteArray::writeInt64(const int64_t &value) {
        writeUint64(EncodeZigzag64(value));
    }

    void ByteArray::writeUint64(const uint64_t &value) {
        uint64_t val = value;
        uint8_t tmp[10];
        uint8_t i = 0;
        while (val >= 0x80) {
            tmp[i++] = (val & 0x7Fu) | 0x80u;
            val >>= 7u;
        }
        tmp[i++]=val;
        write((char*)tmp, i);
    }

    void ByteArray::writeFloat(const float &value) {
        uint32_t val;
        memcpy(&val, &value, sizeof(value));
        writeFuint32(val);
    }

    void ByteArray::writeDouble(const double &value) {
        uint64_t val;
        memcpy(&val, &value, sizeof(value));
        writeFuint64(val);
    }

    void ByteArray::writeStringF16(const std::string &value) {
        writeFuint16(value.size());
        write(value.c_str(), value.size());
    }

    void ByteArray::writeStringF32(const std::string &value) {
        writeFuint32(value.size());
        write(value.c_str(), value.size());
    }

    void ByteArray::writeStringF64(const std::string &value) {
        writeFuint64(value.size());
        write(value.c_str(), value.size());
    }

    void ByteArray::writeStringVint(const std::string &value) {
        writeUint64(value.size());
        write(value.c_str(), value.size());
    }

    void ByteArray::writeStringWithoutLength(const std::string &value) {
        write(value.c_str(), value.size());
    }

    int8_t ByteArray::readFint8() {
        int8_t val;
        read(&val, sizeof(val));
        return val;
    }

    uint8_t ByteArray::readFuint8() {
        uint8_t val;
        read(&val, sizeof(val));
        return val;
    }

    int16_t ByteArray::readFint16() {
        int16_t val;
        read(&val, sizeof(val));
        return byteswap_t(val);
    }

    uint16_t ByteArray::readFuint16() {
        uint16_t val;
        read(&val, sizeof(val));
        return byteswap_t(val);
    }

    int32_t ByteArray::readFint32() {
        int32_t val;
        read(&val, sizeof(val));
        return byteswap_t(val);
    }

    uint32_t ByteArray::readFuint32() {
        uint32_t val;
        read(&val, sizeof(val));
        return byteswap_t(val);
    }

    int64_t ByteArray::readFint64() {
        int64_t val;
        read(&val, sizeof(val));
        return byteswap_t(val);
    }

    uint64_t ByteArray::readFuint64() {
        uint64_t val;
        read(&val, sizeof(val));
        return byteswap_t(val);
    }

    int32_t ByteArray::readInt32() {
        return DecodeZigzag32(readUint32());
    }

    uint32_t ByteArray::readUint32() {
        uint32_t result = 0;
        for (uint i = 0; i < 32; i += 7) {
            uint8_t b = readFuint8();
            if (b < 0x80) {
                result |= ((uint32_t)b) << i;
                break;
            } else {
                result |= ((uint32_t)b & 0x7f) << i;
            }
        }
        return result;
    }

    int64_t ByteArray::readInt64() {
        return DecodeZigzag32(readUint32());
    }

    uint64_t ByteArray::readUint64() {
        uint64_t result = 0;
        for (uint i = 0; i < 64; i += 7) {
            uint8_t b = readFuint8();
            if (b < 0x80) {
                result |= ((uint64_t)b) << i;
                break;
            } else {
                result |= ((uint64_t)b & 0x7f) << i;
            }
        }
        return result;
    }

    float ByteArray::readFloat() {
        uint32_t v = readFuint32();
        float value;
        memcpy(&value, &v, sizeof(value));
        return value;
    }

    double ByteArray::readDouble() {
        uint64_t v = readFuint64();
        double value;
        memcpy(&value, &v, sizeof(value));
        return value;
    }

    std::string ByteArray::readStringF16() {
        uint16_t len = readFuint16();
        std::string buff;
        buff.resize(len);
        read(&buff[0], len);
        return buff;
    }

    std::string ByteArray::readStringF32() {
        uint32_t len = readFuint32();
        std::string buff;
        buff.resize(len);
        read(&buff[0], len);
        return buff;
    }

    std::string ByteArray::readStringF64() {
        uint64_t len = readFuint64();
        std::string buff;
        buff.resize(len);
        read(&buff[0], len);
        return buff;
    }

    std::string ByteArray::readStringVint() {
        uint64_t len = readUint64();
        std::string buff;
        buff.resize(len);
        read(&buff[0], len);
        return buff;
    }

    void ByteArray::clear() {
        m_position = m_size = 0;
        m_capacity = m_blockSize;
        Node* tmp = m_root->next;
        while (tmp) {
            m_cur = tmp;
            tmp = tmp->next;
            delete m_cur;
        }
        m_cur = m_root;
        m_root->next = nullptr;
    }

    void ByteArray::write(const void *buf, size_t size) {
        if (!size) return;
        addCapacity(size);
        size_t pos = 0;
        size_t npos = m_position % m_blockSize;
        size_t left = m_blockSize - npos;

        m_position += size;
        if (m_position > m_size) m_size = m_position;

        if (left > size) {
            memcpy(m_cur->ptr + npos, (char*)buf, size);
            return;
        } else {
            memcpy(m_cur->ptr + npos, (char*)buf, left);
            size -= left;
            pos += left;
        }

        size_t cnt = size / m_blockSize;
        for (size_t i = 0; i < cnt; i++) {
            m_cur = m_cur->next;
            memcpy(m_cur->ptr, (char*)buf + pos, m_blockSize);
            pos += m_blockSize;
        }
        m_cur = m_cur->next;
        if (size % m_blockSize) {
            memcpy(m_cur->ptr, (char*)buf + pos, size % m_blockSize);
        }
    }

    void ByteArray::read(void *buf, size_t size, bool peek) {
        if (size > getReadSize()) {
            throw std::out_of_range("read");
        }
        if (!size) return;
        size_t pos = 0;
        size_t npos = m_position % m_blockSize;
        size_t left = m_blockSize - npos;
        Node* cur = m_cur;

        if (!peek)
            m_position += size;

        if (left > size) {
            memcpy((char*)buf, m_cur->ptr + npos, size);
            return;
        } else {
            memcpy((char*)buf, m_cur->ptr + npos, left);
            size -= left;
            pos += left;
        }

        size_t cnt = size / m_blockSize;
        for (size_t i = 0; i < cnt; i++) {
            cur = cur->next;
            memcpy((char*)buf + pos, cur->ptr, m_blockSize);
            pos += m_blockSize;
        }
        cur = cur->next;
        if (size % m_blockSize) {
            memcpy((char*)buf + pos, cur->ptr, size % m_blockSize);
        }
        if (!peek)
            m_cur = cur;
    }

    void ByteArray::setPosition(size_t v) {
        if (v > m_size) {
            throw std::out_of_range("setPosition");
        }
        m_position = v;
        m_cur = m_root;
        while (v >= m_blockSize) {
            v -= m_blockSize;
            m_cur = m_cur->next;
        }
    }

    bool ByteArray::writeToFile(const std::string &name) const {
        std::ofstream ofs;
        ofs.open(name, std::ios::trunc | std::ios::binary);
        if (!ofs) {
            LOG_ERROR(g_logger) << "writeToFile name=" << name <<
                " errno=" << errno << " errstr=" << strerror(errno);
            return false;
        }
        size_t size = getReadSize();
        size_t npos = m_position % m_blockSize;
        size_t left = m_blockSize - npos;
        Node* cur = m_cur;

        if (left > size) {
            ofs.write(cur->ptr + npos, size);
            return true;
        } else {
            ofs.write(cur->ptr + npos , left);
            size -= left;
        }

        size_t cnt = size / m_blockSize;
        for (size_t i = 0; i < cnt; ++i) {
            cur = cur->next;
            ofs.write(cur->ptr, m_blockSize);
        }
        cur = cur->next;
        if (size % m_blockSize) {
            ofs.write(cur->ptr, size % m_blockSize);
        }
        return true;
    }

    bool ByteArray::readFromFile(const std::string &name) {
        std::ifstream ifs;
        ifs.open(name, std::ios::binary);
        if (!ifs) {
            LOG_ERROR(g_logger) << "readFromFile name=" << name <<
                                " errno=" << errno << " errstr=" << strerror(errno);
            return false;
        }
        std::shared_ptr<char> buff(new char[m_blockSize], [](char* ptr) {
            delete[] ptr;
        });
        while(!ifs.eof()) {
            ifs.read(buff.get(), m_blockSize);
            write(buff.get(), ifs.gcount());
        }
        return true;
    }

    void ByteArray::addCapacity(size_t size) {
        if (!size) return;
        if (getCapacity() > size) {
            return;
        }
        int oldCapacity = getCapacity();
        size -= oldCapacity;
        size_t count = size / m_blockSize + 1;
        Node* tmp = m_cur;
        for (size_t i = 0; i < count; ++i) {
            tmp->next = new Node(m_blockSize);
            tmp = tmp->next;
            m_capacity += m_blockSize;
        }
    }

    bool ByteArray::isLittleEndian(size_t size) const {
        return m_endian == (uint8_t)LITTLE_ENDIAN;
    }

    void ByteArray::setIsLittleEndian(bool val) {
        m_endian = val ? (uint8_t)LITTLE_ENDIAN : (uint8_t)BIG_ENDIAN;
    }

    uint32_t ByteArray::EncodeZigzag32(const int32_t &v) {
        return (v >> 31u) ^ (v << 1u);
    }

    uint64_t ByteArray::EncodeZigzag64(const int64_t &v) {
        return (v >> 63u) ^ (v << 1u);
    }

    int32_t ByteArray::DecodeZigzag32(const uint32_t &v) {
        return (v >> 1u) ^ -(v & 1u);
    }

    int64_t ByteArray::DecodeZigzag64(const uint64_t &v) {
        return (v >> 1u) ^ -(v & 1u);
    }

    std::string ByteArray::toString() {
        std::string str;
        str.resize(getReadSize());
        if (str.empty()) return str;
        read(&str[0], str.size(), true);
        return str;
    }

    std::string ByteArray::toHexString(uint16_t cnt) {
        std::string str = toString();
        std::stringstream ss;
        for (size_t i = 0; i < str.size(); ++i) {
            if (i > 0 && i % cnt == 0) {
                for (size_t j = i - cnt; j < i; j++) {
                    if (isprint(str[j])) ss << str[j];
                    else ss << '.';
                }
                ss << std::endl;
            }
            ss << std::setw(2) << std::setfill('0') << std::hex
                << (int)(uint8_t)str[i] << " ";
        }
        size_t left = str.size() % cnt;
        if (left) {
            for (size_t i = 0; i < cnt - left; ++i) {
                ss << "   ";
            }
        }
        else left = cnt;
        for (size_t i = str.size() - left; i < str.size(); ++i) {
            if (isprint(str[i])) ss << str[i];
            else ss << '.';
        }
        return ss.str();
    }

    size_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, size_t len) {
        len = len > getReadSize() ? getReadSize() : len;
        if (!len) return 0;

        uint64_t size = len;
        size_t npos = m_position % m_blockSize;
        size_t left = m_blockSize - npos;

        struct iovec iov;

        if (left > len) {
            iov.iov_base = m_cur->ptr + npos;
            iov.iov_len = len;
            buffers.push_back(iov);
            return len;
        } else {
            iov.iov_base = m_cur->ptr + npos;
            iov.iov_len = left;
            buffers.push_back(iov);
            len -= left;
        }

        size_t cnt = len / m_blockSize;
        for (size_t i = 0; i < cnt; i++) {
            m_cur = m_cur->next;
            iov.iov_base = m_cur->ptr;
            iov.iov_len = m_blockSize;
            buffers.push_back(iov);
        }
        m_cur = m_cur->next;
        if (len % m_blockSize) {
            iov.iov_base = m_cur->ptr;
            iov.iov_len = len % m_blockSize;
            buffers.push_back(iov);
        }
        return size;
    }

    uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers
            ,uint64_t len, uint64_t position) const {
        len = len > m_size - position ? m_size - position : len;
        if (!len) return 0;

        uint64_t size = len;

        size_t npos = position % m_blockSize;
        size_t cnt = position / m_blockSize;
        size_t left = m_blockSize - npos;
        Node* cur = m_root;
        while(cnt--) {
            cur = cur->next;
        }

        struct iovec iov;

        if (left > len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            buffers.push_back(iov);
            return len;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = left;
            buffers.push_back(iov);
            len -= left;
        }

        cnt = len / m_blockSize;
        for (size_t i = 0; i < cnt; i++) {
            cur = cur->next;
            iov.iov_base = cur->ptr;
            iov.iov_len = m_blockSize;
            buffers.push_back(iov);
        }
        cur = cur->next;
        if (len % m_blockSize) {
            iov.iov_base = cur->ptr;
            iov.iov_len = len % m_blockSize;
            buffers.push_back(iov);
        }
        return size;

    }

    uint64_t ByteArray::getWriteBuffers(std::vector<iovec>& buffers, uint64_t len) {
        if (!len) return 0;
        addCapacity(len);
        uint64_t size = len;
        size_t npos = m_position % m_blockSize;
        size_t left = m_blockSize - npos;

        struct iovec iov;

        m_position += len;
        if (len > m_size) m_size = len;

        if (left > len) {
            iov.iov_base = m_cur->ptr + npos;
            iov.iov_len = len;
            buffers.push_back(iov);
            return len;
        } else {
            iov.iov_base = m_cur->ptr + npos;
            iov.iov_len = left;
            buffers.push_back(iov);
            len -= left;
        }

        size_t cnt = len / m_blockSize;
        for (size_t i = 0; i < cnt; i++) {
            m_cur = m_cur->next;
            iov.iov_base = m_cur->ptr;
            iov.iov_len = m_blockSize;
            buffers.push_back(iov);
        }
        m_cur = m_cur->next;
        if (len % m_blockSize) {
            iov.iov_base = m_cur->ptr;
            iov.iov_len = len % m_blockSize;
            buffers.push_back(iov);
        }
        return size;
    }
}