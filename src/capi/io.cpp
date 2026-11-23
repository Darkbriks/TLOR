#include "capi/handles.hpp"

#include <tlor/io.hpp>

#include <cstring>
#include <filesystem>
#include <memory>

using tlor::capi::demand_of;
using tlor::capi::guard;
using tlor::capi::need;
using tlor::capi::need_text;
using tlor::capi::state_or_initial;
using tlor::capi::utf8_path;

extern "C" {

tlor_status tlor_instance_load(const char* tables_dir, const char* map_data_dir, const char* service_time_csv,
                               int load_point_policy, tlor_instance** out, char first_date[11]) {
    return guard([&] {
        need(out, "output pointer") = nullptr;
        tlor::LoadOptions options{
            utf8_path(tables_dir, "tables_dir"),
            utf8_path(map_data_dir, "map_data_dir"),
            service_time_csv == nullptr ? std::filesystem::path{} : utf8_path(service_time_csv, "service_time_csv"),
            load_point_policy != 0,
        };
        auto loaded = tlor::load_instance(options);
        const std::string date = tlor::format_date(loaded.first_date);
        if (date.size() != 10) {
            throw tlor::Error(tlor::Status::invalid_argument, "first date " + date + " does not fit YYYY-MM-DD");
        }
        auto handle = std::make_unique<tlor_instance>(std::make_shared<tlor::Instance>(std::move(loaded.instance)));
        if (first_date != nullptr) {
            std::memcpy(first_date, date.c_str(), date.size() + 1);
        }
        *out = handle.release();
    });
}

tlor_status tlor_write_solution(const char* out_dir, const tlor_instance* instance, const tlor_state* start,
                                const tlor_solution* solution, const double* demand_kg, const char* first_date) {
    return guard([&] {
        const auto& inst = *need(instance, "instance").value;
        const auto date = tlor::parse_date(need_text(first_date, "first_date"));
        const auto demand = demand_of(inst, demand_kg);
        tlor::write_solution(utf8_path(out_dir, "out_dir"), inst, state_or_initial(start, inst),
                             *need(solution, "solution").value, demand ? *demand : inst.demand(), date);
    });
}

tlor_status tlor_write_routes_json(const char* path, const tlor_day_routes* routes) {
    return guard([&] { tlor::write_routes_json(utf8_path(path, "path"), *need(routes, "routes").value); });
}

}
