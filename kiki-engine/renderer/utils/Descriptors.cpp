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
        bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

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

    DescriptorSetLayout createCubemapDescriptorLayout(VulkanWindow const& window) {
        VkDescriptorSetLayoutBinding bindings[1]{};
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = bindings;

        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        if (auto const res = vkCreateDescriptorSetLayout(window.device, &layoutInfo, nullptr, &layout); res != VK_SUCCESS) {
            throw Kiki::FatalError( "Unable to create descriptor set layout\n" "vkCreateDescriptorSetLayout() returned {}", toString(res));
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

    DescriptorSetLayout createGBufferDescriptorLayout(VulkanWindow const& window) {
        VkDescriptorSetLayoutBinding bindings[5]{};

        // base colour
        bindings[0].binding = 0; // must match the index of the corresponding binding = N declarations in the shaders
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        // normals
        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        // roughness and metalness
        bindings[2].binding = 2;
        bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[2].descriptorCount = 1;
        bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        // world pos
        bindings[3].binding = 3;
        bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[3].descriptorCount = 1;
        bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        // depth buffer
        bindings[4].binding = 4;
        bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[4].descriptorCount = 1;
        bindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
        layoutInfo.pBindings = bindings;

        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        if (auto const res = vkCreateDescriptorSetLayout(window.device, &layoutInfo, nullptr, &layout); VK_SUCCESS != res) {
            throw Kiki::FatalError("Unable to create descriptor set layout\n" "vkCreateDescriptorSetLayout() returned {}", toString(res));
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

    void initialiseDeferredLightingDescriptorSet(VulkanWindow const& window, GBuffers& gbuffers, Image& depthBuffer, Sampler& sampler, VkDescriptorSet& deferredLightingDescriptors) {
        VkWriteDescriptorSet desc[5]{};

        VkDescriptorImageInfo texColourInfo{};
        texColourInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        texColourInfo.imageView = gbuffers.textureColour.view;
        texColourInfo.sampler = sampler.handle;

        VkDescriptorImageInfo normalsInfo{};
        normalsInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        normalsInfo.imageView = gbuffers.normals.view;
        normalsInfo.sampler = sampler.handle;

        VkDescriptorImageInfo roughnessMetalnessInfo{};
        roughnessMetalnessInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        roughnessMetalnessInfo.imageView = gbuffers.roughnessMetalness.view;
        roughnessMetalnessInfo.sampler = sampler.handle;

        VkDescriptorImageInfo worldPosInfo{};
        worldPosInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        worldPosInfo.imageView = gbuffers.worldPos.view;
        worldPosInfo.sampler = sampler.handle;

        VkDescriptorImageInfo depthInfo{};
        depthInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        depthInfo.imageView = depthBuffer.view;
        depthInfo.sampler = sampler.handle;

        desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        desc[0].dstSet = deferredLightingDescriptors;
        desc[0].dstBinding = 0;
        desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        desc[0].descriptorCount = 1;
        desc[0].pImageInfo = &texColourInfo;

        desc[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        desc[1].dstSet = deferredLightingDescriptors;
        desc[1].dstBinding = 1;
        desc[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        desc[1].descriptorCount = 1;
        desc[1].pImageInfo = &normalsInfo;

        desc[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        desc[2].dstSet = deferredLightingDescriptors;
        desc[2].dstBinding = 2;
        desc[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        desc[2].descriptorCount = 1;
        desc[2].pImageInfo = &roughnessMetalnessInfo;

        desc[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        desc[3].dstSet = deferredLightingDescriptors;
        desc[3].dstBinding = 3;
        desc[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        desc[3].descriptorCount = 1;
        desc[3].pImageInfo = &worldPosInfo;

        desc[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        desc[4].dstSet = deferredLightingDescriptors;
        desc[4].dstBinding = 4;
        desc[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        desc[4].descriptorCount = 1;
        desc[4].pImageInfo = &depthInfo;

        vkUpdateDescriptorSets(window.device, 5, desc, 0, nullptr);
    }
}