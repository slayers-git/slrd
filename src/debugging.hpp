/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_DEBUGGING_HPP__
#define __SLRD_DEBUGGING_HPP__

namespace slrd {
    /* Check if the debugger is present */
    void isDebuggerPresent ();
    /* Set the breakpoint */
    void setBreakpointIfDebuggerPresent ();
};

#endif /* #define __SLRD_DEBUGGING_HPP__ */
