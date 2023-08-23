export module collections.aligned_vector;

import std;

export template<typename T>
class aligned_vector {
private:
    void* mPtr = nullptr;
    std::size_t mAlignment{}, mSize{}, mBlockSize{};

    void cleanup() {
        if (!mPtr) return;
#ifdef _WIN32
        _aligned_free(mPtr);
#else
        std::free(mPtr);
#endif
    }

public:
    aligned_vector() = default;

    void resize(std::size_t alignment, std::size_t size) {
        cleanup();
        mAlignment = alignment;
        mBlockSize = sizeof(T);
        if (alignment > 0) {
            mBlockSize = (mBlockSize + alignment - 1) & ~(alignment - 1);
        }
        mSize = size;
#ifdef _WIN32
        mPtr = _aligned_malloc(mem_size(), mAlignment);
#else
        mPtr = std::aligned_alloc(mAlignment, mem_size());
#endif
        if (!mPtr) {
            throw std::runtime_error("Failed to allocate aligned memory");
        }
    }

    ~aligned_vector() {
        cleanup();
    }

    std::size_t size() {
        return mSize;
    }

    std::size_t alignment() {
        return mAlignment;
    }

    std::size_t block_size() {
        return mBlockSize;
    }

    T& operator[](std::size_t i) {
        auto bytePtr = reinterpret_cast<std::byte*>(mPtr);
        return *reinterpret_cast<T*>(bytePtr + i * mBlockSize);
    }

    std::size_t mem_size() {
        return mBlockSize * mSize;
    }

    void* data() {
        return mPtr;
    }
};
