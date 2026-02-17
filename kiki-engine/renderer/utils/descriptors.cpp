#include "descriptors.hpp"

#include "error.hpp"
#include "to_string.hpp"

namespace labut2
{
    DescriptorPool create_descriptor_pool(VulkanContext const& aContext, std::uint32_t aMaxDescriptors, std::uint32_t aMaxSets) {
        VkDescriptorPoolSize const pools[] = {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, aMaxDescriptors},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, aMaxDescriptors}
        };

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.maxSets = aMaxSets;
        poolInfo.poolSizeCount = sizeof(pools) / sizeof(pools[0]);
        poolInfo.pPoolSizes = pools;

        VkDescriptorPool pool = VK_NULL_HANDLE;
        if (auto const res = vkCreateDescriptorPool(aContext.device, &poolInfo, nullptr, &pool); res != VK_SUCCESS) {
            throw Error("Unable to create descriptor pool\n" "vkCreateDescriptorPool() returned {}", to_string(res));
        }

        return DescriptorPool(aContext.device, pool);
    }

    VkDescriptorSet alloc_desc_set(VulkanContext const& aContext, VkDescriptorPool aPool, VkDescriptorSetLayout aDescriptorSetLayout) {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = aPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &aDescriptorSetLayout;

        VkDescriptorSet dset = VK_NULL_HANDLE;
        if (auto const res = vkAllocateDescriptorSets(aContext.device, &allocInfo, &dset); res != VK_SUCCESS) {
            throw Error("Unable to allocate descriptor set\n" "vkAllocateDescriptorSets() returned {}", to_string(res));
        }

        return dset;
    }
}
