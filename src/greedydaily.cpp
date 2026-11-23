#include <limits>
#include <stdexcept>
#include <utility>

#include "greedydaily.hpp"

greedy::greedy(double capacity, Matrix distance, std::vector<double> weight)
    : capacity(capacity),
      distance(std::move(distance)),
      weight(std::move(weight)),
      visited(this->distance.size(), false)
{
}

double greedy::compute()
{
    actual_position = 0;
    situation_truck = 0;
    total_distance = 0;
    nb_tour = 1;
    visited.assign(distance.size(), false);
    visited[0] = true;

    int visited_count = 0;

    while (visited_count < static_cast<int>(distance.size()) - 1)
    {
        double minimum = std::numeric_limits<double>::infinity();
        int prochain = -1;

        // Search from the current position in the matrix
        for (int j = 0; j < static_cast<int>(distance.size()); ++j)
        {
            if (!visited[j] && j != 0 &&
                distance[actual_position][j] < minimum &&
                situation_truck + weight[j] <= capacity)
            {
                minimum = distance[actual_position][j];
                prochain = j;
            }
        }

        if (prochain == -1)
        {
            if (actual_position != 0)
            {
                total_distance += distance[actual_position][0];
                actual_position = 0;
                situation_truck = 0;
                nb_tour++;
            }
            else
            {
                throw std::runtime_error(
                    "Unable to visit all points with this capacity");
            }
        }
        else
        {
            total_distance += minimum;

            situation_truck += weight[prochain];
            visited[prochain] = true;
            visited_count++;
            actual_position = prochain;
        }
    }

    // Return to the depot after the final tour
    if (actual_position != 0)
        total_distance += distance[actual_position][0];

    return total_distance;
}

int greedy::getNumberOfTours() const
{
    return nb_tour;
}