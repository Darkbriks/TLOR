#include "capi/handles.hpp"

using tlor::capi::emit;
using tlor::capi::guard;
using tlor::capi::need;
using tlor::capi::need_text;

extern "C" {

tlor_status tlor_report_create(tlor_report** out) {
    return guard([&] {
        need(out, "output pointer");
        *out = new tlor_report{std::make_shared<tlor::Report>()};
    });
}

void tlor_report_destroy(tlor_report* report) { delete report; }

tlor_status tlor_report_set_context(tlor_report* report, const char* key, const char* value) {
    return guard([&] { need(report, "report").value->set_context(need_text(key, "key"), need_text(value, "value")); });
}

tlor_status tlor_report_phase_begin(tlor_report* report, const char* name) {
    return guard([&] {
        if (report != nullptr) {
            (void)report->value->begin_phase(need_text(name, "name"));
        }
    });
}

tlor_status tlor_report_phase_end(tlor_report* report) {
    return guard([&] {
        if (report != nullptr) {
            report->value->end_phase();
        }
    });
}

tlor_status tlor_report_count(tlor_report* report, const char* name, int64_t delta) {
    return guard([&] {
        if (report != nullptr) {
            report->value->count(need_text(name, "name"), delta);
        }
    });
}

tlor_status tlor_report_write_json(const tlor_report* report, const char* path) {
    return guard([&] { need(report, "report").value->write_json(std::filesystem::path(need_text(path, "path"))); });
}

}
