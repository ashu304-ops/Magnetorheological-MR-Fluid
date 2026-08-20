#pragma once

#include <array>
#include <cstddef>
#include <optional>


template <typename T, std::size_t Capacity>
class RingBuffer
{
    static_assert(
        Capacity > 0,
        "RingBuffer capacity must be greater than zero"
    );


public:

    constexpr RingBuffer() noexcept = default;


    /* ========================================================
     * Push item
     *
     * If full, overwrite oldest item.
     * ======================================================== */

    bool push(const T& item) noexcept
    {
        data_[head_] = item;

        head_ =
            (head_ + 1U) % Capacity;


        if (size_ < Capacity)
        {
            ++size_;
        }
        else
        {
            tail_ =
                (tail_ + 1U) % Capacity;
        }

        return true;
    }


    /* ========================================================
     * Pop oldest item
     * ======================================================== */

    [[nodiscard]]
    std::optional<T> pop() noexcept
    {
        if (size_ == 0U)
        {
            return std::nullopt;
        }


        T item = data_[tail_];

        tail_ =
            (tail_ + 1U) % Capacity;

        --size_;

        return item;
    }


    [[nodiscard]]
    constexpr std::size_t size() const noexcept
    {
        return size_;
    }


    [[nodiscard]]
    constexpr std::size_t capacity() const noexcept
    {
        return Capacity;
    }


    [[nodiscard]]
    constexpr bool empty() const noexcept
    {
        return size_ == 0U;
    }


private:

    std::array<T, Capacity> data_{};

    std::size_t head_{0U};

    std::size_t tail_{0U};

    std::size_t size_{0U};
};