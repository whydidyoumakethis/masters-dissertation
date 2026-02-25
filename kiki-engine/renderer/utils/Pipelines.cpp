#include "Pipelines.hpp"

#include "ToString.hpp"
#include "Shaders.hpp"
#include "../../logging/FatalError.hpp"

namespace rutils {
    PipelineLayout createPipelineLayout(VulkanWindow const& window) {
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = 0;
        layoutInfo.pSetLayouts = nullptr;
        layoutInfo.pushConstantRangeCount = 0;
        layoutInfo.pPushConstantRanges = nullptr;

        VkPipelineLayout layout = VK_NULL_HANDLE;
        if (auto const res = vkCreatePipelineLayout( window.device, &layoutInfo, nullptr, &layout ); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Unable to create pipeline layout\n"
                "vkCreatePipelineLayout() returned {}", rutils::toString(res)
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
        auto const vShader = rutils::loadShader("shaders/triangle.vert.spv"); // TODO: update file path, temporarily hardocded
        auto const fShader = rutils::loadShader("shaders/triangle.frag.spv");

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

        // For now, we don’t have any vertex input attributes - the geometry is generated/defined in the vertex shader.
        VkPipelineVertexInputStateCreateInfo inputInfo{};
        inputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        // Define which primitive (point, line, triangle, ...) the input is assembled into for rasterization.
        VkPipelineInputAssemblyStateCreateInfo assemblyInfo{};
        assemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        assemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        assemblyInfo.primitiveRestartEnable = VK_FALSE;

        // Define viewport and scissor regions
        VkViewport viewport{};
        viewport.x = 0.f;
        viewport.y = 0.f;
        viewport.width = float( window.swapchainExtent.width );
        viewport.height = float( window.swapchainExtent.height);
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;

        VkRect2D scissor{};
        scissor.offset = VkOffset2D{ 0, 0 };
        scissor.extent = VkExtent2D{ window.swapchainExtent.width, window.swapchainExtent.height };

        VkPipelineViewportStateCreateInfo viewportInfo{};
        viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportInfo.viewportCount = 1;
        viewportInfo.pViewports = &viewport;
        viewportInfo.scissorCount = 1;
        viewportInfo.pScissors = &scissor;

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
        pipeInfo.pDepthStencilState = nullptr; // no depth or stencil buffers
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
}