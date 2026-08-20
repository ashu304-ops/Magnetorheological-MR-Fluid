#pragma once

#include <vector>
#include <numeric>
#include <algorithm>
#include <cstddef>
#include <cmath>

class SignalFilter {
public:
    explicit SignalFilter(size_t windowSize = 5)
        : windowSize_(windowSize) {
        history_.reserve(windowSize);
    }

    float filter(float rawValue) {
        if (history_.size() >= windowSize_) {
            history_.erase(history_.begin());
        }

        history_.push_back(rawValue);

        float sum =
            std::accumulate(
                history_.begin(),
                history_.end(),
                0.0f
            );

        float avg =
            sum / static_cast<float>(history_.size());

        return std::clamp(avg, -5.0f, 5.0f);
    }

    float getPeak() const {
        if (history_.empty()) {
            return 0.0f;
        }

        auto maxIt =
            std::max_element(
                history_.begin(),
                history_.end(),
                [](float a, float b) {
                    return std::abs(a) < std::abs(b);
                }
            );

        return *maxIt;
    }

    // Clear old sensor samples after a fault.
    void reset() noexcept {
        history_.clear();
    }

private:
    size_t windowSize_;
    std::vector<float> history_;
};