#pragma once

#include <array>
#include <cstddef>
#include <algorithm>
#include <cmath>

template <std::size_t Capacity>
class SignalFilter
{
    static_assert(Capacity > 0U, "SignalFilter capacity must be greater than zero");

public:

    constexpr SignalFilter() noexcept = default;

    float filter(float rawValue) noexcept
    {
        history_[head_] = rawValue;

        head_ = (head_ + 1U) % Capacity;

        if (count_ < Capacity)
        {
            ++count_;
        }

        float sum = 0.0f;

        for (std::size_t i = 0U; i < count_; ++i)
        {
            sum += history_[i];
        }

        const float average =
            sum / static_cast<float>(count_);

        return std::clamp(
            average,
            -5.0f,
            5.0f
        );
    }

    float getPeak() const noexcept
    {
        if (count_ == 0U)
        {
            return 0.0f;
        }

        float peak = history_[0];

        for (std::size_t i = 1U; i < count_; ++i)
        {
            if (std::abs(history_[i]) >
                std::abs(peak))
            {
                peak = history_[i];
            }
        }

        return peak;
    }

    void reset() noexcept
    {
        history_.fill(0.0f);
        head_ = 0U;
        count_ = 0U;
    }

    [[nodiscard]]
    constexpr std::size_t size() const noexcept
    {
        return count_;
    }

    [[nodiscard]]
    constexpr std::size_t capacity() const noexcept
    {
        return Capacity;
    }

private:

    std::array<float, Capacity> history_{};

    std::size_t head_{0U};

    std::size_t count_{0U};
};