/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_ERROR_HPP__
#define __SLRD_ERROR_HPP__

#include <string>

namespace slrd {
    /* Get error message for the failed command */
    const std::string& getErrorString ();
};

#endif /* #define __SLRD_ERROR_HPP__ */
