#include "Descriptors.hpp"

#include "ToString.hpp"
#include "../../logging/FatalError.hpp"

namespace rutils {
    DescriptorSetLayout createSceneDescriptorLayout(VulkanWindow const& window) {
        VkDescriptorSetLayoutBinding bindings[1]{};
        bindings[0].binding = 0; // number must match the index of the corresponding
        // binding = N declaration in the shader(s)!
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = sizeof(bindings)/sizeof(bindings[0]);
        layoutInfo.pBindings = bindings;

        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        if (auto const res = vkCreateDescriptorSetLayout(window.device, &layoutInfo, nullptr, &layout); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Unable to create descriptor set layout\n"
                "vkCreateDescriptorSetLayout() returned {}", toString(res)
            );
        }

        return DescriptorSetLayout(window.device, layout);
    }

    DescriptorSetLayout createObjectDescriptorLayout(VulkanWindow const& window) {
        VkDescriptorSetLayoutBinding bindings[1]{};
        bindings[0].binding = 0; // this must match the shaders
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = sizeof(bindings)/sizeof(bindings[0]);
        layoutInfo.pBindings = bindings;

        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        if (auto const res = vkCreateDescriptorSetLayout(window.device, &layoutInfo, nullptr, &layout); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Unable to create descriptor set layout\n"
                "vkCreateDescriptorSetLayout() returned {}", toString(res)
            );
        }

        return DescriptorSetLayout(window.device, layout);
    }

    DescriptorSetLayout createMaterialDescriptorLayout(VulkanWindow const& window) {
        VkDescriptorSetLayoutBinding bindings[2]{};

        // base colour
        bindings[0].binding = 0; // must match the index of the corresponding binding = N declarations in the shaders
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; // different to create_scene_descriptor_layout
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        // roughness and metalness
        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
        layoutInfo.pBindings = bindings;

        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        if (auto const res = vkCreateDescriptorSetLayout(window.device, &layoutInfo, nullptr, &layout); res != VK_SUCCESS) {
            throw Kiki::FatalError("Unable to create descriptor set layout\n" "vkCreateDescriptorSetLayout() returned {}", rutils::toString(res));
        }

        return DescriptorSetLayout(window.device, layout);
    }

    DescriptorPool createDescriptorPool(VulkanWindow const& window, std::uint32_t aMaxDescriptors, std::uint32_t aMaxSets) {
        VkDescriptorPoolSize const pools[] = {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, aMaxDescriptors },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, aMaxDescriptors }
        };

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.maxSets = aMaxSets;
        poolInfo.poolSizeCount = sizeof(pools)/sizeof(pools[0]);
        poolInfo.pPoolSizes = pools;

        VkDescriptorPool pool = VK_NULL_HANDLE;
        if (auto const res = vkCreateDescriptorPool(window.device, &poolInfo, nullptr, &pool); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Unable to create descriptor pool\n"
                "vkCreateDescriptorPool() returned {}", toString(res)
            );
        }

        return DescriptorPool(window.device, pool);
    }

    VkDescriptorSet allocDescSet(VulkanWindow const& window, VkDescriptorPool aPool, VkDescriptorSetLayout aSetLayout) {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = aPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &aSetLayout;

        VkDescriptorSet dset = VK_NULL_HANDLE;
        if (auto const res = vkAllocateDescriptorSets(window.device, &allocInfo, &dset); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Unable to allocate descriptor set\n"
                "vkAllocateDescriptorSets() returned {}", toString(res)
            );
        }

        return dset;
    }
}