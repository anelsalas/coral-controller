#include "filters/FilterBase.h"

namespace coral {

FilterBase::FilterBase(std::string name, std::vector<FilterParameter> parameters)
    : name_(std::move(name)), parameters_(std::move(parameters)) {}

std::string_view FilterBase::name() const { return name_; }
std::vector<FilterParameter>& FilterBase::parameters() { return parameters_; }
const std::vector<FilterParameter>& FilterBase::parameters() const { return parameters_; }
const FilterParameter& FilterBase::parameter(std::size_t index) const { return parameters_.at(index); }

}  // namespace coral