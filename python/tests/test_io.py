import datetime
import json
from pathlib import Path

import pytest

import tlor
from conftest import small_arrays, week_solution

TINY = Path(__file__).resolve().parents[2] / "tests" / "fixtures" / "tiny"


def test_load_and_write(tmp_path):
    inst = tlor.Instance.load(TINY, TINY, service_time=TINY / "service_time.csv")
    assert inst.first_date == datetime.date(2023, 4, 14)
    assert inst.node_count == 5
    fleet = tlor.Fleet([1000], [28800], 3)
    result = tlor.run_rolling(inst, fleet, first=0, last=2, horizon=2, step=1, schedule="threshold",
                              route="nearest_neighbour")
    tlor.write_solution(tmp_path, inst, result.solution)
    assert (tmp_path / "routes_2023-04-16.json").exists()
    assert (tmp_path / "collection_schedule.csv").read_text().startswith("date;point_id;collected_kg;fill_rate\n")


def test_load_options_and_errors(tmp_path):
    inst = tlor.Instance.load(str(TINY), str(TINY), service_time=str(TINY / "service_time.csv"), point_policy=False)
    assert inst.first_date == datetime.date(2023, 4, 14)
    with pytest.raises(tlor.IOFailure, match="active_points.csv"):
        tlor.Instance.load(tmp_path, TINY, service_time=TINY / "service_time.csv")
    with pytest.raises(tlor.IOFailure, match="exemple_points_service_time.csv"):
        tlor.Instance.load(TINY, TINY)


def test_write_solution_needs_a_date(tmp_path):
    inst = tlor.Instance(**small_arrays())
    solution = week_solution()
    with pytest.raises(tlor.InvalidArgument, match="first_date"):
        tlor.write_solution(tmp_path, inst, solution)
    tlor.write_solution(tmp_path / "a", inst, solution, first_date=datetime.date(2023, 4, 14))
    tlor.write_solution(tmp_path / "b", inst, solution, first_date="2023-04-14")
    assert (tmp_path / "a" / "routes_2023-04-16.json").read_text() == (tmp_path / "b" / "routes_2023-04-16.json").read_text()


def test_write_routes_json(tmp_path):
    routes = tlor.DayRoutes(1)
    routes.add(0, [5, 0, 1, 5])
    routes.add(1, [5, 3, 5])
    tlor.write_routes_json(tmp_path / "routes.json", routes)
    assert json.loads((tmp_path / "routes.json").read_text()) == [[5, 0, 1, 5], [5, 3, 5]]
    with pytest.raises(tlor.IOFailure):
        tlor.write_routes_json(tmp_path / "missing" / "routes.json", routes)
