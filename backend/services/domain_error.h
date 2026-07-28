#pragma once

#include <stdexcept>
#include <string>

enum class DomainErrorKind { Validation, NotFound, Conflict, Forbidden, ProviderUnavailable };

class DomainError : public std::runtime_error
{
public:
    DomainError(DomainErrorKind kind, std::string code, std::string message)
        : std::runtime_error(std::move(message)), kind_(kind), code_(std::move(code)) {}

    DomainErrorKind kind() const noexcept { return kind_; }
    const std::string &code() const noexcept { return code_; }

private:
    DomainErrorKind kind_;
    std::string code_;
};
