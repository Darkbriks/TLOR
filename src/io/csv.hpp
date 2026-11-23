#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tlor::io {

std::string shown(const std::filesystem::path& path);

std::string read_text(const std::filesystem::path& file);

std::optional<double> parse_double(std::string_view text);

std::optional<std::int64_t> parse_integer(std::string_view text);

class CsvTable {
public:
    static CsvTable read(const std::filesystem::path& file);

    [[nodiscard]] std::size_t column(std::string_view name) const;
    [[nodiscard]] std::size_t row_count() const noexcept;
    [[nodiscard]] std::string_view cell(std::size_t row, std::size_t column) const;
    [[nodiscard]] double number(std::size_t row, std::size_t column) const;
    [[nodiscard]] double number_or(std::size_t row, std::size_t column, double empty) const;
    [[nodiscard]] std::int64_t integer(std::size_t row, std::size_t column) const;
    [[nodiscard]] bool boolean(std::size_t row, std::size_t column) const;
    [[nodiscard]] std::string where(std::size_t row, std::size_t column) const;
    [[nodiscard]] std::string where(std::size_t row) const;

private:
    std::string file_;
    std::vector<std::string> header_;
    std::vector<std::vector<std::string>> rows_;
    std::vector<std::size_t> lines_;
};

}
