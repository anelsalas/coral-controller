#pragma once
#include <string>
#include <utility>
#include <variant>

namespace coral {

// Either a value or an error message. Used instead of exceptions for expected failures
// (missing device, bad input, I/O errors) so callers must handle them explicitly.
template <typename T>
class [[nodiscard]] Result {
public:
    static Result success(T value) { return Result(std::move(value)); }
    static Result failure(std::string message) { return Result(Failure{std::move(message)}); }

    bool ok() const { return std::holds_alternative<T>(state_); }
    const T& value() const { return std::get<T>(state_); }
    T& value() { return std::get<T>(state_); }
    const std::string& error() const { return std::get<Failure>(state_).message; }

private:
    struct Failure {
        std::string message;
    };
    explicit Result(T value) : state_(std::move(value)) {}
    explicit Result(Failure failure) : state_(std::move(failure)) {}

    std::variant<T, Failure> state_;
};

using Status = Result<std::monostate>;
inline Status okStatus() { return Status::success(std::monostate{}); }

}  // namespace coral