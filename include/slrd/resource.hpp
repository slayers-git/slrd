/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_RESOURCE_HPP__
#define __SLRD_RESOURCE_HPP__

#include <string>

namespace slrd {
    template<typename T>
    class Resource {
    private:
        std::string m_name;
        static std::string getDefaultName ();

    public:
        [[nodiscard]] const std::string& getName () {
            return m_name;
        }
        void setName (std::string_view name);
    };
};

#endif /* #define __SLRD_RESOURCE_HPP__ */
