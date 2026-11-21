#ifndef GREEDYDAILY_HPP
#define GREEDYDAILY_HPP

#include <vector>

using Matrix = std::vector<std::vector<double>>;

class greedy
{
private:
    double capacity;
    Matrix distance;
    std::vector<double> weight;
    std::vector<bool> visited;

    double total_distance = 0;
    int nb_tour = 0;
    int actual_position = 0;
    double situation_truck = 0;

public:
    greedy(double capacity, Matrix distance, std::vector<double> weight);

    double compute();
    int getNumberOfTours() const;
};

#endif
