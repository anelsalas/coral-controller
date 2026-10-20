#pragma once
#include <string>
#include <vector>

namespace coral {

enum class ParameterKind { Integer, Real, Boolean, Choice };

// A named, range-checked filter setting. The UI generates its control from kind/range/options.
class FilterParameter {
public:
    static FilterParameter integer(std::string name, int initial, int minimum, int maximum);
    static FilterParameter real(std::string name, double initial, double minimum, double maximum);
    static FilterParameter boolean(std::string name, bool initial);
    static FilterParameter choice(std::string name, std::vector<std::string> options, int initialIndex);

    const std::string& name() const;
    ParameterKind kind() const;
    double minimum() const;
    double maximum() const;
    const std::vector<std::string>& options() const;

    void setValue(double value);  // clamped to [minimum, maximum], rounded for integer kinds
    double value() const;
    int asInt() const;
    double asReal() const;
    bool asBool() const;
    int asChoiceIndex() const;

private:
    FilterParameter(std::string name, ParameterKind kind, double minimum, double maximum,
                    std::vector<std::string> options, double initial);
    double normalized(double value) const;

    std::string name_;
    ParameterKind kind_;
    double minimum_;
    double maximum_;
    std::vector<std::string> options_;
    double value_;
};

}  // namespace coral