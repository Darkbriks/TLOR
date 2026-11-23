import numpy as np
import pytest

import tlor


def small_arrays():
    x = np.array([1, 2, 3, 4, 5, 0])
    diff = x[None, :] - x[:, None]
    distance = 100.0 * np.abs(diff) + np.where(x[None, :] < x[:, None], 50.0, 0.0)
    np.fill_diagonal(distance, 0.0)
    demand = np.tile(np.array([30.0, 10.0, 20.0, 50.0, 0.0, 0.0]), (7, 1))
    return {
        "distance_m": distance,
        "duration_s": distance / 10.0,
        "service_s": np.array([60.0, 60, 60, 60, 60, 120]),
        "capacity_kg": np.array([100.0, 100, 100, 100, 100, 0]),
        "active": np.array([1, 1, 1, 1, 0, 0], dtype=bool),
        "demand_kg": demand,
        "depot": 5,
    }


@pytest.fixture
def inst():
    return tlor.Instance(**small_arrays())


@pytest.fixture
def fleet():
    available = np.ones((7, 2), dtype=bool)
    available[5, :] = False
    return tlor.Fleet([150.0, 150.0], [3600.0, 3600.0], 7, available)


def week_solution():
    plan = tlor.Plan(0, 7)
    plan.add(2, 0)
    solution = tlor.Solution(plan)
    routes = tlor.DayRoutes(2)
    routes.add(0, [5, 0, 5])
    solution.set_routes(routes)
    return solution
