#include <tlor/report.hpp>

#include <tlor/version.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <map>
#include <vector>

namespace tlor {

namespace {

using Clock = std::chrono::steady_clock;

struct Node {
    std::string name;
    std::int64_t count = 0;
    std::int64_t total_ns = 0;
    std::int64_t min_ns = 0;
    std::int64_t max_ns = 0;
    std::map<std::string, std::int64_t, std::less<>> counters;
    std::vector<std::unique_ptr<Node>> children;

    Node* child(std::string_view child_name) {
        for (auto& c : children) {
            if (c->name == child_name) {
                return c.get();
            }
        }
        children.push_back(std::make_unique<Node>(Node{std::string(child_name)}));
        return children.back().get();
    }

    void record(std::int64_t ns) {
        min_ns = count == 0 ? ns : std::min(min_ns, ns);
        max_ns = count == 0 ? ns : std::max(max_ns, ns);
        total_ns += ns;
        ++count;
    }
};

struct Call {
    CallInfo info;
    Status status = Status::ok;
    std::int64_t wall_ns = 0;
    Node root;
    std::optional<Indicators> indicators;
};

enum class FrameKind { phase, call, nested_call };

struct Frame {
    FrameKind kind;
    Node* node;
    Call* call;
    Clock::time_point start;
};

std::string utc_now() {
    const auto now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
    const auto days = std::chrono::floor<std::chrono::days>(now);
    const std::chrono::year_month_day ymd{days};
    const std::chrono::hh_mm_ss hms{now - days};
    char buffer[32];
    std::snprintf(buffer, sizeof buffer, "%04d-%02u-%02uT%02d:%02d:%02dZ", static_cast<int>(ymd.year()),
                  static_cast<unsigned>(ymd.month()), static_cast<unsigned>(ymd.day()),
                  static_cast<int>(hms.hours().count()), static_cast<int>(hms.minutes().count()),
                  static_cast<int>(hms.seconds().count()));
    return buffer;
}

nlohmann::json to_json(const Value& value) {
    return std::visit([](const auto& v) { return nlohmann::json(v); }, value);
}

nlohmann::json counters_json(const Node& node) {
    nlohmann::json counters = nlohmann::json::object();
    for (const auto& [name, value] : node.counters) {
        counters[name] = value;
    }
    return counters;
}

nlohmann::json children_json(const Node& node) {
    nlohmann::json children = nlohmann::json::array();
    for (const auto& child : node.children) {
        children.push_back({
            {"name", child->name},
            {"count", child->count},
            {"total_ns", child->total_ns},
            {"min_ns", child->min_ns},
            {"max_ns", child->max_ns},
            {"counters", counters_json(*child)},
            {"children", children_json(*child)},
        });
    }
    return children;
}

}

struct Report::Impl {
    std::string started_at = utc_now();
    std::map<std::string, std::string, std::less<>> user;
    Node root;
    std::vector<std::unique_ptr<Call>> calls;
    std::vector<Frame> frames;

    Node& container() { return frames.empty() ? root : *frames.back().node; }

    std::int64_t elapsed(const Frame& frame) const {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - frame.start).count();
    }
};

Report::Report() : impl_(std::make_unique<Impl>()) {}

Report::~Report() = default;

Report::Report(Report&&) noexcept = default;

Report& Report::operator=(Report&&) noexcept = default;

void Report::set_context(std::string key, std::string value) { impl_->user.insert_or_assign(std::move(key), std::move(value)); }

void Report::begin_phase(std::string_view name) {
    Node* node = impl_->container().child(name);
    impl_->frames.push_back({FrameKind::phase, node, nullptr, Clock::now()});
}

void Report::end_phase() {
    if (impl_->frames.empty() || impl_->frames.back().kind != FrameKind::phase) {
        throw Error(Status::invalid_argument, "no open phase to end");
    }
    const Frame frame = impl_->frames.back();
    impl_->frames.pop_back();
    frame.node->record(impl_->elapsed(frame));
}

void Report::count(std::string_view name, std::int64_t delta) {
    auto& counters = impl_->container().counters;
    auto it = counters.find(name);
    if (it == counters.end()) {
        counters.emplace(std::string(name), delta);
    } else {
        it->second += delta;
    }
}

void Report::begin_call(CallInfo info) {
    if (in_call()) {
        Node* node = impl_->container().child("call:" + info.step + "/" + info.method);
        impl_->frames.push_back({FrameKind::nested_call, node, nullptr, Clock::now()});
        return;
    }
    auto call = std::make_unique<Call>();
    call->info = std::move(info);
    Call* raw = call.get();
    impl_->calls.push_back(std::move(call));
    impl_->frames.push_back({FrameKind::call, &raw->root, raw, Clock::now()});
}

void Report::end_call(Status status, std::optional<Indicators> indicators) {
    if (impl_->frames.empty() || impl_->frames.back().kind == FrameKind::phase) {
        throw Error(Status::invalid_argument, "no open call to end");
    }
    const Frame frame = impl_->frames.back();
    impl_->frames.pop_back();
    if (frame.kind == FrameKind::nested_call) {
        frame.node->record(impl_->elapsed(frame));
        return;
    }
    frame.call->wall_ns = impl_->elapsed(frame);
    frame.call->status = status;
    frame.call->indicators = std::move(indicators);
}

bool Report::in_call() const noexcept {
    return std::ranges::any_of(impl_->frames, [](const Frame& f) { return f.kind != FrameKind::phase; });
}

std::string Report::to_json() const {
    nlohmann::json user = nlohmann::json::object();
    for (const auto& [key, value] : impl_->user) {
        user[key] = value;
    }
    nlohmann::json calls = nlohmann::json::array();
    for (const auto& call : impl_->calls) {
        nlohmann::json params = nlohmann::json::object();
        for (const auto& [name, value] : call->info.params.values()) {
            params[name] = tlor::to_json(value);
        }
        const Value* seed = call->info.params.find("seed");
        nlohmann::json indicators = nullptr;
        if (call->indicators) {
            indicators = nlohmann::json::object();
            for (const auto& [name, value] : call->indicators->values) {
                indicators[name] = value;
            }
        }
        calls.push_back({
            {"step", call->info.step},
            {"method", call->info.method},
            {"params", params},
            {"seed", seed != nullptr ? tlor::to_json(*seed) : nlohmann::json(nullptr)},
            {"instance",
             {{"nodes", call->info.size.nodes},
              {"active_points", call->info.size.active_points},
              {"days", call->info.size.days},
              {"trucks", call->info.size.trucks}}},
            {"status", std::string(to_string(call->status))},
            {"wall_ns", call->wall_ns},
            {"phases", children_json(call->root)},
            {"counters", counters_json(call->root)},
            {"series", nlohmann::json::object()},
            {"indicators", indicators},
        });
    }
    nlohmann::json report = {
        {"schema_version", 1},
        {"context",
         {{"library_version", std::string(library_version())},
          {"git_commit", std::string(git_commit())},
          {"build_type", std::string(build_type())},
          {"compiler", std::string(compiler())},
          {"os", std::string(os_name())},
          {"started_at", impl_->started_at},
          {"user", user}}},
        {"phases", children_json(impl_->root)},
        {"counters", counters_json(impl_->root)},
        {"calls", calls},
    };
    return report.dump(2);
}

void Report::write_json(const std::filesystem::path& path) const {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw Error(Status::io_error, "cannot open report file " + path.string());
    }
    out << to_json() << '\n';
    out.close();
    if (!out) {
        throw Error(Status::io_error, "cannot write report file " + path.string());
    }
}

Phase::Phase(Report* report, std::string_view name) : report_(report) {
    if (report_ != nullptr) {
        report_->begin_phase(name);
    }
}

Phase::~Phase() {
    if (report_ != nullptr) {
        try {
            report_->end_phase();
        } catch (...) {
        }
    }
}

void count(Report* report, std::string_view name, std::int64_t delta) {
    if (report != nullptr) {
        report->count(name, delta);
    }
}

}
