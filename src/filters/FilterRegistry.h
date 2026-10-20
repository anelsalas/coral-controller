#pragma once
#include "filters/IFilter.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace coral {

using FilterFactory = std::function<std::unique_ptr<IFilter>()>;

// Ordered catalog of available filters (VIEW-009). Register with one line: registry.add<MyFilter>();
class FilterRegistry {
public:
    template <typename FilterType>
    void add() {
        addFactory(std::string(FilterType::kName), [] { return std::make_unique<FilterType>(); });
    }

    void addFactory(std::string name, FilterFactory factory);
    std::vector<std::string> names() const;
    std::vector<std::unique_ptr<IFilter>> createAll() const;

private:
    struct Entry {
        std::string name;
        FilterFactory factory;
    };
    std::vector<Entry> entries_;
};

}  // namespace coral