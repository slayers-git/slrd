/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_COMMAND_QUEUE_HPP__
#define __SLRD_COMMAND_QUEUE_HPP__

#include "object.hpp"

#include <memory>
#include <span>

namespace slrd {
    class ICommandBuffer;
    class IFence;

    struct SubmitInfo {
        /* Fence to singal when the execution is complete. */
        IFence *fence;
        /* Which command buffers need to be executed */
        std::span<ICommandBuffer *> commandBuffers;
    };

    class ICommandQueue : public IObject {
    public:
        virtual ~ICommandQueue () = default;

        /* Get command buffer from this queue */
        virtual ICommandBuffer *getCommandBuffer (bool primary = true) = 0;

        /* Reset buffers inherited from here (Only works in specific cases) */
        virtual int reset () = 0;
        /* Wait for the queue to finish (only works if the HARDWARE flag is not specified) */
        virtual int wait () = 0;

        /* Submit work to the gpu */
        virtual int submit (const SubmitInfo& info) = 0;
    };

    enum CommandQueueFlag {
        COMMAND_QUEUE_GRAPHICS = 2,
        COMMAND_QUEUE_TRANSFER = 4,
        COMMAND_QUEUE_COMPUTE  = 8
    };
    using CommandQueueFlags = uint32_t;

    struct CommandQueueInfo {
        CommandQueueFlags flags;
    };
};

#endif /* #define __SLRD_COMMAND_QUEUE_HPP__ */
