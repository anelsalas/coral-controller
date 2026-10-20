#pragma once
#include "filters/IFilter.h"

#include <string>

namespace coral {

// Convenience base: stores name + parameters. A new filter only declares
// `static constexpr std::string_view kName`, its parameters, and apply().
class FilterBase : public IFilter {
public:
    std::string_view name() const override;
    std::vector<FilterParameter>& parameters() override;
    const std::vector<FilterParameter>& parameters() const override;

protected:
    FilterBase(std::string name, std::vector<FilterParameter> parameters);
    const FilterParameter& parameter(std::size_t index) const;

private:
    std::string name_;
    std::vector<FilterParameter> parameters_;
};

}  // namespace coral