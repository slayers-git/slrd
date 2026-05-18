#include "slrdframework/profiler.hpp"
#include <iostream>

inline std::unique_ptr<Profiler> s_profiler;

void Profiler::newFrame () {
    m_scopes.clear ();
    startScope ("Frame");
}

void Profiler::endFrame () {
    endScope ("Frame");
}

void Profiler::printData () {
    std::cout << "Profiler frame: \n";
    for (auto scope : m_scopes) {
        auto& data = scope.second;
        std::cout << std::format ("- {}: {:.6f}ms\n", data.name, data.duration);
    }
}

void Profiler::startScope (const char *scopeName) {
    if (m_scopes.find (scopeName) == m_scopes.end ()) {
        m_scopes.emplace (scopeName, ProfilerData{ scopeName, 0,
                std::chrono::high_resolution_clock::now () });
    } else {
        m_scopes[scopeName].tp = std::chrono::high_resolution_clock::now ();
    }
}

void Profiler::endScope (const char *scopeName) {
    if (m_scopes.find (scopeName) == m_scopes.end ())
        throw std::runtime_error ("invalid scope");
    auto& scope = m_scopes[scopeName];
    std::chrono::duration<float, std::nano> dur = std::chrono::high_resolution_clock::now () -
        scope.tp;

    scope.duration += dur.count () / 1'000'000'000;
}

std::vector<ProfilerData> Profiler::getData () const {
    std::vector<ProfilerData> data;
    data.reserve (m_scopes.size ());
    for (const auto& scope : m_scopes) {
        data.emplace_back (scope.second);
    }

    return data;
}
