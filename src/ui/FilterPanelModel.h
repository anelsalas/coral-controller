#pragma once
#include "filters/FilterPipeline.h"

namespace coral {

// UI-toolkit-independent view of the pipeline for the filter panel (VIEW-008).
class FilterPanelModel {
public:
    explicit FilterPanelModel(FilterPipeline& pipeline);

    std::size_t filterCount() const;
    std::string_view filterName(std::size_t filterIndex) const;
    bool isEnabled(std::size_t filterIndex) const;
    void setEnabled(std::size_t filterIndex, bool enabled);
    const std::vector<FilterParameter>& parameters(std::size_t filterIndex) const;
    void setParameter(std::size_t filterIndex, std::size_t parameterIndex, double value);
    void moveUp(std::size_t filterIndex);
    void moveDown(std::size_t filterIndex);

private:
    FilterPipeline& pipeline_;
};

}  // namespace coral