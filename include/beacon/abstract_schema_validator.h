// MIT License
//
// Copyright (c) 2025 José Henrique Noronha Oliveira e Silva
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <functional>
#include <variant>
#include <optional>
#include <vector>
#include <regex>
#include <memory>
#include <sstream>

namespace beacon::validation {
    /**
     * ValidationResult encapsulates sucess or error details from validation.
     */
    struct ValidationResult {
        bool success;
        std::string error_message;
        std::string path;

        // Default constructor: failure with empty error and path
        ValidationResult() : success(false), error_message(""), path("") {}

        // Constructor to initialize all fields
        ValidationResult(bool success_, std::string error_message_ = "", std::string path_ = "")
            : success(success_), error_message(std::move(error_message_)), path(std::move(path_)) {}

        // Factory method for success result
        static ValidationResult ok() {
            return ValidationResult(true);
        }

        // Factory method for failure result
        static ValidationResult fail(std::string msg, std::string path = "") {
            return ValidationResult(false, std::move(msg), std::move(path));
        }

        // Convert to bool: true if success, false if failure
        explicit operator bool() const {
            return success;
        }

        // Combine with path prefix for nested errors
        ValidationResult prepend_path(const std::string& prefix) const {
            if (success) return *this;
            std::string new_path = prefix;
            if (!new_path.empty() && !path.empty()) new_path += ".";
            new_path += path;
            return {false, error_message, new_path};
        }
    };

    /**
     * Abstract base interface for a field validator.
     * A validator receives a JSON value and returns a ValidationResult.
     */
    class ValidatableField {
    public:
        virtual ~ValidatableField() = default;

        virtual ValidationResult validate(const nlohmann::json& value) const = 0;
    };

    using ValidatorPtr = std::unique_ptr<ValidatableField>;

    /**
     * A lambda-based validator wrapper.
     * Allow easy creation of custom validators.
     */
    class LambdaValidator : public ValidatableField {
        public:
        using ValidatorFn = std::function<ValidationResult(const nlohmann::json&)>;

        explicit LambdaValidator(ValidatorFn fn) : validator(std::move(fn)) {}

        ValidationResult validate(const nlohmann::json& value) const override {
            return validator(value);
        }

        private:
        ValidatorFn validator;
    };

    /**
     * Combines multiple validators with logical AND.
     */
    class AndValidator : public ValidatableField {
        public:
        explicit AndValidator(std::vector<ValidatorPtr> validators)
            : validators_(std::move(validators)) {}

        ValidationResult validate(const nlohmann::json& value) const override {
            for (const auto& validator : validators_) {
                ValidationResult result = validator->validate(value);
                if (!result) return result;
            }
            return ValidationResult::ok();
        }

        private:
        std::vector<ValidatorPtr> validators_;
    };

    /**
     * Combines multiple validators with logical OR.
     */
    class OrValidator : public ValidatableField {
    public:
        explicit OrValidator(std::vector<ValidatorPtr> validators)
            :validators_(std::move(validators)) {}

        ValidationResult validate(const nlohmann::json &value) const override {
            std::vector<std::string> errors;

            for (const auto& validator : validators_) {
                ValidationResult result = validator->validate(value);
                if (result) return ValidationResult::ok();
                errors.push_back(result.error_message);
            }

            std::ostringstream oss;
            oss << "None matched. Errors: ";
            for (size_t i = 0; i < errors.size(); ++i) {
                oss << errors[i];
                if (i + 1 < errors.size()) oss << "; ";
            }

            return ValidationResult::fail(oss.str());
        }
    private:
        std::vector<ValidatorPtr> validators_;
    };

    /**
     * Validator to check if a JSON is an object matching a nested schema.
     * This allows recursive/nested validation.
     */
    class ObjectValidator : public ValidatableField {
        public:
        explicit ObjectValidator(std::unordered_map<std::string, ValidatorPtr> schema,
            std::vector<std::string> required_fields = {})
                : schema_(std::move(schema)), required_fields_(std::move(required_fields)) {}

        ValidationResult validate(const nlohmann::json &value) const override {
            if (!value.is_object()) {
                return ValidationResult::fail("Not an object");
            }

            for (const std::string& req : required_fields_) {
                if (!value.contains(req)) return ValidationResult::fail("Required field not found");
            }

            for (const auto& [field, validator] : schema_) {
                if (!value.contains(field)) continue; // optional field
                ValidationResult result = validator->validate(value.at(field));
                if (!result) return result.prepend_path(field);
            }
            return ValidationResult::ok();
        }
    private:
        std::unordered_map<std::string, ValidatorPtr> schema_;
        std::vector<std::string> required_fields_;
    };

    /**
     * Validator for arrays
     * Checks that all elements satisfy the element_validator.
     * Optionally can enforce min and max size.
     */
    class ArrayValidator : public ValidatableField {
    public:
        explicit ArrayValidator(ValidatorPtr element_validator,
                                std::optional<size_t> min_size = std::nullopt,
                                std::optional<size_t> max_size = std::nullopt)
            : element_validator_(std::move(element_validator)), min_size_(min_size), max_size_(max_size) {}

        ValidationResult validate(const nlohmann::json& value) const override {
            if (!value.is_array()) {
                return ValidationResult::fail("Not an array");
            }
            size_t size = value.size();
            if (min_size_ && size < *min_size_) {
                return ValidationResult::fail("Array size < " + std::to_string(*min_size_));
            }
            if (max_size_ && size > *max_size_) {
                return ValidationResult::fail("Array size > " + std::to_string(*max_size_));
            }
            for (size_t i = 0; i < size; ++i) {
                ValidationResult res = element_validator_->validate(value.at(i));
                if (!res) return res.prepend_path("[" + std::to_string(i) + "]");
            }
            return ValidationResult::ok();
        }

    private:
        ValidatorPtr element_validator_;
        std::optional<size_t> min_size_;
        std::optional<size_t> max_size_;
    };

    /**
     * Core schema class with fluent API.
     * Use field(...) and then chain validation methods.
     */
    class AbstractSchemaValidator {
class FieldBuilder {
    public:
        FieldBuilder(AbstractSchemaValidator& parent, std::string name)
            : parent_(parent), field_name_(std::move(name)) {}

        FieldBuilder& required() {
            required_ = true;
            return *this;
        }

        FieldBuilder& optional() {
            required_ = false;
            return *this;
        }

        FieldBuilder& validator(ValidatorPtr v) {
            validators_.push_back(std::move(v));
            return *this;
        }

        FieldBuilder& is_string() {
            return validator(make_is_string());
        }

        FieldBuilder& is_non_empty_string() {
            return validator(make_non_empty_string());
        }

        FieldBuilder& is_integer() {
            return validator(make_is_integer());
        }

        FieldBuilder& is_boolean() {
            return validator(make_is_boolean());
        }

        FieldBuilder& is_array() {
            return validator(make_is_array());
        }

        FieldBuilder& is_object() {
            return validator(make_is_object());
        }

        FieldBuilder& min_length(size_t min_len) {
            return validator(make_min_length(min_len));
        }

        FieldBuilder& max_length(size_t max_len) {
            return validator(make_max_length(max_len));
        }

        FieldBuilder& matches_regex(const std::string& pattern) {
            return validator(make_regex(pattern));
        }

        FieldBuilder& min_integer(int min_val) {
            return validator(make_min_integer(min_val));
        }

        FieldBuilder& max_integer(int max_val) {
            return validator(make_max_integer(max_val));
        }

        FieldBuilder& or_validator(std::vector<ValidatorPtr> validators) {
            return validator(std::make_unique<OrValidator>(std::move(validators)));
        }

        FieldBuilder& and_validator(std::vector<ValidatorPtr> validators) {
            return validator(std::make_unique<AndValidator>(std::move(validators)));
        }

        FieldBuilder& nested_object(std::unordered_map<std::string, ValidatorPtr> schema,
                                    std::vector<std::string> required_fields = {}) {
            return validator(std::make_unique<ObjectValidator>(std::move(schema), std::move(required_fields)));
        }

        FieldBuilder& array_of(ValidatorPtr element_validator,
                               std::optional<size_t> min_size = std::nullopt,
                               std::optional<size_t> max_size = std::nullopt) {
            return validator(std::make_unique<ArrayValidator>(std::move(element_validator), min_size, max_size));
        }

        /**
         * Finalize the field and add it to the parent schema.
         */
        AbstractSchemaValidator& done() {
            if (validators_.empty()) {
                // no validators, accept anything (always valid)
                parent_.add_field_schema(
                    field_name_,
                    std::make_unique<LambdaValidator>(
                        [](const nlohmann::json&) { return ValidationResult::ok(); }),
                    required_ ? FieldRequirement::Required : FieldRequirement::Optional);
            } else if (validators_.size() == 1) {
                parent_.add_field_schema(field_name_, std::move(validators_[0]),
                                        required_ ? FieldRequirement::Required : FieldRequirement::Optional);
            } else {
                parent_.add_field_schema(
                    field_name_,
                    std::make_unique<AndValidator>(std::move(validators_)),
                    required_ ? FieldRequirement::Required : FieldRequirement::Optional);
            }
            return parent_;
        }

    private:
        AbstractSchemaValidator& parent_;
        std::string field_name_;
        bool required_ = true; // default required
        std::vector<ValidatorPtr> validators_;

        // Helpers to build common validators for chaining

        static ValidatorPtr make_is_string() {
            return std::make_unique<LambdaValidator>(
                [](const nlohmann::json& val) -> ValidationResult {
                    if (!val.is_string())
                        return ValidationResult::fail("Not a string");
                    return ValidationResult::ok();
                });
        }

        static ValidatorPtr make_non_empty_string() {
            return std::make_unique<LambdaValidator>(
                [](const nlohmann::json& val) -> ValidationResult {
                    if (!val.is_string())
                        return ValidationResult::fail("Not a string");
                    if (val.get<std::string>().empty())
                        return ValidationResult::fail("Empty string");
                    return ValidationResult::ok();
                });
        }

        static ValidatorPtr make_is_integer() {
            return std::make_unique<LambdaValidator>(
                [](const nlohmann::json& val) -> ValidationResult {
                    if (!val.is_number_integer())
                        return ValidationResult::fail("Not an integer");
                    return ValidationResult::ok();
                });
        }

        static ValidatorPtr make_is_boolean() {
            return std::make_unique<LambdaValidator>(
                [](const nlohmann::json& val) -> ValidationResult {
                    if (!val.is_boolean())
                        return ValidationResult::fail("Not a boolean");
                    return ValidationResult::ok();
                });
        }

        static ValidatorPtr make_is_array() {
            return std::make_unique<LambdaValidator>(
                [](const nlohmann::json& val) -> ValidationResult {
                    if (!val.is_array())
                        return ValidationResult::fail("Not an array");
                    return ValidationResult::ok();
                });
        }

        static ValidatorPtr make_is_object() {
            return std::make_unique<LambdaValidator>(
                [](const nlohmann::json& val) -> ValidationResult {
                    if (!val.is_object())
                        return ValidationResult::fail("Not an object");
                    return ValidationResult::ok();
                });
        }

        static ValidatorPtr make_min_length(size_t min_len) {
            return std::make_unique<LambdaValidator>(
                [min_len](const nlohmann::json& val) -> ValidationResult {
                    if (val.is_string()) {
                        if (val.get<std::string>().size() < min_len)
                            return ValidationResult::fail("String length < " + std::to_string(min_len));
                    } else if (val.is_array()) {
                        if (val.size() < min_len)
                            return ValidationResult::fail("Array size < " + std::to_string(min_len));
                    } else {
                        return ValidationResult::fail("Value has no length");
                    }
                    return ValidationResult::ok();
                });
        }

        static ValidatorPtr make_max_length(size_t max_len) {
            return std::make_unique<LambdaValidator>(
                [max_len](const nlohmann::json& val) -> ValidationResult {
                    if (val.is_string()) {
                        if (val.get<std::string>().size() > max_len)
                            return ValidationResult::fail("String length > " + std::to_string(max_len));
                    } else if (val.is_array()) {
                        if (val.size() > max_len)
                            return ValidationResult::fail("Array size > " + std::to_string(max_len));
                    } else {
                        return ValidationResult::fail("Value has no length");
                    }
                    return ValidationResult::ok();
                });
        }

        static ValidatorPtr make_regex(const std::string& pattern) {
            return std::make_unique<LambdaValidator>(
                [pattern](const nlohmann::json& val) -> ValidationResult {
                    if (!val.is_string())
                        return ValidationResult::fail("Not a string for regex");
                    try {
                        std::regex re(pattern);
                        if (!std::regex_match(val.get<std::string>(), re))
                            return ValidationResult::fail("Does not match regex: " + pattern);
                    } catch (const std::regex_error& e) {
                        return ValidationResult::fail(std::string("Invalid regex: ") + e.what());
                    }
                    return ValidationResult::ok();
                });
        }

        static ValidatorPtr make_min_integer(int min_val) {
            return std::make_unique<LambdaValidator>(
                [min_val](const nlohmann::json& val) -> ValidationResult {
                    if (!val.is_number_integer())
                        return ValidationResult::fail("Not an integer");
                    if (val.get<int>() < min_val)
                        return ValidationResult::fail("Integer < " + std::to_string(min_val));
                    return ValidationResult::ok();
                });
        }

        static ValidatorPtr make_max_integer(int max_val) {
            return std::make_unique<LambdaValidator>(
                [max_val](const nlohmann::json& val) -> ValidationResult {
                    if (!val.is_number_integer())
                        return ValidationResult::fail("Not an integer");
                    if (val.get<int>() > max_val)
                        return ValidationResult::fail("Integer > " + std::to_string(max_val));
                    return ValidationResult::ok();
                });
        }
    };

    AbstractSchemaValidator() = default;

    /**
     * Begin defining a new field in the schema.
     */
    FieldBuilder field(const std::string& name) {
        return FieldBuilder(*this, name);
    }

    /**
     * Validate a JSON object against the defined schema.
     */
    ValidationResult validate(const nlohmann::json& json) const {
        if (!json.is_object()) {
            return ValidationResult::fail("Root is not an object");
        }

        for (const auto& [field_name, entry] : schema_) {
            if (json.contains(field_name)) {
                ValidationResult res = entry.validator->validate(json.at(field_name));
                if (!res) return res.prepend_path(field_name);
            } else {
                if (entry.requirement == FieldRequirement::Required) {
                    return ValidationResult::fail("Missing required field '" + field_name + "'");
                }
            }
        }
        return ValidationResult::ok();
    }

private:
    enum class FieldRequirement {
        Required,
        Optional
    };

    struct FieldSchemaEntry {
        ValidatorPtr validator;
        FieldRequirement requirement;
    };

    std::unordered_map<std::string, FieldSchemaEntry> schema_;

    void add_field_schema(std::string name, ValidatorPtr validator, FieldRequirement requirement) {
        schema_.emplace(std::move(name), FieldSchemaEntry{std::move(validator), requirement});
    }
    };
}
