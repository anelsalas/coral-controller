#pragma once
#include "filters/IFilter.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace coral {

struct PipelineError {
    std::string filterName;
    std::string message;
};

struct PipelineOutput {
    cv::Mat image;
    std::optional<PipelineError> error;  // set => image is the unfiltered input
};

// Ordered list of filters, each enabled or disabled (VIEW-006). Used from the UI thread only.
class FilterPipeline {
public:
    void append(std::unique_ptr<IFilter> filter, bool enabled);
    std::size_t size() const;
    IFilter& filterAt(std::size_t index);
    const IFilter& filterAt(std::size_t index) const;
    bool isEnabled(std::size_t index) const;
    void setEnabled(std::size_t index, bool enabled);
    void move(std::size_t from, std::size_t to);

    PipelineOutput apply(const cv::Mat& input) const;

private:
    struct Stage {
        std::unique_ptr<IFilter> filter;
        bool enabled;
    };
    std::vector<Stage> stages_;
};

}  // namespace coral