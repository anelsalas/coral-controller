#include "filters/FilterParameter.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace coral {

FilterParameter::FilterParameter(std::string name, ParameterKind kind, double minimum, double maximum,
                                 std::vector<std::string> options, double initial)
    : name_(std::move(name)),
      kind_(kind),
      minimum_(minimum),
      maximum_(maximum),
      options_(std::move(options)),
      value_(0) {
    if (minimum_ > maximum_) throw std::invalid_argument("parameter '" + name_ + "': minimum > maximum");
    setValue(initial);
}

FilterParameter FilterParameter::integer(std::string name, int initial, int minimum, int maximum) {
    return FilterParameter(std::move(name), ParameterKind::Integer, minimum, maximum, {}, initial);
}

FilterParameter FilterParameter::real(std::string name, double initial, double minimum, double maximum) {
    return FilterParameter(std::move(name), ParameterKind::Real, minimum, maximum, {}, initial);
}

FilterParameter FilterParameter::boolean(std::string name, bool initial) {
    return FilterParameter(std::move(name), ParameterKind::Boolean, 0, 1, {}, initial ? 1 : 0);
}

FilterParameter FilterParameter::choice(std::string name, std::vector<std::string> options, int initialIndex) {
    if (options.empty()) throw std::invalid_argument("choice parameter '" + name + "' needs options");
    const double last = static_cast<double>(options.size() - 1);
    return FilterParameter(std::move(name), ParameterKind::Choice, 0, last, std::move(options), initialIndex);
}

const std::string& FilterParameter::name() const { return name_; }
ParameterKind FilterParameter::kind() const { return kind_; }
double FilterParameter::minimum() const { return minimum_; }
double FilterParameter::maximum() const { return maximum_; }
const std::vector<std::string>& FilterParameter::options() const { return options_; }

void FilterParameter::setValue(double value) { value_ = normalized(value); }

double FilterParameter::normalized(double value) const {
    if (kind_ == ParameterKind::Boolean) return value != 0 ? 1 : 0;
    const double clamped = std::clamp(value, minimum_, maximum_);
    return kind_ == ParameterKind::Real ? clamped : std::round(clamped);
}

double FilterParameter::value() const { return value_; }
int FilterParameter::asInt() const { return static_cast<int>(value_); }
double FilterParameter::asReal() const { return value_; }
bool FilterParameter::asBool() const { return value_ != 0; }
int FilterParameter::asChoiceIndex() const { return static_cast<int>(value_); }

}  // namespace coral