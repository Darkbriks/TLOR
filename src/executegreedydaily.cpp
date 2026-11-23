#include "executegreedydaily.hpp"

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>

#include "greedydaily.hpp"

namespace
{
std::string trim(std::string value)
{
    while (!value.empty() &&
           std::isspace(static_cast<unsigned char>(value.front())))
    {
        value.erase(value.begin());
    }

    while (!value.empty() &&
           std::isspace(static_cast<unsigned char>(value.back())))
    {
        value.pop_back();
    }

    return value;
}

std::vector<std::string> splitCsvLine(const std::string& line)
{
    std::vector<std::string> columns;
    std::stringstream stream(line);
    std::string column;

    while (std::getline(stream, column, ';'))
    {
        columns.push_back(trim(column));
    }

    if (!line.empty() && line.back() == ';')
    {
        columns.emplace_back();
    }

    return columns;
}

std::unordered_map<std::string, std::size_t> readHeader(
    const std::string& headerLine)
{
    std::unordered_map<std::string, std::size_t> header;
    const std::vector<std::string> columns = splitCsvLine(headerLine);

    for (std::size_t i = 0; i < columns.size(); ++i)
    {
        header[columns[i]] = i;
    }

    return header;
}

std::size_t skipWhitespace(const std::string& text, std::size_t position)
{
    while (position < text.size() &&
           std::isspace(static_cast<unsigned char>(text[position])))
    {
        ++position;
    }
    return position;
}

double parseJsonNumber(const std::string& text, std::size_t& position)
{
    position = skipWhitespace(text, position);
    char* end = nullptr;
    const double value = std::strtod(text.c_str() + position, &end);

    if (end == text.c_str() + position)
    {
        throw std::runtime_error("Invalid number in distances.json");
    }

    position = static_cast<std::size_t>(end - text.c_str());
    return value;
}

Matrix parseDistancesJson(const std::string& text)
{
    Matrix distance;
    std::size_t position = skipWhitespace(text, 0);

    if (position >= text.size() || text[position] != '[')
    {
        throw std::runtime_error("Invalid distances JSON");
    }
    ++position;

    while (true)
    {
        position = skipWhitespace(text, position);
        if (position < text.size() && text[position] == ']')
        {
            ++position;
            break;
        }

        if (position >= text.size() || text[position] != '[')
        {
            throw std::runtime_error("A matrix row is invalid");
        }
        ++position;

        std::vector<double> row;
        while (true)
        {
            row.push_back(parseJsonNumber(text, position));
            position = skipWhitespace(text, position);

            if (position < text.size() && text[position] == ',')
            {
                ++position;
                continue;
            }
            if (position < text.size() && text[position] == ']')
            {
                ++position;
                break;
            }
            throw std::runtime_error("Invalid separator in distances.json");
        }

        distance.push_back(std::move(row));
        position = skipWhitespace(text, position);

        if (position < text.size() && text[position] == ',')
        {
            ++position;
            continue;
        }
        if (position < text.size() && text[position] == ']')
        {
            ++position;
            break;
        }
        throw std::runtime_error("Invalid end of distances.json");
    }

    return distance;
}

std::filesystem::path findProjectRoot()
{
    std::filesystem::path directory = std::filesystem::current_path();

    for (std::size_t depth = 0; depth < 8; ++depth)
    {
        const std::filesystem::path csv =
            directory / "data" / "tables" / "active_points.csv";
        const std::filesystem::path json =
            directory / "Maps" / "offline map" / "data" / "distances.json";

        if (std::filesystem::exists(csv) && std::filesystem::exists(json))
        {
            return directory;
        }

        if (!directory.has_parent_path())
        {
            break;
        }
        directory = directory.parent_path();
    }

    throw std::runtime_error(
        "Unable to find active_points.csv and distances.json");
}

const std::string& column(
    const std::vector<std::string>& columns,
    const std::unordered_map<std::string, std::size_t>& header,
    const std::string& name)
{
    const auto found = header.find(name);
    if (found == header.end())
    {
        throw std::runtime_error("Missing CSV column: " + name);
    }
    if (found->second >= columns.size())
    {
        throw std::runtime_error("Missing CSV value for: " + name);
    }
    return columns[found->second];
}
}

void executeGreedyDaily()
{
    try
    {
        const std::filesystem::path projectRoot = findProjectRoot();
        const std::filesystem::path activePointsPath =
            projectRoot / "data" / "tables" / "active_points.csv";
        const std::filesystem::path distancesPath =
            projectRoot / "Maps" / "offline map" / "data" / "distances.json";

        std::ifstream activePointsFile(activePointsPath);
        if (!activePointsFile)
        {
            throw std::runtime_error("Unable to open active_points.csv");
        }

        std::string headerLine;
        if (!std::getline(activePointsFile, headerLine))
        {
            throw std::runtime_error("active_points.csv is empty");
        }
        const std::unordered_map<std::string, std::size_t> header =
            readHeader(headerLine);

        std::vector<int> originalIds;
        std::vector<double> weights;
        originalIds.push_back(328);
        weights.push_back(0.0);

        std::string line;
        while (std::getline(activePointsFile, line))
        {
            if (trim(line).empty())
            {
                continue;
            }

            const std::vector<std::string> columns = splitCsvLine(line);
            const int pointId =
                std::stoi(column(columns, header, "point_id"));

            if (pointId == 328)
            {
                continue;
            }

            const std::string& active =
                column(columns, header, "active");
            if (active != "True")
            {
                continue;
            }

            const std::string& capacityKg =
                column(columns, header, "capacity_kg");
            if (capacityKg.empty())
            {
                throw std::runtime_error(
                    "Missing capacity_kg for point " +
                    std::to_string(pointId));
            }

            originalIds.push_back(pointId);
            weights.push_back(std::stod(capacityKg));
        }

        std::ifstream distancesFile(distancesPath);
        if (!distancesFile)
        {
            throw std::runtime_error("Unable to open distances.json");
        }
        std::stringstream distancesText;
        distancesText << distancesFile.rdbuf();
        const Matrix globalDistance =
            parseDistancesJson(distancesText.str());

        Matrix distance(originalIds.size(),
                         std::vector<double>(originalIds.size(), 0.0));

        for (std::size_t localFrom = 0;
             localFrom < originalIds.size();
             ++localFrom)
        {
            const int originalFrom = originalIds[localFrom];
            if (originalFrom < 0 ||
                originalFrom >= static_cast<int>(globalDistance.size()))
            {
                throw std::runtime_error(
                    "Point index outside the matrix: " +
                    std::to_string(originalFrom));
            }

            for (std::size_t localTo = 0;
                 localTo < originalIds.size();
                 ++localTo)
            {
                const int originalTo = originalIds[localTo];
                if (originalTo < 0 ||
                    originalTo >=
                        static_cast<int>(globalDistance[originalFrom].size()))
                {
                    throw std::runtime_error(
                        "Point index outside the matrix: " +
                        std::to_string(originalTo));
                }

                distance[localFrom][localTo] =
                    globalDistance[originalFrom][originalTo];
            }
        }

        const double capacity = 10000.0;
        greedy algorithm(capacity, distance, weights);
        double totalDistance = algorithm.compute();

        std::cout << "Active points: " << originalIds.size() - 1 << '\n';
        std::cout << "Depot: local index 0, global identifier 328\n";
        std::cout << "Number of tours: " << algorithm.getNumberOfTours()
                  << '\n';
        std::cout << "Total distance: " << totalDistance / 1000.0
                  << " km\n";
    }
    catch (const std::runtime_error& error)
    {
        std::cerr << "Error: " << error.what() << '\n';
    }
}
