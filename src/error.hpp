/* SPDX-License-Identifer: LGPL-3.0-or-later */

#include <format>
#include <slrd/error.hpp>

namespace slrd {
    inline thread_local std::string g_errorString = "";

    template<typename... Args>
    inline void setError (const std::string& fmt, Args&&... args) {
        g_errorString = std::vformat (fmt, std::make_format_args (args...));
    }
};
