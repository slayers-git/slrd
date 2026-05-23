/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_COMMAND_POOL_HPP__
#define __SLRD_COMMAND_POOL_HPP__

#include "object.hpp"
#include "commandbuffer.hpp"
#include "commandqueue.hpp"

namespace slrd {
    class ICommandQueue;

    enum CommandPoolFlag {
        COMMAND_POOL_FLAG_NONE = 0,
        /**
         * Make it possible for the buffers allocated from this pool to be
         * individually reset.
         *
         * @note The use of this flag is discouraged, as it cannot be supported
         *       on APIs, such as D3D12 */
        COMMAND_POOL_FLAG_INDIVIDUAL_RESET = 1,
        /**
         * Hint that this CommandPool is short-lived. May affect memory
         * underlying memory allocations on certain APIs. */
        COMMAND_POOL_FLAG_TRANSIENT = 2,
    };

    struct CommandPoolInfo {
        /**
         * What type of commands will be written to this queue */
        CommandQueueType type = COMMAND_QUEUE_TYPE_GRAPHICS;
        /**
         * Command pool flags */
        uint32_t flags = COMMAND_POOL_FLAG_NONE;
    };

    /**
     * This class represents a CommandBuffer allocator.
     *
     * If the application is multithreaded, then for every thread that writes
     * command buffers you have to have one allocator that will be used for
     * that thread in particular. */
    class ICommandPool : public IObject {
    public:
        virtual ~ICommandPool () = default;

        /**
         * Reset buffers that were allocated from this pool */
        virtual int reset () = 0;

        /**
         * Allocate a CommandBuffer from this pool */
        virtual Ref<ICommandBuffer> allocate (const CommandBufferInfo&) = 0;
    };
};

#endif /* #define __SLRD_COMMAND_POOL_HPP__ */
