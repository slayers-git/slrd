#ifndef __SLRD_FRAMEWORK_PROFILER_HPP__
#define __SLRD_FRAMEWORK_PROFILER_HPP__

#include <chrono>
#include <unordered_map>

struct ProfilerData {
    /* Scope name */
    const char *name;
    float duration = 0;

    std::chrono::time_point<std::chrono::high_resolution_clock> tp {};
};

class Profiler {
private:
    std::unordered_map<const char *, ProfilerData> m_scopes;

public:
    Profiler () = default;

    void newFrame ();
    void endFrame ();

    void printData ();
    void startScope (const char *scopeName);
    void endScope (const char *scopeName);

    std::vector<ProfilerData> getData () const;
};

#endif /* #define __SLRD_FRAMEWORK_PROFILER_HPP__ */
