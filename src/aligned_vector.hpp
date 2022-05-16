#pragma once

#include <cstdlib>

template<typename T>
class aligned_vector {
private:
    void* mPtr = nullptr;
    size_t mAlignment{}, mSize{}, mBlockSize{};

    void cleanup() {
        if (!mPtr) return;
#ifdef _WIN32
        _aligned_free(mPtr);
#else
        free(mPtr);
#endif
    }

public:
    aligned_vector() = default;

    void resize(size_t alignment, size_t size) {
        cleanup();
        mAlignment = alignment;
        mBlockSize = sizeof(T);
        if (alignment > 0) {
            mBlockSize = (mBlockSize + alignment - 1) & ~(alignment - 1);
        }
        mSize = size;
#ifdef _WIN32
        mPtr = _aligned_malloc(mBlockSize, mem_size());
#else
        mPtr = aligned_alloc(mBlockSize, mem_size());
#endif
        if (!mPtr) {
            throw std::runtime_error("Failed to allocate aligned memory");
        }
    }

    ~aligned_vector() {
        cleanup();
    }

    size_t size() {
        return mSize;
    }

    size_t alignment() {
        return mAlignment;
    }

    size_t block_size() {
        return mBlockSize;
    }

    T& operator[](size_t i) {
        auto bytePtr = reinterpret_cast<std::byte*>(mPtr);
        return *reinterpret_cast<T*>(bytePtr + i * mBlockSize);
    }

    size_t mem_size() {
        return mBlockSize * mSize;
    }

    void* data() {
        return mPtr;
    }
};
