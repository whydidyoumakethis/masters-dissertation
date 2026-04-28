#include "Pipelines.hpp"

#include "ToString.hpp"
#include "Shaders.hpp"
#include "../../logging/FatalError.hpp"
#include "renderer/RenderManager.hpp"

#include <print>
#include <iostream>

namespace rutils {
    Pipelines createAllPipelines(VulkanWindow const& window, PipelineLayouts const& pipelineLayouts) {
        Pipelines pipelines;

        std::filesystem::current_path(PROJECT_SHADER_PATH);

        pipelines.pbr = createPipeline(window, pipelineLayouts.pbrPipelineLayout.handle);
        pipelines.pbr_alpha = createAlphaPipeline(window, pipelineLayouts.pbrPipelineLayout.handle);
        pipelines.deferred_geometry = createDeferredGeometryPipeline(window, pipelineLayouts.pbrPipelineLayout.handle);
        pipelines.deferred_geometry_alpha = createDeferredGeometryAlphaPipeline(window, pipelineLayouts.pbrPipelineLayout.handle);
        pipelines.deferred_lighting = createDeferredLightingPipeline(window, pipelineLayouts.deferredPipelineLayout.handle);
        pipelines.fxaa = createFXAAPipeline(window, pipelineLayouts.postprocessPipelineLayout.handle);
        pipelines.ssr = createSSRPipeline(window, pipelineLayouts.postprocessPipelineLayout.handle);
        pipelines.ssao = createSSAOPipeline(window, pipelineLayouts.ssaoPipelineLayout.handle);
        pipelines.ssao_hblur = createSSAOHBlurPipeline(window, pipelineLayouts.ssaoBlurPipelineLayout.handle);
        pipelines.ssao_blurred = createSSAOBlurredPipeline(window, pipelineLayouts.ssaoBlurPipelineLayout.handle);
        pipelines.tonemap = createTonemapPipeline(window, pipelineLayouts.tonemapPipelineLayout.handle);
        pipelines.shadowMap = createShadowMapPipeline(window, pipelineLayouts.shadowMapPipelineLayout.handle);

        pipelines.interfaceShape = createInterfacePipeline(window, pipelineLayouts.interfaceShapeLayout.handle, Kiki::RenderManager::get().shaderPaths.interface_shape_f);
        pipelines.interfaceText = createInterfacePipeline(window, pipelineLayouts.interfaceTextLayout.handle, Kiki::RenderManager::get().shaderPaths.interface_text_f);

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

        // tangents
        vertexInputs[3].binding = 3;
        vertexInputs[3].stride = sizeof(glm::vec4);
        vertexInputs[3].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
      
        // --- bone IDs ---
        vertexInputs[4].binding = 4;
        vertexInputs[4].stride = sizeof(glm::ivec4);
        vertexInputs[4].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        // --- weights ---
        vertexInputs[5].binding = 5;
        vertexInputs[5].stride = sizeof(glm::vec4);
        vertexInputs[5].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;



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

        vertexAttributes[3].binding = 3;
        vertexAttributes[3].location = 3; // must match shader
        vertexAttributes[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        vertexAttributes[3].offset = 0;
      
        // bone IDs
        vertexAttributes[4].binding = 4;
        vertexAttributes[4].location = 4;
        vertexAttributes[4].format = VK_FORMAT_R32G32B32A32_SINT;
        vertexAttributes[4].offset = 0;

        // weights
        vertexAttributes[5].binding = 5;
        vertexAttributes[5].location = 5;
        vertexAttributes[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        vertexAttributes[5].offset = 0;

        inputInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        inputInfo->vertexBindingDescriptionCount = 6; // number of vertexInputs above
        inputInfo->pVertexBindingDescriptions = vertexInputs;
        inputInfo->vertexAttributeDescriptionCount = 6; // number of vertexAttributes above
        inputInfo->pVertexAttributeDescriptions = vertexAttributes;

        // define which primitive (point, line, triangle...) the input is assembled into for rasterisation
        assemblyInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        assemblyInfo->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        assemblyInfo->primitiveRestartEnable = VK_FALSE;
    }

    PipelineLayout createPipelineLayout(VulkanWindow const& window, VkDescriptorSetLayout sceneLayout, VkDescriptorSetLayout materialLayout, VkDescriptorSetLayout animationLayout)
    {
        VkDescriptorSetLayout layouts[] = {
            // Order must match the set = N in the shaders
            sceneLayout, // set 0
			materialLayout, // set 1
            animationLayout
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

    PipelineLayout createPostProcessingPipelineLayout(VulkanWindow const& window, VkDescriptorSetLayout sceneLayout, VkDescriptorSetLayout postProcessingLayout) {
        VkDescriptorSetLayout layouts[] = {
            // Order must match the set = N in the shaders
            sceneLayout, // set 0
            postProcessingLayout // set 1
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

    PipelineLayout createSSAOPipelineLayout(VulkanWindow const& window, VkDescriptorSetLayout sceneLayout, VkDescriptorSetLayout ssaoLayout) {
        VkDescriptorSetLayout layouts[] = {
            // Order must match the set = N in the shaders
            sceneLayout, // set 0
            ssaoLayout // set 1
        };

        VkPushConstantRange pushRange{};
        pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushRange.offset = 0;
        pushRange.size = sizeof(SSAOSettings);

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

    PipelineLayout createSSAOBlurPipelineLayout(VulkanWindow const& window, VkDescriptorSetLayout ssaoBlurLayout) {
        VkDescriptorSetLayout layouts[] = {
            // Order must match the set = N in the shaders
            ssaoBlurLayout // set 0
        };

        VkPushConstantRange pushRange{};
        pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushRange.offset = 0;
        pushRange.size = sizeof(SSAOSettings);

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

    PipelineLayout createTonemapPipelineLayout(VulkanWindow const& window, VkDescriptorSetLayout tonemapLayout) {
        VkDescriptorSetLayout layouts[] = {
            // Order must match the set = N in the shaders
            tonemapLayout // set 0
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

    PipelineLayout createShadowMapPipelineLayout(VulkanWindow const& window, VkDescriptorSetLayout shadowMatrixLayout, VkDescriptorSetLayout boneLayout) {
        VkDescriptorSetLayout layouts[] = {
            // Order must match the set = N in the shaders
            shadowMatrixLayout, // set 0
            boneLayout // set 1
        };

        VkPushConstantRange pushRange{};
        pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushRange.offset = 0;
        pushRange.size = sizeof(ShadowData);

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

    void createInterfacePipelineLayout(VulkanWindow const& window, VkDescriptorSetLayout interfaceLayout, VkDescriptorSetLayout textDescriptorLayout, PipelineLayouts* layouts) {
        VkDescriptorSetLayout shapeLayout[] = {
            interfaceLayout
        };

        VkPushConstantRange shapeConstants{};
        shapeConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        shapeConstants.offset = 0;
        shapeConstants.size = sizeof(ShapeData);

        VkPipelineLayoutCreateInfo shapeLayoutInfo{};
        shapeLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        shapeLayoutInfo.setLayoutCount = 1;
        shapeLayoutInfo.pSetLayouts = shapeLayout;
        shapeLayoutInfo.pushConstantRangeCount = 1;
        shapeLayoutInfo.pPushConstantRanges = &shapeConstants;

        VkPipelineLayout tempShapeLayout = VK_NULL_HANDLE;
        if (auto const res = vkCreatePipelineLayout(window.device, &shapeLayoutInfo, nullptr, &tempShapeLayout); VK_SUCCESS != res) {
            throw Kiki::FatalError("Unable to create pipeline layout\n"
                "vkCreatePipelineLayout() returned {}", toString(res)
            );
        }

        layouts->interfaceShapeLayout = rutils::PipelineLayout(window.device, tempShapeLayout);

        VkDescriptorSetLayout textLayout[] = {
            interfaceLayout,
            textDescriptorLayout
        };

        VkPushConstantRange textConstants{};
        textConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        textConstants.offset = 0;
        textConstants.size = sizeof(ShapeData);

        VkPipelineLayoutCreateInfo textLayoutInfo{};
        textLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        textLayoutInfo.setLayoutCount = 2;
        textLayoutInfo.pSetLayouts = textLayout;
        textLayoutInfo.pushConstantRangeCount = 1;
        textLayoutInfo.pPushConstantRanges = &textConstants;

        VkPipelineLayout tempTextLayout = VK_NULL_HANDLE;
        if (auto const res = vkCreatePipelineLayout(window.device, &textLayoutInfo, nullptr, &tempTextLayout); VK_SUCCESS != res) {
            throw Kiki::FatalError("Unable to create pipeline layout\n"
                "vkCreatePipelineLayout() returned {}", toString(res)
            );
        }

        layouts->interfaceTextLayout = rutils::PipelineLayout(window.device, tempTextLayout);
    }

    Pipeline createPipeline(VulkanWindow const& window, VkPipelineLayout pipelineLayout) {
        // Load shader code
        // For this example, we only use the vertex and fragment shaders. Other shader stages (geometry, tessellation)
        // aren’t needed here, and as such we omit them.
        //
        // This uses a Vulkan 1.4 feature (from VK KHR maintenance5), which allows us to skip the VkShaderModule
        // creation and instead directly pass SPIR-V code to the pipeline creation.
        auto const vShader = rutils::loadShader(Kiki::RenderManager::get().shaderPaths.pbr_v.string().c_str());
        auto const fShader = rutils::loadShader(Kiki::RenderManager::get().shaderPaths.pbr_f.string().c_str());

        VkShaderModuleCreateInfo code[2]{};
        code[0].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[0].codeSize = vShader.size()*sizeof(std::uint32_t);
        code[0].pCode = vShader.data();

        code[1].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[1].codeSize = fShader.size()*sizeof(std::uint32_t);
        code[1].pCode = fShader.data();

        // Define shader stages in the pipeline
        VkPipelineShaderStageCreateInfo stages[2]{};
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

        VkVertexInputBindingDescription vertexInputs[6]{};
        VkVertexInputAttributeDescription vertexAttributes[6]{};
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

        vkDestroyShaderModule(window.device, vertModule, nullptr);
        vkDestroyShaderModule(window.device, fragModule, nullptr);

        return rutils::Pipeline(window.device, pipe);
    }

    Pipeline createAlphaPipeline(VulkanWindow const& window, VkPipelineLayout pipelineLayout) {
        // Load shader code
        // For this example, we only use the vertex and fragment shaders. Other shader stages (geometry, tessellation)
        // aren’t needed here, and as such we omit them.
        //
        // This uses a Vulkan 1.4 feature (from VK KHR maintenance5), which allows us to skip the VkShaderModule
        // creation and instead directly pass SPIR-V code to the pipeline creation.
        auto const vShader = rutils::loadShader(Kiki::RenderManager::get().shaderPaths.pbr_alpha_v.string().c_str());
        auto const fShader = rutils::loadShader(Kiki::RenderManager::get().shaderPaths.pbr_alpha_f.string().c_str());

        VkShaderModuleCreateInfo code[2]{};
        code[0].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[0].codeSize = vShader.size()*sizeof(std::uint32_t);
        code[0].pCode = vShader.data();

        code[1].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[1].codeSize = fShader.size()*sizeof(std::uint32_t);
        code[1].pCode = fShader.data();

        // Define shader stages in the pipeline
        VkPipelineShaderStageCreateInfo stages[2]{};
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


        VkVertexInputBindingDescription vertexInputs[6]{};
        VkVertexInputAttributeDescription vertexAttributes[6]{};
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
        rasterInfo.cullMode = VK_CULL_MODE_NONE;
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

        vkDestroyShaderModule(window.device, vertModule, nullptr);
        vkDestroyShaderModule(window.device, fragModule, nullptr);

        return rutils::Pipeline(window.device, pipe);
    }


    Pipeline createDeferredGeometryPipeline(VulkanWindow const& aWindow, VkPipelineLayout aPipelineLayout) {
        // load shader code
        // we only use the vertex and fragment shaders here
        auto const vShader = rutils::loadShader(Kiki::RenderManager::get().shaderPaths.deferred_geometry_v.string().c_str());
        auto const fShader = rutils::loadShader(Kiki::RenderManager::get().shaderPaths.deferred_geometry_f.string().c_str());

        VkShaderModuleCreateInfo code[2]{};
        code[0].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[0].codeSize = vShader.size()*sizeof(std::uint32_t);
        code[0].pCode = vShader.data();

        code[1].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[1].codeSize = fShader.size()*sizeof(std::uint32_t);
        code[1].pCode = fShader.data();

        // Define shader stages in the pipeline
        VkPipelineShaderStageCreateInfo stages[2]{};
        VkShaderModule vertModule;
        VkShaderModule fragModule;

        vkCreateShaderModule(aWindow.device, &code[0], nullptr, &vertModule);
        vkCreateShaderModule(aWindow.device, &code[1], nullptr, &fragModule);

        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vertModule;
        stages[0].pName = "main";

        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = fragModule;
        stages[1].pName = "main";

        VkVertexInputBindingDescription vertexInputs[6]{};
        VkVertexInputAttributeDescription vertexAttributes[6]{};
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
        
        VkFormat const colorFormats[4] = {VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R8G8_UNORM, VK_FORMAT_R16G16B16A16_SFLOAT};
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

        vkDestroyShaderModule(aWindow.device, vertModule, nullptr);
        vkDestroyShaderModule(aWindow.device, fragModule, nullptr);

        return Pipeline(aWindow.device, pipe);
    }

    Pipeline createDeferredGeometryAlphaPipeline(VulkanWindow const& aWindow, VkPipelineLayout aPipelineLayout) {
        // load shader code
        // we only use the vertex and fragment shaders here
        auto const vShader = rutils::loadShader(Kiki::RenderManager::get().shaderPaths.deferred_geometry_alpha_v.string().c_str());
        auto const fShader = rutils::loadShader(Kiki::RenderManager::get().shaderPaths.deferred_geometry_alpha_f.string().c_str());

        VkShaderModuleCreateInfo code[2]{};
        code[0].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[0].codeSize = vShader.size()*sizeof(std::uint32_t);
        code[0].pCode = vShader.data();

        code[1].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[1].codeSize = fShader.size()*sizeof(std::uint32_t);
        code[1].pCode = fShader.data();

        // Define shader stages in the pipeline
        VkPipelineShaderStageCreateInfo stages[2]{};
        VkShaderModule vertModule;
        VkShaderModule fragModule;

        vkCreateShaderModule(aWindow.device, &code[0], nullptr, &vertModule);
        vkCreateShaderModule(aWindow.device, &code[1], nullptr, &fragModule);

        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vertModule;
        stages[0].pName = "main";

        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = fragModule;
        stages[1].pName = "main";

        VkVertexInputBindingDescription vertexInputs[6]{};
        VkVertexInputAttributeDescription vertexAttributes[6]{};
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
        
        VkFormat const colorFormats[4] = {VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R8G8_UNORM, VK_FORMAT_R16G16B16A16_SFLOAT};
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

        vkDestroyShaderModule(aWindow.device, vertModule, nullptr);
        vkDestroyShaderModule(aWindow.device, fragModule, nullptr);

        return Pipeline(aWindow.device, pipe);
    }

    Pipeline createDeferredLightingPipeline(VulkanWindow const& aWindow, VkPipelineLayout aPipelineLayout) {
        // load shader code
        // we only use the vertex and fragment shaders here
        auto const vShader = rutils::loadShader(Kiki::RenderManager::get().shaderPaths.deferred_lighting_v.string().c_str());
        auto const fShader = rutils::loadShader(Kiki::RenderManager::get().shaderPaths.deferred_lighting_f.string().c_str());

        VkShaderModuleCreateInfo code[2]{};
        code[0].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[0].codeSize = vShader.size()*sizeof(std::uint32_t);
        code[0].pCode = vShader.data();

        code[1].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[1].codeSize = fShader.size()*sizeof(std::uint32_t);
        code[1].pCode = fShader.data();

        // Define shader stages in the pipeline
        VkPipelineShaderStageCreateInfo stages[2]{};
        VkShaderModule vertModule;
        VkShaderModule fragModule;

        vkCreateShaderModule(aWindow.device, &code[0], nullptr, &vertModule);
        vkCreateShaderModule(aWindow.device, &code[1], nullptr, &fragModule);

        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vertModule;
        stages[0].pName = "main";

        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = fragModule;
        stages[1].pName = "main";

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
        
        VkFormat const colorFormats[] = {aWindow.hdrFormat};
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

        vkDestroyShaderModule(aWindow.device, vertModule, nullptr);
        vkDestroyShaderModule(aWindow.device, fragModule, nullptr);

        return Pipeline(aWindow.device, pipe);
    }

    Pipeline createShadowMapPipeline(VulkanWindow const& aWindow, VkPipelineLayout aPipelineLayout) {
        // load shader code
        // we only use the vertex and fragment shaders here
        auto const vShader = rutils::loadShader(Kiki::RenderManager::get().shaderPaths.shadowmap_v.string().c_str());

        VkShaderModuleCreateInfo code[1]{};
        code[0].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[0].codeSize = vShader.size()*sizeof(std::uint32_t);
        code[0].pCode = vShader.data();

        // Define shader stages in the pipeline
        VkPipelineShaderStageCreateInfo stages[1]{};
        VkShaderModule vertModule;

        vkCreateShaderModule(aWindow.device, &code[0], nullptr, &vertModule);

        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vertModule;
        stages[0].pName = "main";

        VkVertexInputBindingDescription vertexInputs[6]{};
        VkVertexInputAttributeDescription vertexAttributes[6]{};
        VkPipelineVertexInputStateCreateInfo inputInfo{};
        VkPipelineInputAssemblyStateCreateInfo assemblyInfo{};
        setup_vertex_inputs(vertexInputs, vertexAttributes, &inputInfo, &assemblyInfo);

        // define viewport and scissor regions
        VkViewport viewport{};
        VkRect2D scissor{};
        VkPipelineViewportStateCreateInfo viewportInfo{};

        // define viewport and scissor regions
        viewport.x = 0.f;
        viewport.y = 0.f;
        viewport.width = float(1024);
        viewport.height = float(1024);
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;

        scissor.offset = VkOffset2D{0, 0};
        scissor.extent = VkExtent2D{1024, 1024};

        viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportInfo.viewportCount = 1;
        viewportInfo.pViewports = &viewport;
        viewportInfo.scissorCount = 1;
        viewportInfo.pScissors = &scissor;

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

        VkPipelineDepthStencilStateCreateInfo depthInfo{};
        depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthInfo.depthTestEnable = VK_TRUE;
        depthInfo.depthWriteEnable = VK_TRUE;
        depthInfo.depthBoundsTestEnable = VK_FALSE;
        depthInfo.stencilTestEnable = VK_FALSE;
        depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depthInfo.minDepthBounds = 0.f;
        depthInfo.maxDepthBounds = 1.f;

        // pipeline rendering info
        // related to dynamic rendering (core in Vulkan 1.3)
        VkPipelineRenderingCreateInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        
        renderingInfo.colorAttachmentCount = 0;
        renderingInfo.pColorAttachmentFormats = nullptr;
        renderingInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;

        // create pipeline
        VkGraphicsPipelineCreateInfo pipeInfo{};
        pipeInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeInfo.pNext = &renderingInfo;

        pipeInfo.stageCount = 1; // vertex stage
        pipeInfo.pStages = stages;

        pipeInfo.pVertexInputState = &inputInfo;
        pipeInfo.pInputAssemblyState = &assemblyInfo;
        pipeInfo.pTessellationState = nullptr; // no tessellation
        pipeInfo.pViewportState = &viewportInfo;
        pipeInfo.pRasterizationState = &rasterInfo;
        pipeInfo.pMultisampleState = &samplingInfo;
        pipeInfo.pDepthStencilState = &depthInfo;
        pipeInfo.pColorBlendState = nullptr;
        pipeInfo.pDynamicState = nullptr; // no dynamic states

        pipeInfo.layout = aPipelineLayout;
        pipeInfo.subpass = 0; // first subpass of aRenderPass

        VkPipeline pipe = VK_NULL_HANDLE;
        if (auto const res = vkCreateGraphicsPipelines(aWindow.device, VK_NULL_HANDLE, 1, &pipeInfo, nullptr, &pipe); VK_SUCCESS != res) {
            throw Kiki::FatalError("Unable to create shadow map pipeline\n" "vkCreateGraphicsPipelines() returned {}", rutils::toString(res));
        }

        vkDestroyShaderModule(aWindow.device, vertModule, nullptr);

        return Pipeline(aWindow.device, pipe);
    }

    Pipeline createFXAAPipeline(VulkanWindow const& aWindow, VkPipelineLayout aPipelineLayout) {
        // load shader code
        // we only use the vertex and fragment shaders here
        auto const vShader = rutils::loadShader(Kiki::RenderManager::get().shaderPaths.deferred_lighting_v.string().c_str());
        auto const fShader = rutils::loadShader(Kiki::RenderManager::get().shaderPaths.fxaa_f.string().c_str());

        VkShaderModuleCreateInfo code[2]{};
        code[0].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[0].codeSize = vShader.size()*sizeof(std::uint32_t);
        code[0].pCode = vShader.data();

        code[1].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[1].codeSize = fShader.size()*sizeof(std::uint32_t);
        code[1].pCode = fShader.data();

        // Define shader stages in the pipeline
        VkPipelineShaderStageCreateInfo stages[2]{};
        VkShaderModule vertModule;
        VkShaderModule fragModule;

        vkCreateShaderModule(aWindow.device, &code[0], nullptr, &vertModule);
        vkCreateShaderModule(aWindow.device, &code[1], nullptr, &fragModule);

        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vertModule;
        stages[0].pName = "main";

        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = fragModule;
        stages[1].pName = "main";

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

        vkDestroyShaderModule(aWindow.device, vertModule, nullptr);
        vkDestroyShaderModule(aWindow.device, fragModule, nullptr);

        return Pipeline(aWindow.device, pipe);
    }

    Pipeline createTonemapPipeline(VulkanWindow const& aWindow, VkPipelineLayout aPipelineLayout) {
        // load shader code
        // we only use the vertex and fragment shaders here
        auto const vShader = rutils::loadShader(Kiki::RenderManager::get().shaderPaths.deferred_lighting_v.string().c_str());
        auto const fShader = rutils::loadShader(Kiki::RenderManager::get().shaderPaths.tonemap_f.string().c_str());

        VkShaderModuleCreateInfo code[2]{};
        code[0].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[0].codeSize = vShader.size()*sizeof(std::uint32_t);
        code[0].pCode = vShader.data();

        code[1].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[1].codeSize = fShader.size()*sizeof(std::uint32_t);
        code[1].pCode = fShader.data();

        // Define shader stages in the pipeline
        VkPipelineShaderStageCreateInfo stages[2]{};
        VkShaderModule vertModule;
        VkShaderModule fragModule;

        vkCreateShaderModule(aWindow.device, &code[0], nullptr, &vertModule);
        vkCreateShaderModule(aWindow.device, &code[1], nullptr, &fragModule);

        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vertModule;
        stages[0].pName = "main";

        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = fragModule;
        stages[1].pName = "main";

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
        renderingInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;

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

        vkDestroyShaderModule(aWindow.device, vertModule, nullptr);
        vkDestroyShaderModule(aWindow.device, fragModule, nullptr);

        return Pipeline(aWindow.device, pipe);
    }

    Pipeline createSSAOPipeline(VulkanWindow const& aWindow, VkPipelineLayout aPipelineLayout) {
        // load shader code
        // we only use the vertex and fragment shaders here
        auto const vShader = rutils::loadShader(Kiki::RenderManager::get().shaderPaths.deferred_lighting_v.string().c_str());
        auto const fShader = rutils::loadShader(Kiki::RenderManager::get().shaderPaths.ssao_f.string().c_str());

        VkShaderModuleCreateInfo code[2]{};
        code[0].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[0].codeSize = vShader.size()*sizeof(std::uint32_t);
        code[0].pCode = vShader.data();

        code[1].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[1].codeSize = fShader.size()*sizeof(std::uint32_t);
        code[1].pCode = fShader.data();

        // Define shader stages in the pipeline
        VkPipelineShaderStageCreateInfo stages[2]{};
        VkShaderModule vertModule;
        VkShaderModule fragModule;

        vkCreateShaderModule(aWindow.device, &code[0], nullptr, &vertModule);
        vkCreateShaderModule(aWindow.device, &code[1], nullptr, &fragModule);

        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vertModule;
        stages[0].pName = "main";

        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = fragModule;
        stages[1].pName = "main";

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
        blendStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT;

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
        
        VkFormat const colorFormats[] = {VK_FORMAT_R16_SFLOAT};
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachmentFormats = colorFormats;
        renderingInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;

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

        vkDestroyShaderModule(aWindow.device, vertModule, nullptr);
        vkDestroyShaderModule(aWindow.device, fragModule, nullptr);

        return Pipeline(aWindow.device, pipe);
    }

    Pipeline createSSAOHBlurPipeline(VulkanWindow const& aWindow, VkPipelineLayout aPipelineLayout) {
        // load shader code
        // we only use the vertex and fragment shaders here
        auto const vShader = rutils::loadShader(Kiki::RenderManager::get().shaderPaths.deferred_lighting_v.string().c_str());
        auto const fShader = rutils::loadShader(Kiki::RenderManager::get().shaderPaths.ssao_hblur_f.string().c_str());

        VkShaderModuleCreateInfo code[2]{};
        code[0].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[0].codeSize = vShader.size()*sizeof(std::uint32_t);
        code[0].pCode = vShader.data();

        code[1].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[1].codeSize = fShader.size()*sizeof(std::uint32_t);
        code[1].pCode = fShader.data();

        // Define shader stages in the pipeline
        VkPipelineShaderStageCreateInfo stages[2]{};
        VkShaderModule vertModule;
        VkShaderModule fragModule;

        vkCreateShaderModule(aWindow.device, &code[0], nullptr, &vertModule);
        vkCreateShaderModule(aWindow.device, &code[1], nullptr, &fragModule);

        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vertModule;
        stages[0].pName = "main";

        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = fragModule;
        stages[1].pName = "main";

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
        blendStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT;

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
        
        VkFormat const colorFormats[] = {VK_FORMAT_R16_SFLOAT};
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachmentFormats = colorFormats;
        renderingInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;

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

        vkDestroyShaderModule(aWindow.device, vertModule, nullptr);
        vkDestroyShaderModule(aWindow.device, fragModule, nullptr);

        return Pipeline(aWindow.device, pipe);
    }

    Pipeline createSSAOBlurredPipeline(VulkanWindow const& aWindow, VkPipelineLayout aPipelineLayout) {
        // load shader code
        // we only use the vertex and fragment shaders here
        auto const vShader = rutils::loadShader(Kiki::RenderManager::get().shaderPaths.deferred_lighting_v.string().c_str());
        auto const fShader = rutils::loadShader(Kiki::RenderManager::get().shaderPaths.ssao_vblur_f.string().c_str());

        VkShaderModuleCreateInfo code[2]{};
        code[0].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[0].codeSize = vShader.size()*sizeof(std::uint32_t);
        code[0].pCode = vShader.data();

        code[1].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[1].codeSize = fShader.size()*sizeof(std::uint32_t);
        code[1].pCode = fShader.data();

        // Define shader stages in the pipeline
        VkPipelineShaderStageCreateInfo stages[2]{};
        VkShaderModule vertModule;
        VkShaderModule fragModule;

        vkCreateShaderModule(aWindow.device, &code[0], nullptr, &vertModule);
        vkCreateShaderModule(aWindow.device, &code[1], nullptr, &fragModule);

        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vertModule;
        stages[0].pName = "main";

        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = fragModule;
        stages[1].pName = "main";

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
        blendStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT;

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
        
        VkFormat const colorFormats[] = {VK_FORMAT_R16_SFLOAT};
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachmentFormats = colorFormats;
        renderingInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;

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

        vkDestroyShaderModule(aWindow.device, vertModule, nullptr);
        vkDestroyShaderModule(aWindow.device, fragModule, nullptr);

        return Pipeline(aWindow.device, pipe);
    }

    Pipeline createSSRPipeline(VulkanWindow const& aWindow, VkPipelineLayout aPipelineLayout) {
        // load shader code
        // we only use the vertex and fragment shaders here
        auto const vShader = rutils::loadShader(Kiki::RenderManager::get().shaderPaths.deferred_lighting_v.string().c_str());
        auto const fShader = rutils::loadShader(Kiki::RenderManager::get().shaderPaths.ssr_f.string().c_str());

        VkShaderModuleCreateInfo code[2]{};
        code[0].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[0].codeSize = vShader.size()*sizeof(std::uint32_t);
        code[0].pCode = vShader.data();

        code[1].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[1].codeSize = fShader.size()*sizeof(std::uint32_t);
        code[1].pCode = fShader.data();

        // Define shader stages in the pipeline
        VkPipelineShaderStageCreateInfo stages[2]{};
        VkShaderModule vertModule;
        VkShaderModule fragModule;

        vkCreateShaderModule(aWindow.device, &code[0], nullptr, &vertModule);
        vkCreateShaderModule(aWindow.device, &code[1], nullptr, &fragModule);

        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vertModule;
        stages[0].pName = "main";

        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = fragModule;
        stages[1].pName = "main";

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
        
        VkFormat const colorFormats[] = {aWindow.hdrFormat};
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

        vkDestroyShaderModule(aWindow.device, vertModule, nullptr);
        vkDestroyShaderModule(aWindow.device, fragModule, nullptr);

        return Pipeline(aWindow.device, pipe);
    }
    

    Pipeline createInterfacePipeline(VulkanWindow const& window, VkPipelineLayout layout, std::filesystem::path fShaderPath) {
        auto const vShader = rutils::loadShader(Kiki::RenderManager::get().shaderPaths.interface_v.string().c_str());
        auto const fShader = rutils::loadShader(fShaderPath.string().c_str());

        VkShaderModuleCreateInfo code[2]{};
        code[0].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[0].codeSize = vShader.size() * sizeof(std::uint32_t);
        code[0].pCode = vShader.data();

        code[1].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        code[1].codeSize = fShader.size() * sizeof(std::uint32_t);
        code[1].pCode = fShader.data();

        // Define shader stages in the pipeline
        VkPipelineShaderStageCreateInfo stages[2]{};
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

        VkVertexInputBindingDescription vertexInputs[1]{};
        vertexInputs[0].binding = 0;
        vertexInputs[0].stride = sizeof(glm::vec2) + sizeof(glm::vec2);
        vertexInputs[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription vertexAttributes[2]{};
        vertexAttributes[0].binding = 0;
        vertexAttributes[0].location = 0; 
        vertexAttributes[0].format = VK_FORMAT_R32G32_SFLOAT;
        vertexAttributes[0].offset = 0;

        vertexAttributes[1].binding = 0;
        vertexAttributes[1].location = 1;
        vertexAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
        vertexAttributes[1].offset = sizeof(glm::vec2);

        VkPipelineVertexInputStateCreateInfo inputInfo{};
        inputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        inputInfo.vertexBindingDescriptionCount = 1;
        inputInfo.pVertexBindingDescriptions = vertexInputs;
        inputInfo.vertexAttributeDescriptionCount = 2;
        inputInfo.pVertexAttributeDescriptions = vertexAttributes;

        VkPipelineInputAssemblyStateCreateInfo assemblyInfo{};
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
        rasterInfo.cullMode = VK_CULL_MODE_NONE;
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
        renderingInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;

        // Pipeline depth info
        VkPipelineDepthStencilStateCreateInfo depthInfo{};
        depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthInfo.depthTestEnable = VK_FALSE;
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

        pipeInfo.layout = layout;
        pipeInfo.subpass = 0; // first subpass of aRenderPass

        VkPipeline pipe = VK_NULL_HANDLE;
        if (auto const res = vkCreateGraphicsPipelines(window.device, VK_NULL_HANDLE, 1, &pipeInfo, nullptr, &pipe); VK_SUCCESS != res) {
            throw Kiki::FatalError("Unable to create graphics pipeline\n"
                "vkCreateGraphicsPipelines() returned {}", rutils::toString(res)
            );
        }

        vkDestroyShaderModule(window.device, vertModule, nullptr);
        vkDestroyShaderModule(window.device, fragModule, nullptr);

        return rutils::Pipeline(window.device, pipe);
    }
}
