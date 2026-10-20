#include "ui/FilterPanelModel.h"

namespace coral {

FilterPanelModel::FilterPanelModel(FilterPipeline& pipeline) : pipeline_(pipeline) {}

std::size_t FilterPanelModel::filterCount() const { return pipeline_.size(); }

std::string_view FilterPanelModel::filterName(std::size_t filterIndex) const {
    return pipeline_.filterAt(filterIndex).name();
}

bool FilterPanelModel::isEnabled(std::size_t filterIndex) const { return pipeline_.isEnabled(filterIndex); }

void FilterPanelModel::setEnabled(std::size_t filterIndex, bool enabled) {
    pipeline_.setEnabled(filterIndex, enabled);
}

const std::vector<FilterParameter>& FilterPanelModel::parameters(std::size_t filterIndex) const {
    return pipeline_.filterAt(filterIndex).parameters();
}

void FilterPanelModel::setParameter(std::size_t filterIndex, std::size_t parameterIndex, double value) {
    pipeline_.filterAt(filterIndex).parameters().at(parameterIndex).setValue(value);
}

void FilterPanelModel::moveUp(std::size_t filterIndex) {
    if (filterIndex > 0) pipeline_.move(filterIndex, filterIndex - 1);
}

void FilterPanelModel::moveDown(std::size_t filterIndex) {
    if (filterIndex + 1 < pipeline_.size()) pipeline_.move(filterIndex, filterIndex + 1);
}

}  // namespace coral