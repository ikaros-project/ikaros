// Ikaros 3.0

#pragma once

#include <array>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "dictionary.h"
#include "matrix.h"

namespace ikaros
{
    class Component;
    class Kernel;

    enum parameter_type
    {
        no_type = 0,
        number_type,
        rate_type,
        bool_type,
        string_type,
        matrix_type,
    };

    inline constexpr std::array<std::string_view, 6> parameter_strings = {
        "none",
        "number",
        "rate",
        "bool",
        "string",
        "matrix",
    };

    class parameter
    {
    private:
        using parameter_value = std::variant<std::monostate, double, bool, int, std::string, matrix>;

        struct parameter_state
        {
            dictionary info;
            bool has_options = false;
            std::vector<std::string> options;
            bool resolved = false;
            parameter_type type = no_type;
            parameter_value value;
            bool dynamic = false;
            std::optional<double> minimum;
            std::optional<double> maximum;
        };

        std::shared_ptr<parameter_state> state_;

        std::shared_ptr<parameter_state> clone_state() const;
        void bind_to(const parameter & p);
        void validate_numeric_value(double value) const;
        double numeric_value(const std::string & conversion_name) const;
        matrix * matrix_value() noexcept;
        const matrix * matrix_value() const noexcept;
        matrix & matrix_ref();
        const matrix & matrix_ref() const;
        void set_source_value(const std::string & value);

        friend class Component;
        friend class Kernel;

    public:
        parameter();
        parameter(dictionary info);
        parameter(const std::string type, const std::string options = "");
        parameter(const parameter & p);
        parameter(parameter && p) noexcept = default;

        parameter & operator=(const parameter & p);
        parameter & operator=(parameter && p) noexcept = default;
        double operator=(double v);
        std::string operator=(std::string v);
        void set_matrix(const matrix & v);

        operator const matrix & () const = delete;
        operator std::string() const;
        operator double() const;
        explicit operator bool() const;

        void print(std::string name = "") const;
        void info() const;

        bool as_bool() const;
        float as_float() const;
        double as_double() const;
        long as_long() const;
        int as_int() const;
        matrix as_matrix() const;
        std::string as_int_string() const;
        std::string as_string() const;
        bool empty() const;
        int size() const;
        float get(int index, float default_value) const;
        float operator[](int index) const;

        parameter_type get_type() const noexcept;
        bool has_options() const noexcept;
        bool is_resolved() const noexcept;
        std::vector<std::string> options() const;
        std::string unit() const;
        dictionary metadata() const;

        std::string json() const;

        friend std::ostream & operator<<(std::ostream & os, const parameter & p);
        bool compare_string(const std::string & value) const;
    };
}
