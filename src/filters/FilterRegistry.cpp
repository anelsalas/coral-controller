#include "filters/FilterRegistry.h"

#include <algorithm>
#include <stdexcept>

namespace coral {

void FilterRegistry::addFactory(std::string name, FilterFactory factory) {
    const bool duplicate =
        std::any_of(entries_.begin(), entries_.end(), [&](const Entry& entry) { return entry.name == name; });
    if (duplicate) throw std::logic_error("filter registered twice: " + name);
    entries_.push_back(Entry{std::move(name), std::move(factory)});
}

std::vector<std::string> FilterRegistry::names() const {
    std::vector<std::string> result;
    for (const auto& entry : entries_) result.push_back(entry.name);
    return result;
}

std::vector<std::unique_ptr<IFilter>> FilterRegistry::createAll() const {
    std::vector<std::unique_ptr<IFilter>> filters;
    for (const auto& entry : entries_) filters.push_back(entry.factory());
    return filters;
}

}  // namespace coral