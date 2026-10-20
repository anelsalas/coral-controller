#include "filters/FilterPipeline.h"

#include <exception>
#include <stdexcept>

namespace coral {

void FilterPipeline::append(std::unique_ptr<IFilter> filter, bool enabled) {
    if (!filter) throw std::invalid_argument("FilterPipeline::append: null filter");
    stages_.push_back(Stage{std::move(filter), enabled});
}

std::size_t FilterPipeline::size() const { return stages_.size(); }
IFilter& FilterPipeline::filterAt(std::size_t index) { return *stages_.at(index).filter; }
const IFilter& FilterPipeline::filterAt(std::size_t index) const { return *stages_.at(index).filter; }
bool FilterPipeline::isEnabled(std::size_t index) const { return stages_.at(index).enabled; }
void FilterPipeline::setEnabled(std::size_t index, bool enabled) { stages_.at(index).enabled = enabled; }

void FilterPipeline::move(std::size_t from, std::size_t to) {
    if (from >= stages_.size() || to >= stages_.size()) throw std::out_of_range("FilterPipeline::move");
    Stage stage = std::move(stages_[from]);
    stages_.erase(stages_.begin() + static_cast<std::ptrdiff_t>(from));
    stages_.insert(stages_.begin() + static_cast<std::ptrdiff_t>(to), std::move(stage));
}

PipelineOutput FilterPipeline::apply(const cv::Mat& input) const {
    cv::Mat current = input;
    for (const auto& stage : stages_) {
        if (!stage.enabled) continue;
        const std::string name(stage.filter->name());
        try {
            current = stage.filter->apply(current);
        } catch (const std::exception& error) {
            return PipelineOutput{input, PipelineError{name, error.what()}};
        }
        if (current.empty()) return PipelineOutput{input, PipelineError{name, "filter produced an empty image"}};
    }
    return PipelineOutput{current, std::nullopt};
}

}  // namespace coral