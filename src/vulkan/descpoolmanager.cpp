/* SPDX-License-Identifer: LGPL-3.0-or-later */

#include "descpoolmanager.hpp"
#include "debug.hpp"
#include "vulkan/error.hpp"

namespace slrd {
    int DescriptorPoolManager::init (VkDevice device, const PoolKey& key, uint32_t initial_sets) {
        m_key = key;
        m_device = device;
        m_setsPerPool = initial_sets;

        auto poolIdx = createPool (initial_sets);
        if (poolIdx == UINT32_MAX) {
            return -1;
        }

        return 0;
    }

    uint32_t DescriptorPoolManager::getPoolID () {
        if (!m_readyPools.empty()) {
            return *m_readyPools.begin();
        }

        return UINT32_MAX;
    }

    VkDescriptorSet DescriptorPoolManager::allocateInPool(uint32_t poolIdx,
            VkDescriptorSetLayout layout) {
        VkDescriptorSet vkset;
        auto pool = m_pools[poolIdx].pool;

        VkDescriptorSetAllocateInfo set_info {};
        set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        set_info.pSetLayouts = &layout;
        set_info.descriptorSetCount = 1;
        set_info.descriptorPool = pool;

        auto res = vkAllocateDescriptorSets(m_device, &set_info, &vkset);
        if (res == VK_ERROR_OUT_OF_POOL_MEMORY || res == VK_ERROR_FRAGMENTED_POOL) {
            return VK_NULL_HANDLE;
        }

        m_pools[poolIdx].allocations++;
        return vkset;
    }

    std::tuple<VkDescriptorSet, uint32_t> DescriptorPoolManager::allocateSet (VkDescriptorSetLayout layout) {
        VkDescriptorSet vkset;
        uint32_t pool_idx;

        auto it = m_readyPools.begin();
        while (it != m_readyPools.end()) {
            pool_idx = *it;
            vkset = allocateInPool(pool_idx, layout);
            if (vkset) {
                break;
            } else {
                m_pools[pool_idx].state = POOL_STATE_FULL;
                it = m_readyPools.erase(it);
            }
        };

        if (!vkset) {
            uint32_t pool_idx = createPool(m_setsPerPool);
            if (pool_idx == UINT32_MAX) {
                return std::make_tuple(VK_NULL_HANDLE, UINT32_MAX);
            }

            m_setsPerPool *= 2;
            vkset = allocateInPool(pool_idx, layout);
            if (!vkset) 
                return std::make_tuple(VK_NULL_HANDLE, UINT32_MAX);
        }

        return std::make_tuple(vkset, pool_idx);
    }

    uint32_t DescriptorPoolManager::createPool (uint32_t setsPerPool) {
        std::vector<VkDescriptorPoolSize> poolSizes;
        poolSizes.reserve (m_key.m_array.size ());
        for (uint32_t i = 0; i < m_key.m_array.size (); ++i) {
            if (m_key.m_array[i] > 0) {
                VkDescriptorPoolSize ps;
                ps.type = VkDescriptorType (i);
                ps.descriptorCount = m_key.m_array[i] * setsPerPool;

                poolSizes.push_back (ps);
            }
        }

        VkDescriptorPoolCreateInfo pInfo {};
        pInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pInfo.maxSets = setsPerPool;
        pInfo.pPoolSizes = poolSizes.data ();
        pInfo.poolSizeCount = poolSizes.size ();
        pInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        VkDescriptorPool vkpool;
        VK_WRAP_RETURN (vkCreateDescriptorPool (m_device, &pInfo, nullptr, &vkpool),
                UINT32_MAX);

        for (uint32_t i = 0; i < m_pools.size (); ++i) {
            if (m_pools[i].state == POOL_STATE_UNALLOCATED) {
                m_pools[i] = { vkpool, POOL_STATE_READY, 0 };
                m_readyPools.push_back(i);
                return i;
            }
        }

        m_pools.push_back ({ vkpool, POOL_STATE_READY, 0 });
        m_readyPools.push_back(m_pools.size() - 1);

        return m_pools.size () - 1;
    }

    void DescriptorPoolManager::deletePool (uint32_t poolIdx) {
        SLRD_ASSERT (poolIdx < m_pools.size ());
        if (m_pools[poolIdx].pool)
            vkDestroyDescriptorPool (m_device, m_pools[poolIdx].pool, nullptr);
        m_pools[poolIdx] = { VK_NULL_HANDLE, POOL_STATE_UNALLOCATED, 0 };
        m_readyPools.erase(std::find(m_readyPools.begin(),
                    m_readyPools.end(), poolIdx));
    }

    void DescriptorPoolManager::freeSet (uint32_t pool, VkDescriptorSet set) {
        SLRD_ASSERT (pool < m_pools.size ());
        SLRD_ASSERT (m_pools[pool].state != POOL_STATE_UNALLOCATED);
        SLRD_DEBUG_CRIT_IF (m_pools[pool].allocations == 0,
                "The number of allocations in the pool is zero, but there is a " \
                "request to free a set allocated from this pool. This might " \
                "indicate a serious memory-related bug in the application!");

        m_pools[pool].allocations--;
        if (m_pools[pool].state == POOL_STATE_FULL) {
            m_readyPools.push_back(pool);
        }
        m_pools[pool].state = POOL_STATE_READY;

        if (m_pools[pool].allocations == 0) {
            m_freePoolsAmount++;
            if (m_freePoolsAmount > MAX_FREE_POOLS) {
                clearUnusedPools ();
            }
        }

        vkFreeDescriptorSets (m_device, m_pools[pool].pool, 1, &set);
    }

    void DescriptorPoolManager::clearUnusedPools () {
        for (uint32_t i = 0; i < m_pools.size (); ++i) {
            if (m_pools[i].state != POOL_STATE_UNALLOCATED && !m_pools[i].allocations) {
                deletePool (i);
            }
        }
    }

    /* Reset all pools */
    void DescriptorPoolManager::reset () {
        for (uint32_t pool_idx = 0; pool_idx < m_pools.size(); ++pool_idx) {
            auto& pool = m_pools[pool_idx];
            vkResetDescriptorPool (m_device, pool.pool, 0);

            pool.allocations = 0;
            if (pool.state == POOL_STATE_FULL)
                m_readyPools.push_back(pool_idx);

            pool.state = POOL_STATE_READY;
        }
    }

    void DescriptorPoolManager::clear () {
        for (uint32_t i = 0; i < m_pools.size (); ++i) {
            deletePool (i);
        }
    }

    DescriptorPoolManager::~DescriptorPoolManager () {
        clear ();
    }
};
