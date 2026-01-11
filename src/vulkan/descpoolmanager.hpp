/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __VULKAN_DESCPOOLMANAGER_HPP__
#define __VULKAN_DESCPOOLMANAGER_HPP__

#include <vector>
#include <vulkan/vulkan.h>
#include <array>

namespace slrd {
    /* This is the key that identifies a pool */
    struct PoolKey {
        std::array<size_t, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT + 1> m_array;

        PoolKey () {
            m_array.fill (0);
        };

        bool operator< (const PoolKey& other) const {
            return m_array < other.m_array;
        }
    };

    /* This class manages pools, so that the allocations can happen all the time */
    class DescriptorPoolManager {
    private:
        enum PoolState {
            POOL_STATE_UNALLOCATED,
            POOL_STATE_READY,
            POOL_STATE_FULL
        };
        struct PoolInfo {
            VkDescriptorPool pool = VK_NULL_HANDLE;
            PoolState state = POOL_STATE_UNALLOCATED;
            uint32_t allocations = 0;
        };

        /* The key to the pool */
        PoolKey m_key;
        VkDevice m_device = VK_NULL_HANDLE;

        /* The pools managed by this manager */
        std::vector<PoolInfo> m_pools;

        /* The amount of completely free pools */
        uint32_t m_freePoolsAmount;

        /* how many sets per pool should we allocate next */
        uint32_t m_setsPerPool;

        /* Delete the pool by this id */
        void deletePool (uint32_t poolIdx);

    public:
        /* The maximum amount of free pools allowed before they are freed */
        static constexpr uint32_t MAX_FREE_POOLS = 2;

        DescriptorPoolManager () = default;
        ~DescriptorPoolManager ();

        /* Initialize the pool manager with an initial pool allocated
         * it will have exactly N initial sets available. */
        int init (VkDevice device, const PoolKey& key, uint32_t initial_sets);

        /* Get a pool for allocations */
        uint32_t getPoolID ();

        /* Create a pool */
        uint32_t createPool (uint32_t setsPerPool);

        /* Destroy all pools that are not used by any UniformSet */
        void clearUnusedPools ();

        /* Allocate descriptor sets from the pool */
        std::tuple<VkDescriptorSet, uint32_t> allocateSet (VkDescriptorSetLayout layout);
        /* Free the descriptor set that was allocated from this manager */
        void freeSet (uint32_t pool, VkDescriptorSet set);
    
        /* Reset all pools */
        void reset ();

        /* Clear all pools */
        void clear ();
    };
};

#endif /* #define __VULKAN_DESCPOOLMANAGER_HPP__ */
