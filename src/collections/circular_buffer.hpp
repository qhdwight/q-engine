#pragma once

#include <array>

template<typename T, size_t Size>
class circular_buffer {
private:
    std::array<T, Size> mArray{};
    size_t mPointer = Size;

    void advance_pointer() {
        mPointer = (mPointer + 1) % Size;
    }

public:
    void push(T const& element) {
        advance_pointer();
        mArray[mPointer] = std::forward<T>(element);
    }

    void push(T&& element) {
        advance_pointer();
        mArray[mPointer] = std::move(element);
    }

    T& peek(size_t rollback = 0) {
        if (rollback >= Size) {
            throw std::runtime_error("Rollback too much");
        }
        size_t pointer = mPointer;
        if (rollback >= mPointer)
            mPointer += Size;
        return mArray[pointer % Size];
    }
};
