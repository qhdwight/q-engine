export module collections.circular_buffer;

import std;

export template<typename T, std::size_t Size>
class circular_buffer {
private:
    std::array<T, Size> mArray{};
    std::size_t mHead = 0;

public:
    void advance() {
        mHead = (mHead + 1) % Size;
    }

    void push(T const& element) {
        advance();
        mArray[mHead] = std::forward<T>(element);
    }

    void push(T&& element) {
        advance();
        mArray[mHead] = std::move(element);
    }

    T& advance_and_grab() {
        advance();
        return peek();
    }

    T& peek() {
        return mArray[mHead];
    }

    T& peek(std::ptrdiff_t offset) {
        if (offset >= Size) {
            throw std::runtime_error("Query offset bigger than buffer size!");
        }
        std::size_t queryHead = mHead;
        if (offset >= mHead)
            queryHead += Size;
        return mArray[queryHead % Size];
    }
};