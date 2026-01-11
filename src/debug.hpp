/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_DEBUG_HPP__
#define __SLRD_DEBUG_HPP__

#include <iostream>
#include <source_location>
#include <string>
#include <format>

#include <slrd/config.hpp>

#if SLRD_DEBUG
#define SLRD_ASSERT(__Condition) do {                           \
    if (!(__Condition)) {                                       \
        slrd::debug::cassert (std::source_location::current (), \
                #__Condition);                                  \
    }                                                           \
} while (0)

#define SLRD_DEBUG_CRIT(...) \
    slrd::debug::crit (std::source_location::current (), __VA_ARGS__)
        
#define SLRD_DEBUG_CRIT_IF(__Condition, ...) \
    if ((__Condition))                       \
        slrd::debug::crit (std::source_location::current (), __VA_ARGS__)

/* Complain that something could've been done better */
#define SLRD_COMPLAIN_IF(__Condition, ...) do { \
    if (__Condition) { \
        slrd::debug::complain (std::source_location::current (), \
            #__Condition __VA_OPT__ (, __VA_ARGS__));   \
    }\
} while (0)

#define SLRD_COMPLAIN_RETURN(__Condition, __Return, ...) do { \
    if (__Condition) { \
        slrd::debug::complain (std::source_location::current (), \
            #__Condition __VA_OPT__ (, __VA_ARGS__));   \
        return (__Return); \
    }\
} while (0)

#define SLRD_COMPLAIN_RETURN_NV(__Condition, ...) do { \
    if (__Condition) { \
        slrd::debug::complain (std::source_location::current (), \
            #__Condition __VA_OPT__ (, __VA_ARGS__));   \
        return; \
    }\
} while (0)

#define SLRD_DEBUG_INFO(...) \
    slrd::debug::info (__VA_ARGS__);

#else
#define SLRD_ASSERT(...)
#define SLRD_DEBUG_CRIT(...) \
        slrd::debug::crit (__VA_ARGS__);
#define SLRD_DEBUG_CRIT_IF(__Condition, ...) \
    if ((__Condition))                       \
        slrd::debug::crit (__VA_ARGS__)
#define SLRD_COMPLAIN_IF(...)
#define SLRD_COMPLAIN_RETURN(__Cond, __RetVal, ...) \
    if (__Cond)                                     \
        return (__RetVal)
#define SLRD_COMPLAIN_RETURN_NV(__Cond, ...) \
    if (__Cond)                              \
        return
#define SLRD_DEBUG_INFO(...)
#endif

namespace slrd {
    namespace debug {
        inline void cassert (const std::source_location& loc,
                const std::string& condition) {
            std::cerr << std::format (
                    "Critical error! Assertion failed in {}:{} in function {}: {}.\n",
                    loc.file_name (),
                    loc.line (),
                    loc.function_name (),
                    condition);
        }

        template<typename... Args>
        inline void info (const std::string& format, const Args&... args) {
            std::cout << "Info (debug): " << std::vformat (format,
                    std::make_format_args (args...)) << '\n';
        }

        template<typename... Args>
        inline void complain (const std::source_location& loc,
                const char *condition, const std::string& format,
                const Args&... args) {
            std::cerr << std::format ("Complaint (debug):"
                    "\n\tOn condition: {} in {}:{} in function {}\n\tMessage: ",
                    condition,
                    loc.file_name (), loc.line (), loc.function_name ())
                << std::vformat (format, std::make_format_args (args...)) << '\n';
        }

        template<typename... Args> [[noreturn]]
        inline void crit (const std::string& format, const Args&&... args) {
            std::cerr << "Critical Error: "
                      << std::vformat (format, std::make_format_args (args...))
                      << '\n';

            exit (-1);
        }
        template<typename... Args> [[noreturn]]
        inline void crit (const std::source_location& loc, const std::string& format, const Args&&... args) {
            std::cerr << std::format ("Critical error (debug):"
                            "\n\tIn {}:{} in function {}\n\tMessage: ",
                            loc.file_name (), loc.line (), loc.function_name ())
                      << std::vformat (format, std::make_format_args (args...))
                      << '\n';

            exit (-1);
        }
    };
};

#endif /* #define __SLRD_DEBUG_HPP__ */
