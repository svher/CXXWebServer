#pragma once

#include <memory>
#include <string>
#include <bits/types/struct_iovec.h>
#include <vector>

namespace svher {
    class ByteArray {
    public:
        typedef std::shared_ptr<ByteArray> ptr;

        struct Node {
            Node(size_t s);
            ~Node();

            char* ptr;
            Node* next;
        };

        ByteArray(size_t base_size = 4096);
        ~ByteArray();

        void writeFint8(const int8_t& value);
        void writeFuint8(const uint8_t &value);
        void writeFint16(const int16_t& value);
        void writeFuint16(const uint16_t &value);
        void writeFint32(const int32_t& value);
        void writeFuint32(const uint32_t &value);
        void writeFint64(const int64_t& value);
        void writeFuint64(const uint64_t &value);

        static uint32_t EncodeZigzag32(const int32_t& v);
        static uint64_t EncodeZigzag64(const int64_t& v);
        static int32_t DecodeZigzag32(const uint32_t& v);
        static int64_t DecodeZigzag64(const uint64_t& v);

        void writeInt32(const int32_t& value);
        void writeUint32(const uint32_t &value);
        void writeInt64(const int64_t& value);
        void writeUint64(const uint64_t &value);

        void writeFloat(const float& value);
        void writeDouble(const double& value);
        void writeStringF16(const std::string& value);
        void writeStringF32(const std::string& value);
        void writeStringF64(const std::string& value);
        void writeStringVint(const std::string& value);
        void writeStringWithoutLength(const std::string& value);

        int8_t readFint8();
        uint8_t readFuint8();
        int16_t readFint16();
        uint16_t readFuint16();
        int32_t readFint32();
        uint32_t readFuint32();
        int64_t readFint64();
        uint64_t readFuint64();

        int32_t readInt32();
        uint32_t readUint32();
        int64_t readInt64();
        uint64_t readUint64();

        float readFloat();
        double readDouble();

        std::string readStringF16();
        std::string readStringF32();
        std::string readStringF64();
        std::string readStringVint();

        void clear();

        void write(const void *buf, size_t size);
        void read(void *buf, size_t size, bool peek = false);

        size_t getPosition() const { return m_position; }
        void setPosition(size_t v);

        bool writeToFile(const std::string& name) const;
        bool readFromFile(const std::string& name);

        size_t getBlockSize() const { return m_blockSize; }
        size_t getReadSize() const { return m_size - m_position; }
        size_t getSize() const { return m_size; }

        bool isLittleEndian(size_t size) const;
        void setIsLittleEndian(bool val);

        std::string toString();
        std::string toHexString(uint16_t cnt = 16);

        size_t getReadBuffers(std::vector<iovec>& buffers, size_t len = ~0ull);
        size_t getReadBuffers(std::vector<iovec>& buffers, size_t len, size_t position) const;
        size_t getWriteBuffers(std::vector<iovec>& buffers, size_t len);
    private:
        void addCapacity(size_t size);
        size_t getCapacity() const { return m_capacity - m_position; }
        // 每一个 node 的大小
        size_t m_blockSize;
        size_t m_position = 0;
        size_t m_capacity;
        size_t m_size = 0;
        uint8_t m_endian;
        Node* m_root;
        Node* m_cur;
    };
}