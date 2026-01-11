/* SPDX-License-Identifer: LGPL-3.0-or-later */

#include "error.hpp"

namespace slrd {
    const std::string& getErrorString () {
        return g_errorString;
    }
};
