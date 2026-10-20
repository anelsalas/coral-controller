#pragma once
#include "filters/FilterBase.h"

#include <stdexcept>

namespace coral::testing {

class AddConstantFilter final : public FilterBase {
public:
    static constexpr std::string_view kName = "Add (test)";
    explicit AddConstantFilter(double amount = 10)
        : FilterBase(std::string(kName), {FilterParameter::real("Amount", amount, 0, 100)}) {}
    cv::Mat apply(const cv::Mat& input) const override { return input + parameter(0).asReal(); }
};

class DoubleFilter final : public FilterBase {
public:
    static constexpr std::string_view kName = "Double (test)";
    DoubleFilter() : FilterBase(std::string(kName), {}) {}
    cv::Mat apply(const cv::Mat& input) const override { return input * 2; }
};

class FailingFilter final : public FilterBase {
public:
    static constexpr std::string_view kName = "Failing (test)";
    FailingFilter() : FilterBase(std::string(kName), {}) {}
    cv::Mat apply(const cv::Mat&) const override { throw std::runtime_error("deliberate failure"); }
};

}  // namespace coral::testing