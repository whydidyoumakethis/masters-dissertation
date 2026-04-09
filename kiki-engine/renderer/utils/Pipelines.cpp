#include "Pipelines.hpp"

#include "ToString.hpp"
#include "Shaders.hpp"
#include "../../logging/FatalError.hpp"

#include <print>
#include <iostream>

namespace rutils {
    Pipelines createAllPipelines(VulkanWindow const& window, PipelineLayouts const& pipelineLayouts) {
        Pipelines pipelines;

        pipelines.pbr = createPipeline(window, pipelineLayouts.pbrPipelineLayout.handle);
        pipelines.pbr_alpha = createAlphaPipeline(window, pipelineLayouts.pbrPipelineLayout.handle);
        pipelines.deferred_geometry = createDeferredGeometryPipeline(window, pipelineLayouts.pbrPipelineLayout.handle);
        pipelines.deferred_geometry_alpha = createDeferredGeometryAlphaPipeline(window, pipelineLayouts.pbrPipelineLayout.handle);
        pipelines.deferred_lighting = createDeferredLightingPipeline(window, pipelineLayouts.deferredPipelineLayout.handle);
        pipelines.skybox = createSkyboxPipeline(window, pipelineLayouts.skyboxPipelineLayout.handle);

        return pipelines;
    }

    void setup_viewport(VulkanWindow const& aWindow, VkViewport* viewport, VkRect2D* scissor, VkPipelineViewportStateCreateInfo* viewportInfo) {
        // define viewport and scissor regions
        viewport->x = 0.f;
        viewport->y = 0.f;
        viewport->width = float(aWindow.swapchainExtent.width);
        viewport->height = float(aWindow.swapchainExtent.height);
        viewport->minDepth = 0.f;
        viewport->maxDepth = 1.f;

        scissor->offset = VkOffset2D{0, 0};
        scissor->extent = VkExtent2D{aWindow.swapchainExtent.width, aWindow.swapchainExtent.height};

        viewportInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportInfo->viewportCount = 1;
        viewportInfo->pViewports = viewport;
        viewportInfo->scissorCount = 1;
        viewportInfo->pScissors = scissor;
    }

    void setup_vertex_inputs(VkVertexInputBindingDescription* vertexInputs, VkVertexInputAttributeDescription* vertexAttributes, VkPipelineVertexInputStateCreateInfo* inputInfo, VkPipelineInputAssemblyStateCreateInfo* assemblyInfo) {
        // positions
        vertexInputs[0].binding = 0;
        vertexInputs[0].stride = sizeof(glm::vec3);
        vertexInputs[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        // texcoords
        vertexInputs[1].binding = 1;
        vertexInputs[1].stride = sizeof(glm::vec2);
        vertexInputs[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        // normals
        vertexInputs[2].binding = 2;
        vertexInputs[2].stride = sizeof(glm::vec3);
        vertexInputs[2].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        vertexAttributes[0].binding = 0; // must match binding above
        vertexAttributes[0].location = 0; // must match shader
        vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertexAttributes[0].offset = 0;

        vertexAttributes[1].binding = 1; // must match binding above
        vertexAttributes[1].location = 1; // must match shader
        vertexAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
        vertexAttributes[1].offset = 0;

        vertexAttributes[2].binding = 2;
        vertexAttributes[2].location = 2; // must match shader
        vertexAttributes[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertexAttributes[2].offset = 0;

        inputInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        inputInfo->vertexBindingDescriptionCount = 3; // number of vertexInputs above
        inputInfo->pVertexBindingDescriptions = vertexInputs;
        inputInfo->vertexAttributeDescriptionCount = 3; // number of vertexAttributes above
        inputInfo->pVertexAttributeDescriptions = vertexAttributes;

        // define which primitive (point, line, triangle...) the input is assembled into for rasterisation
        assemblyInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        assemblyInfo->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        assemblyInfo->primitiveRestartEnable = VK_FALSE;
    }

    PipelineLayout createPipelineLayout(VulkanWindow const& window, VkDescriptorSetLayout sceneLayout, VkDescriptorSetLayout materialLayout) {
        VkDescriptorSetLayout layouts[] = {
            // Order must match the set = N in the shaders
            sceneLayout, // set 0
            materialLayout
        };

        VkPushConstantRange pushRange{};
        pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushRange.offset = 0;
        pushRange.size = sizeof(ObjectData);

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = sizeof(layouts)/sizeof(layouts[0]); // updated!
        layoutInfo.pSetLayouts = layouts; // updated!
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pushRange;

        VkPipelineLayout layout = VK_NULL_HANDLE;
        if (auto const res = vkCreatePipelineLayout(window.device, &layoutInfo, nullptr, &layout); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Unable to create pipeline layout\n"
                "vkCreatePipelineLayout() returned {}", toString(res)
            );
        }

        return rutils::PipelineLayout(window.device, layout);
    }

    Pipeline createPipeline(VulkanWindow const& window, VkPipelineLayout pipelineLayout) {
        // Load shader code
        // For this example, we only use the vertex and fragment shaders. Other shader stages (geometry, tessellation)
        // aren’t needed here, and as such we omit them.
        //
        // This uses a Vulkan 1.4 feature (from VK KHR maintenance5), which allows us to skip the VkShaderModule
        // creation and instead directly pass SPIR-V code to the pipeline creation.
        std::filesystem::current_path(PROJECT_BINARY_DIR);
        auto const vShader = rutils::loadShader("shaders/compiled/default.vert.spv");
        auto const fShader = rutils::loadShader("shaders/compiled/default.frag.spv");

        VkShaderModuleCreateInfo code[2]{};
        code[0].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[0].codeSize = vShader.size()*sizeof(std::uint32_t);
        code[0].pCode = vShader.data();

        code[1].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[1].codeSize = fShader.size()*sizeof(std::uint32_t);
        code[1].pCode = fShader.data();

        // Define shader stages in the pipeline
        VkPipelineShaderStageCreateInfo stages[2]{};
        // stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        // stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        // stages[0].pName = "main";
        // stages[0].pNext = &code[0];

        // stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        // stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        // stages[1].pName = "main";
        // stages[1].pNext = &code[1];
        VkShaderModule vertModule;
        VkShaderModule fragModule;

        vkCreateShaderModule(window.device, &code[0], nullptr, &vertModule);
        vkCreateShaderModule(window.device, &code[1], nullptr, &fragModule);

        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vertModule;
        stages[0].pName = "main";

        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = fragModule;
        stages[1].pName = "main";

        VkVertexInputBindingDescription vertexInputs[3]{};
        VkVertexInputAttributeDescription vertexAttributes[3]{};
        VkPipelineVertexInputStateCreateInfo inputInfo{};
        VkPipelineInputAssemblyStateCreateInfo assemblyInfo{};
        setup_vertex_inputs(vertexInputs, vertexAttributes, &inputInfo, &assemblyInfo);

        VkViewport viewport{};
        VkRect2D scissor{};
        VkPipelineViewportStateCreateInfo viewportInfo{};
        setup_viewport(window, &viewport, &scissor, &viewportInfo);

        // Define rasterization options
        VkPipelineRasterizationStateCreateInfo rasterInfo{};
        rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterInfo.depthClampEnable = VK_FALSE;
        rasterInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterInfo.depthBiasEnable = VK_FALSE;
        rasterInfo.lineWidth = 1.f; // required.

        // Define multisampling state
        VkPipelineMultisampleStateCreateInfo samplingInfo{};
        samplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        samplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // Define blend state
        // We define one blend state per color attachment - this example uses a single color attachment, so we only
        // need one. Right now, we don’t do any blending, so we can ignore most of the members.
        VkPipelineColorBlendAttachmentState blendStates[1]{};
        blendStates[0].blendEnable = VK_FALSE;
        blendStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo blendInfo{};
        blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        blendInfo.logicOpEnable = VK_FALSE;
        blendInfo.attachmentCount = 1;
        blendInfo.pAttachments = blendStates;

        // Pipeline rendering info
        // This is related to dynamic rendering (core in Vulkan 1.3)
        VkPipelineRenderingCreateInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;

        VkFormat const colorFormats[] = { window.swapchainFormat };
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachmentFormats = colorFormats;
        renderingInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;

        // Pipeline depth info
        VkPipelineDepthStencilStateCreateInfo depthInfo{};
        depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthInfo.depthTestEnable = VK_TRUE;
        depthInfo.depthWriteEnable = VK_TRUE;
        depthInfo.depthCompareOp = VK_COMPARE_OP_LESS;
        depthInfo.minDepthBounds = 0.f;
        depthInfo.maxDepthBounds = 1.f;

        // Create pipeline
        VkGraphicsPipelineCreateInfo pipeInfo{};
        pipeInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeInfo.pNext = &renderingInfo; // IMPORTANT! Don’t forget!

        pipeInfo.stageCount = 2; // vertex + fragment stages
        pipeInfo.pStages = stages;

        pipeInfo.pVertexInputState = &inputInfo;
        pipeInfo.pInputAssemblyState = &assemblyInfo;
        pipeInfo.pTessellationState = nullptr; // no tessellation
        pipeInfo.pViewportState = &viewportInfo;
        pipeInfo.pRasterizationState = &rasterInfo;
        pipeInfo.pMultisampleState = &samplingInfo;
        pipeInfo.pDepthStencilState = &depthInfo;
        pipeInfo.pColorBlendState = &blendInfo;
        pipeInfo.pDynamicState = nullptr; // no dynamic states

        pipeInfo.layout = pipelineLayout;
        pipeInfo.subpass = 0; // first subpass of aRenderPass

        VkPipeline pipe = VK_NULL_HANDLE;
        if (auto const res = vkCreateGraphicsPipelines( window.device, VK_NULL_HANDLE, 1,  &pipeInfo, nullptr, &pipe ); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Unable to create graphics pipeline\n"
                "vkCreateGraphicsPipelines() returned {}", rutils::toString(res)
            );
        }

        return rutils::Pipeline(window.device, pipe);
    }

    Pipeline createSkyboxPipeline(VulkanWindow const& window, VkPipelineLayout pipelineLayout) {
        // Load shader code
        auto const vShader = rutils::loadShader("shaders/compiled/skybox.vert.spv");
        auto const fShader = rutils::loadShader("shaders/compiled/skybox.frag.spv");

        VkShaderModuleCreateInfo code[2]{};
        code[0].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[0].codeSize = vShader.size()*sizeof(std::uint32_t);
        code[0].pCode = vShader.data();

        code[1].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[1].codeSize = fShader.size()*sizeof(std::uint32_t);
        code[1].pCode = fShader.data();

        // define shader stages in the pipeline
        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].pName = "main";
        stages[0].pNext = &code[0];

        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].pName = "main";
        stages[1].pNext = &code[1];

        VkVertexInputBindingDescription vertexInputs[1]{};
        VkVertexInputAttributeDescription vertexAttributes[1]{};
        VkPipelineVertexInputStateCreateInfo inputInfo{};
        VkPipelineInputAssemblyStateCreateInfo assemblyInfo{};

        // positions
        vertexInputs[0].binding = 0;
        vertexInputs[0].stride = sizeof(glm::vec3);
        vertexInputs[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        vertexAttributes[0].binding = 0; // must match binding above
        vertexAttributes[0].location = 0; // must match shader
        vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertexAttributes[0].offset = 0;

        inputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        inputInfo.vertexBindingDescriptionCount = 1; // number of vertexInputs above
        inputInfo.pVertexBindingDescriptions = vertexInputs;
        inputInfo.vertexAttributeDescriptionCount = 1; // number of vertexAttributes above
        inputInfo.pVertexAttributeDescriptions = vertexAttributes;

        // define which primitive (point, line, triangle...) the input is assembled into for rasterisation
        assemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        assemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        assemblyInfo.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        VkRect2D scissor{};
        VkPipelineViewportStateCreateInfo viewportInfo{};
        setup_viewport(window, &viewport, &scissor, &viewportInfo);

        // Define rasterization options
        VkPipelineRasterizationStateCreateInfo rasterInfo{};
        rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterInfo.depthClampEnable = VK_FALSE;
        rasterInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterInfo.depthBiasEnable = VK_FALSE;
        rasterInfo.lineWidth = 1.f; // required.

        // Define multisampling state
        VkPipelineMultisampleStateCreateInfo samplingInfo{};
        samplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        samplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // Define blend state
        // We define one blend state per color attachment - this example uses a single color attachment, so we only
        // need one. Right now, we don’t do any blending, so we can ignore most of the members.
        VkPipelineColorBlendAttachmentState blendStates[1]{};
        blendStates[0].blendEnable = VK_FALSE;
        blendStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo blendInfo{};
        blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        blendInfo.logicOpEnable = VK_FALSE;
        blendInfo.attachmentCount = 1;
        blendInfo.pAttachments = blendStates;

        // Pipeline rendering info
        // This is related to dynamic rendering (core in Vulkan 1.3)
        VkPipelineRenderingCreateInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;

        VkFormat const colorFormats[] = { window.swapchainFormat };
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachmentFormats = colorFormats;
        renderingInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;

        // Pipeline depth info
        VkPipelineDepthStencilStateCreateInfo depthInfo{};
        depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthInfo.depthTestEnable = VK_TRUE;
        depthInfo.depthWriteEnable = VK_FALSE;
        depthInfo.stencilTestEnable = VK_FALSE;
        depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depthInfo.minDepthBounds = 0.f;
        depthInfo.maxDepthBounds = 1.f;

        // Create pipeline
        VkGraphicsPipelineCreateInfo pipeInfo{};
        pipeInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeInfo.pNext = &renderingInfo; // IMPORTANT! Don’t forget!

        pipeInfo.stageCount = 2; // vertex + fragment stages
        pipeInfo.pStages = stages;

        pipeInfo.pVertexInputState = &inputInfo;
        pipeInfo.pInputAssemblyState = &assemblyInfo;
        pipeInfo.pTessellationState = nullptr; // no tessellation
        pipeInfo.pViewportState = &viewportInfo;
        pipeInfo.pRasterizationState = &rasterInfo;
        pipeInfo.pMultisampleState = &samplingInfo;
        pipeInfo.pDepthStencilState = &depthInfo;
        pipeInfo.pColorBlendState = &blendInfo;
        pipeInfo.pDynamicState = nullptr; // no dynamic states

        pipeInfo.layout = pipelineLayout;

        VkPipeline pipe = VK_NULL_HANDLE;
        if (auto const res = vkCreateGraphicsPipelines( window.device, VK_NULL_HANDLE, 1,  &pipeInfo, nullptr, &pipe ); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Unable to create graphics pipeline\n"
                "vkCreateGraphicsPipelines() returned {}", rutils::toString(res)
            );
        }

        return rutils::Pipeline(window.device, pipe);
    }

    Pipeline createAlphaPipeline(VulkanWindow const& window, VkPipelineLayout pipelineLayout) {
        // Load shader code
        // For this example, we only use the vertex and fragment shaders. Other shader stages (geometry, tessellation)
        // aren’t needed here, and as such we omit them.
        //
        // This uses a Vulkan 1.4 feature (from VK KHR maintenance5), which allows us to skip the VkShaderModule
        // creation and instead directly pass SPIR-V code to the pipeline creation.
        std::filesystem::current_path(PROJECT_BINARY_DIR);
        auto const vShader = rutils::loadShader("shaders/compiled/default.vert.spv");
        auto const fShader = rutils::loadShader("shaders/compiled/alpha.frag.spv");

        VkShaderModuleCreateInfo code[2]{};
        code[0].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[0].codeSize = vShader.size()*sizeof(std::uint32_t);
        code[0].pCode = vShader.data();

        code[1].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[1].codeSize = fShader.size()*sizeof(std::uint32_t);
        code[1].pCode = fShader.data();

        // Define shader stages in the pipeline
        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].pName = "main";
        stages[0].pNext = &code[0];

        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].pName = "main";
        stages[1].pNext = &code[1];

        VkVertexInputBindingDescription vertexInputs[3]{};
        VkVertexInputAttributeDescription vertexAttributes[3]{};
        VkPipelineVertexInputStateCreateInfo inputInfo{};
        VkPipelineInputAssemblyStateCreateInfo assemblyInfo{};
        setup_vertex_inputs(vertexInputs, vertexAttributes, &inputInfo, &assemblyInfo);

        VkViewport viewport{};
        VkRect2D scissor{};
        VkPipelineViewportStateCreateInfo viewportInfo{};
        setup_viewport(window, &viewport, &scissor, &viewportInfo);

        // Define rasterization options
        VkPipelineRasterizationStateCreateInfo rasterInfo{};
        rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterInfo.depthClampEnable = VK_FALSE;
        rasterInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterInfo.depthBiasEnable = VK_FALSE;
        rasterInfo.lineWidth = 1.f; // required.

        // Define multisampling state
        VkPipelineMultisampleStateCreateInfo samplingInfo{};
        samplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        samplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // Define blend state
        // We define one blend state per color attachment - this example uses a single color attachment, so we only
        // need one. Right now, we don’t do any blending, so we can ignore most of the members.
        VkPipelineColorBlendAttachmentState blendStates[1]{};
        blendStates[0].blendEnable = VK_TRUE; // New! Used to be VK FALSE.
        blendStates[0].colorBlendOp = VK_BLEND_OP_ADD; // New!
        blendStates[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; // New!
        blendStates[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // New!
        blendStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo blendInfo{};
        blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        blendInfo.logicOpEnable = VK_FALSE;
        blendInfo.attachmentCount = 1;
        blendInfo.pAttachments = blendStates;

        // Pipeline rendering info
        // This is related to dynamic rendering (core in Vulkan 1.3)
        VkPipelineRenderingCreateInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;

        VkFormat const colorFormats[] = { window.swapchainFormat };
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachmentFormats = colorFormats;
        renderingInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;

        // Pipeline depth info
        VkPipelineDepthStencilStateCreateInfo depthInfo{};
        depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthInfo.depthTestEnable = VK_TRUE;
        depthInfo.depthWriteEnable = VK_FALSE;
        depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depthInfo.minDepthBounds = 0.f;
        depthInfo.maxDepthBounds = 1.f;

        // Create pipeline
        VkGraphicsPipelineCreateInfo pipeInfo{};
        pipeInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeInfo.pNext = &renderingInfo; // IMPORTANT! Don’t forget!

        pipeInfo.stageCount = 2; // vertex + fragment stages
        pipeInfo.pStages = stages;

        pipeInfo.pVertexInputState = &inputInfo;
        pipeInfo.pInputAssemblyState = &assemblyInfo;
        pipeInfo.pTessellationState = nullptr; // no tessellation
        pipeInfo.pViewportState = &viewportInfo;
        pipeInfo.pRasterizationState = &rasterInfo;
        pipeInfo.pMultisampleState = &samplingInfo;
        pipeInfo.pDepthStencilState = &depthInfo;
        pipeInfo.pColorBlendState = &blendInfo;
        pipeInfo.pDynamicState = nullptr; // no dynamic states

        pipeInfo.layout = pipelineLayout;
        pipeInfo.subpass = 0; // first subpass of aRenderPass

        VkPipeline pipe = VK_NULL_HANDLE;
        if (auto const res = vkCreateGraphicsPipelines( window.device, VK_NULL_HANDLE, 1,  &pipeInfo, nullptr, &pipe ); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Unable to create graphics pipeline\n"
                "vkCreateGraphicsPipelines() returned {}", rutils::toString(res)
            );
        }

        return rutils::Pipeline(window.device, pipe);
    }


    Pipeline createDeferredGeometryPipeline(VulkanWindow const& aWindow, VkPipelineLayout aPipelineLayout) {
        // load shader code
        // we only use the vertex and fragment shaders here
        auto const vertSpirV = rutils::loadShader("shaders/compiled/default.vert.spv");
        auto const fragSpirV = rutils::loadShader("shaders/compiled/deferred_geometry.frag.spv");

        VkShaderModuleCreateInfo code[2]{};
        code[0].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[0].codeSize = vertSpirV.size() * sizeof(std::uint32_t);
        code[0].pCode = vertSpirV.data();

        code[1].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[1].codeSize = fragSpirV.size() * sizeof(std::uint32_t);
        code[1].pCode = fragSpirV.data();

        // define shader stages in the pipeline
        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].pName = "main";
        stages[0].pNext = &code[0];

        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].pName = "main";
        stages[1].pNext = &code[1];

        VkVertexInputBindingDescription vertexInputs[3]{};
        VkVertexInputAttributeDescription vertexAttributes[3]{};
        VkPipelineVertexInputStateCreateInfo inputInfo{};
        VkPipelineInputAssemblyStateCreateInfo assemblyInfo{};
        setup_vertex_inputs(vertexInputs, vertexAttributes, &inputInfo, &assemblyInfo);

        // define viewport and scissor regions
        VkViewport viewport{};
        VkRect2D scissor{};
        VkPipelineViewportStateCreateInfo viewportInfo{};
        setup_viewport(aWindow, &viewport, &scissor, &viewportInfo);

        // define rasterisation options
        VkPipelineRasterizationStateCreateInfo rasterInfo{};
        rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterInfo.depthClampEnable = VK_FALSE;
        rasterInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterInfo.depthBiasEnable = VK_FALSE;
        rasterInfo.lineWidth = 1.f; // required

        // define multisampling state
        VkPipelineMultisampleStateCreateInfo samplingInfo{};
        samplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        samplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // define blend state
        VkPipelineColorBlendAttachmentState blendStates[4]{};
        blendStates[0].blendEnable = VK_FALSE;
        blendStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        blendStates[1].blendEnable = VK_FALSE;
        blendStates[1].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        blendStates[2].blendEnable = VK_FALSE;
        blendStates[2].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        blendStates[3].blendEnable = VK_FALSE;
        blendStates[3].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo blendInfo{};
        blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        blendInfo.logicOpEnable = VK_FALSE;
        blendInfo.attachmentCount = 4;
        blendInfo.pAttachments = blendStates;

        VkPipelineDepthStencilStateCreateInfo depthInfo{};
        depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthInfo.depthTestEnable = VK_TRUE;
        depthInfo.depthWriteEnable = VK_TRUE;
        depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depthInfo.minDepthBounds = 0.f;
        depthInfo.maxDepthBounds = 1.f;

        // pipeline rendering info
        // related to dynamic rendering (core in Vulkan 1.3)
        VkPipelineRenderingCreateInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        
        VkFormat const colorFormats[4] = {VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R8G8_UNORM, VK_FORMAT_R8G8B8A8_UNORM};
        renderingInfo.colorAttachmentCount = 4;
        renderingInfo.pColorAttachmentFormats = colorFormats;
        renderingInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;

        // create pipeline
        VkGraphicsPipelineCreateInfo pipeInfo{};
        pipeInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeInfo.pNext = &renderingInfo;

        pipeInfo.stageCount = 2; // vertex and fragment stages
        pipeInfo.pStages = stages;

        pipeInfo.pVertexInputState = &inputInfo;
        pipeInfo.pInputAssemblyState = &assemblyInfo;
        pipeInfo.pTessellationState = nullptr; // no tessellation
        pipeInfo.pViewportState = &viewportInfo;
        pipeInfo.pRasterizationState = &rasterInfo;
        pipeInfo.pMultisampleState = &samplingInfo;
        pipeInfo.pDepthStencilState = &depthInfo;
        pipeInfo.pColorBlendState = &blendInfo;
        pipeInfo.pDynamicState = nullptr; // no dynamic states

        pipeInfo.layout = aPipelineLayout;
        pipeInfo.subpass = 0; // first subpass of aRenderPass

        VkPipeline pipe = VK_NULL_HANDLE;
        if (auto const res = vkCreateGraphicsPipelines(aWindow.device, VK_NULL_HANDLE, 1, &pipeInfo, nullptr, &pipe); VK_SUCCESS != res) {
            throw Kiki::FatalError("Unable to create deferred pipeline\n" "vkCreateGraphicsPipelines() returned {}", rutils::toString(res));
        }

        return Pipeline(aWindow.device, pipe);
    }

    Pipeline createDeferredGeometryAlphaPipeline(VulkanWindow const& aWindow, VkPipelineLayout aPipelineLayout) {
        // load shader code
        // we only use the vertex and fragment shaders here
        auto const vertSpirV = rutils::loadShader("shaders/compiled/default.vert.spv");
        auto const fragSpirV = rutils::loadShader("shaders/compiled/deferred_geometry_alpha.frag.spv");

        VkShaderModuleCreateInfo code[2]{};
        code[0].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[0].codeSize = vertSpirV.size() * sizeof(std::uint32_t);
        code[0].pCode = vertSpirV.data();

        code[1].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[1].codeSize = fragSpirV.size() * sizeof(std::uint32_t);
        code[1].pCode = fragSpirV.data();

        // define shader stages in the pipeline
        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].pName = "main";
        stages[0].pNext = &code[0];

        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].pName = "main";
        stages[1].pNext = &code[1];

        VkVertexInputBindingDescription vertexInputs[3]{};
        VkVertexInputAttributeDescription vertexAttributes[3]{};
        VkPipelineVertexInputStateCreateInfo inputInfo{};
        VkPipelineInputAssemblyStateCreateInfo assemblyInfo{};
        setup_vertex_inputs(vertexInputs, vertexAttributes, &inputInfo, &assemblyInfo);

        // define viewport and scissor regions
        VkViewport viewport{};
        VkRect2D scissor{};
        VkPipelineViewportStateCreateInfo viewportInfo{};
        setup_viewport(aWindow, &viewport, &scissor, &viewportInfo);

        // define rasterisation options
        VkPipelineRasterizationStateCreateInfo rasterInfo{};
        rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterInfo.depthClampEnable = VK_FALSE;
        rasterInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterInfo.cullMode = VK_CULL_MODE_NONE; // disable backface culling on objects with transparency
        rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterInfo.depthBiasEnable = VK_FALSE;
        rasterInfo.lineWidth = 1.f; // required

        // define multisampling state
        VkPipelineMultisampleStateCreateInfo samplingInfo{};
        samplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        samplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // define blend state
        // we define one blend state per colour attachment
        VkPipelineColorBlendAttachmentState blendStates[4]{};
        blendStates[0].blendEnable = VK_FALSE;
        blendStates[0].colorBlendOp = VK_BLEND_OP_ADD;
        blendStates[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blendStates[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        blendStates[1].blendEnable = VK_FALSE;
        blendStates[1].colorBlendOp = VK_BLEND_OP_ADD;
        blendStates[1].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blendStates[1].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendStates[1].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        blendStates[2].blendEnable = VK_FALSE;
        blendStates[2].colorBlendOp = VK_BLEND_OP_ADD;
        blendStates[2].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blendStates[2].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendStates[2].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        blendStates[3].blendEnable = VK_FALSE;
        blendStates[3].colorBlendOp = VK_BLEND_OP_ADD;
        blendStates[3].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blendStates[3].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendStates[3].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo blendInfo{};
        blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        blendInfo.logicOpEnable = VK_FALSE;
        blendInfo.attachmentCount = 4;
        blendInfo.pAttachments = blendStates;

        VkPipelineDepthStencilStateCreateInfo depthInfo{};
        depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthInfo.depthTestEnable = VK_TRUE;
        depthInfo.depthWriteEnable = VK_TRUE;
        depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depthInfo.minDepthBounds = 0.f;
        depthInfo.maxDepthBounds = 1.f;

        // pipeline rendering info
        // related to dynamic rendering (core in Vulkan 1.3)
        VkPipelineRenderingCreateInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        
        VkFormat const colorFormats[4] = {VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R8G8_UNORM, VK_FORMAT_R8G8B8A8_UNORM};
        renderingInfo.colorAttachmentCount = 4;
        renderingInfo.pColorAttachmentFormats = colorFormats;
        renderingInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;

        // create pipeline
        VkGraphicsPipelineCreateInfo pipeInfo{};
        pipeInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeInfo.pNext = &renderingInfo;

        pipeInfo.stageCount = 2; // vertex and fragment stages
        pipeInfo.pStages = stages;

        pipeInfo.pVertexInputState = &inputInfo;
        pipeInfo.pInputAssemblyState = &assemblyInfo;
        pipeInfo.pTessellationState = nullptr; // no tessellation
        pipeInfo.pViewportState = &viewportInfo;
        pipeInfo.pRasterizationState = &rasterInfo;
        pipeInfo.pMultisampleState = &samplingInfo;
        pipeInfo.pDepthStencilState = &depthInfo;
        pipeInfo.pColorBlendState = &blendInfo;
        pipeInfo.pDynamicState = nullptr; // no dynamic states

        pipeInfo.layout = aPipelineLayout;
        pipeInfo.subpass = 0; // first subpass of aRenderPass

        VkPipeline pipe = VK_NULL_HANDLE;
        if (auto const res = vkCreateGraphicsPipelines(aWindow.device, VK_NULL_HANDLE, 1, &pipeInfo, nullptr, &pipe); VK_SUCCESS != res) {
            throw Kiki::FatalError("Unable to create graphics pipeline\n" "vkCreateGraphicsPipelines() returned {}", rutils::toString(res));
        }

        return Pipeline(aWindow.device, pipe);
    }

    Pipeline createDeferredLightingPipeline(VulkanWindow const& aWindow, VkPipelineLayout aPipelineLayout) {
        // load shader code
        // we only use the vertex and fragment shaders here
        auto const vertSpirV = rutils::loadShader("shaders/compiled/fullscreen.vert.spv");
        auto const fragSpirV = rutils::loadShader("shaders/compiled/deferred_lighting.frag.spv");

        VkShaderModuleCreateInfo code[2]{};
        code[0].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[0].codeSize = vertSpirV.size() * sizeof(std::uint32_t);
        code[0].pCode = vertSpirV.data();

        code[1].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[1].codeSize = fragSpirV.size() * sizeof(std::uint32_t);
        code[1].pCode = fragSpirV.data();

        // define shader stages in the pipeline
        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].pName = "main";
        stages[0].pNext = &code[0];

        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].pName = "main";
        stages[1].pNext = &code[1];

        // empty as we're generating the fullscreen quad procedurally
        VkPipelineVertexInputStateCreateInfo inputInfo{};
        inputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        // define which primitive (point, line, triangle...) the input is assembled into for rasterisation
        VkPipelineInputAssemblyStateCreateInfo assemblyInfo{};
        assemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        assemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        assemblyInfo.primitiveRestartEnable = VK_FALSE;

        // define viewport and scissor regions
        VkViewport viewport{};
        VkRect2D scissor{};
        VkPipelineViewportStateCreateInfo viewportInfo{};
        setup_viewport(aWindow, &viewport, &scissor, &viewportInfo);

        // define rasterisation options
        VkPipelineRasterizationStateCreateInfo rasterInfo{};
        rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterInfo.depthClampEnable = VK_FALSE;
        rasterInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterInfo.cullMode = VK_CULL_MODE_NONE;
        rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterInfo.depthBiasEnable = VK_FALSE;
        rasterInfo.lineWidth = 1.f; // required

        // define multisampling state
        VkPipelineMultisampleStateCreateInfo samplingInfo{};
        samplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        samplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // define blend state
        VkPipelineColorBlendAttachmentState blendStates[1]{};
        blendStates[0].blendEnable = VK_FALSE;
        blendStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo blendInfo{};
        blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        blendInfo.logicOpEnable = VK_FALSE;
        blendInfo.attachmentCount = 1;
        blendInfo.pAttachments = blendStates;

        VkPipelineDepthStencilStateCreateInfo depthInfo{};
        depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthInfo.depthTestEnable = VK_FALSE;
        depthInfo.depthWriteEnable = VK_FALSE;
        depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depthInfo.minDepthBounds = 0.f;
        depthInfo.maxDepthBounds = 1.f;

        // pipeline rendering info
        // related to dynamic rendering (core in Vulkan 1.3)
        VkPipelineRenderingCreateInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        
        VkFormat const colorFormats[] = {aWindow.swapchainFormat};
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachmentFormats = colorFormats;
        renderingInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;

        // create pipeline
        VkGraphicsPipelineCreateInfo pipeInfo{};
        pipeInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeInfo.pNext = &renderingInfo;

        pipeInfo.stageCount = 2;
        pipeInfo.pStages = stages;

        pipeInfo.pVertexInputState = &inputInfo;
        pipeInfo.pInputAssemblyState = &assemblyInfo;
        pipeInfo.pTessellationState = nullptr; // no tessellation
        pipeInfo.pViewportState = &viewportInfo;
        pipeInfo.pRasterizationState = &rasterInfo;
        pipeInfo.pMultisampleState = &samplingInfo;
        pipeInfo.pDepthStencilState = &depthInfo;
        pipeInfo.pColorBlendState = &blendInfo;
        pipeInfo.pDynamicState = nullptr; // no dynamic states

        pipeInfo.layout = aPipelineLayout;
        pipeInfo.subpass = 0; // first subpass of aRenderPass

        VkPipeline pipe = VK_NULL_HANDLE;
        if (auto const res = vkCreateGraphicsPipelines(aWindow.device, VK_NULL_HANDLE, 1, &pipeInfo, nullptr, &pipe); VK_SUCCESS != res) {
            throw Kiki::FatalError("Unable to create graphics pipeline\n" "vkCreateGraphicsPipelines() returned {}", rutils::toString(res));
        }

        return Pipeline(aWindow.device, pipe);
    }
}
