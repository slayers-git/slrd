/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_COMMAND_QUEUE_HPP__
#define __SLRD_COMMAND_QUEUE_HPP__

#include "object.hpp"

#include <memory>
#include <span>

namespace slrd {
    class ICommandBuffer;
    class IFence;

    struct FenceSubmitInfo {
        IFence *fence = nullptr;
        uint64_t value = 0;
    };

    struct SubmitInfo {
        /* Fences to wait on, before executing the command buffers */
        std::span<FenceSubmitInfo> waitFences;
        /* Fences to signal after executing the command buffers */
        std::span<FenceSubmitInfo> signalFences;

        /* Which command buffers need to be executed */
        std::span<ICommandBuffer *> commandBuffers;
    };

    class ICommandQueue : public IObject {
    public:
        virtual ~ICommandQueue () = default;

        /* Wait for the queue to finish */
        virtual int wait () = 0;

        /* Submit work to the gpu */
        virtual int submit (const SubmitInfo& info) = 0;
    };

    enum CommandQueueType {
        /**
         * This queue has to support graphics commands */
        COMMAND_QUEUE_TYPE_GRAPHICS = 1,
        /**
         * This queue has to support compute commands */
        COMMAND_QUEUE_TYPE_COMPUTE  = 2,
        /**
         * This queue has to support transfer commands */
        COMMAND_QUEUE_TYPE_TRANSFER = 3,
    };

    struct CommandQueueInfo {
        /**
         * Type of the queue that defines what commands can be processed by it
         *
         * @note Depending on the API a command queue of type graphics may also
         *       be able to execute compute. However, for async compute prefer
         *       creating two separate queues with types graphics and compute. */
        CommandQueueType type = COMMAND_QUEUE_TYPE_GRAPHICS;
    };
};

#endif /* #define __SLRD_COMMAND_QUEUE_HPP__ */
