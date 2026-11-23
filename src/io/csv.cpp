#include "io/csv.hpp"

#include <tlor/error.hpp>

#include <charconv>
#include <fstream>
#include <iterator>
#include <system_error>

#if !(defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201611L)
#include <locale>
#include <sstream>
#endif

namespace tlor::io {

namespace {

[[noreturn]] void invalid(const std::string& message) { throw Error(Status::invalid_argument, message); }

std::string_view trim(std::string_view text) {
    const auto first = text.find_first_not_of(" \t\r");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = text.find_last_not_of(" \t\r");
    return text.substr(first, last - first + 1);
}

std::vector<std::string> split(std::string_view line) {
    std::vector<std::string> fields;
    std::size_t start = 0;
    while (true) {
        const auto end = line.find(';', start);
        fields.emplace_back(trim(line.substr(start, end == std::string_view::npos ? end : end - start)));
        if (end == std::string_view::npos) {
            return fields;
        }
        start = end + 1;
    }
}

}

std::string shown(const std::filesystem::path& path) {
    const std::u8string text = path.u8string();
    return {reinterpret_cast<const char*>(text.data()), text.size()};
}

std::string read_text(const std::filesystem::path& file) {
    std::error_code error;
    if (!std::filesystem::is_regular_file(file, error)) {
        throw Error(Status::io_error, "cannot open " + shown(file) + ": no such file");
    }
    std::ifstream in(file, std::ios::binary);
    if (!in) {
        throw Error(Status::io_error, "cannot open " + shown(file));
    }
    std::string text{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    if (in.bad()) {
        throw Error(Status::io_error, "cannot read " + shown(file));
    }
    return text;
}

std::optional<double> parse_double(std::string_view text) {
    if (text.empty()) {
        return std::nullopt;
    }
    double value = 0;
#if defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201611L
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size()) {
        return std::nullopt;
    }
#else
    std::istringstream in{std::string(text)};
    in.imbue(std::locale::classic());
    in >> value;
    if (in.fail() || in.peek() != std::char_traits<char>::eof()) {
        return std::nullopt;
    }
#endif
    return value;
}

std::optional<std::int64_t> parse_integer(std::string_view text) {
    std::int64_t value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (text.empty() || error != std::errc{} || end != text.data() + text.size()) {
        return std::nullopt;
    }
    return value;
}

CsvTable CsvTable::read(const std::filesystem::path& file) {
    CsvTable table;
    table.file_ = shown(file);
    const std::string text = read_text(file);
    std::string_view rest(text);
    if (rest.starts_with("\xEF\xBB\xBF")) {
        rest.remove_prefix(3);
    }
    bool header = true;
    for (std::size_t line = 1; !rest.empty(); ++line) {
        const auto end = rest.find('\n');
        const std::string_view current = rest.substr(0, end);
        rest = end == std::string_view::npos ? std::string_view{} : rest.substr(end + 1);
        if (trim(current).empty()) {
            continue;
        }
        auto fields = split(current);
        if (header) {
            table.header_ = std::move(fields);
            header = false;
            continue;
        }
        if (fields.size() != table.header_.size()) {
            invalid(table.file_ + " line " + std::to_string(line) + " has " + std::to_string(fields.size()) +
                    " fields, expected " + std::to_string(table.header_.size()));
        }
        table.rows_.push_back(std::move(fields));
        table.lines_.push_back(line);
    }
    if (header) {
        invalid(table.file_ + " is empty");
    }
    return table;
}

std::size_t CsvTable::column(std::string_view name) const {
    for (std::size_t i = 0; i < header_.size(); ++i) {
        if (header_[i] == name) {
            return i;
        }
    }
    invalid(file_ + " has no column '" + std::string(name) + "'");
}

std::size_t CsvTable::row_count() const noexcept { return rows_.size(); }

std::string_view CsvTable::cell(std::size_t row, std::size_t column) const { return rows_.at(row).at(column); }

double CsvTable::number(std::size_t row, std::size_t column) const {
    const auto value = parse_double(cell(row, column));
    if (!value) {
        invalid(where(row, column) + ": '" + std::string(cell(row, column)) + "' is not a number");
    }
    return *value;
}

double CsvTable::number_or(std::size_t row, std::size_t column, double empty) const {
    return cell(row, column).empty() ? empty : number(row, column);
}

std::int64_t CsvTable::integer(std::size_t row, std::size_t column) const {
    const auto value = parse_integer(cell(row, column));
    if (!value) {
        invalid(where(row, column) + ": '" + std::string(cell(row, column)) + "' is not an integer");
    }
    return *value;
}

bool CsvTable::boolean(std::size_t row, std::size_t column) const {
    const auto text = cell(row, column);
    if (text == "True" || text == "true") {
        return true;
    }
    if (text == "False" || text == "false") {
        return false;
    }
    invalid(where(row, column) + ": '" + std::string(text) + "' is not True or False");
}

std::string CsvTable::where(std::size_t row, std::size_t column) const {
    return where(row) + " column " + header_.at(column);
}

std::string CsvTable::where(std::size_t row) const { return file_ + " line " + std::to_string(lines_.at(row)); }

}
