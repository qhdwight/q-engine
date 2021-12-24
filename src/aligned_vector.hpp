#pragma once

[[nodiscard]] static void* alignedAlloc(size_t size, size_t alignment) {
#if defined(_MSC_VER) || defined(__MINGW32__)
    return _aligned_malloc(size, alignment);
#else
    void* data;
    int res = posix_memalign(&data, alignment, size);
    return res ? nullptr : data;
#endif
}

static void alignedFree(void* data) {
#if defined(_MSC_VER) || defined(__MINGW32__)
    _aligned_free(data);
#else
    free(data);
#endif
}

template<typename T>
class aligned_vector {
private:
    void* mPtr = nullptr;
    size_t mAlignment{}, mSize{}, mBlockSize{};

    void free() {
        if (!mPtr) return;
        alignedFree(mPtr);
    }

public:
    aligned_vector() = default;

    void resize(size_t alignment, size_t size) {
        free();
        mAlignment = alignment;
        mBlockSize = sizeof(T);
        if (alignment > 0) {
            mBlockSize = (mBlockSize + alignment - 1) & ~(alignment - 1);
        }
        mSize = size;
        mPtr = alignedAlloc(mem_size(), mBlockSize);
        if (!mPtr) {
            throw std::runtime_error("Failed to allocate aligned memory");
        }
    }

    ~aligned_vector() {
        free();
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
