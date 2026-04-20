#include "Commands.hpp"

#include "Synchronisation.hpp"
#include "ToString.hpp"
#include "Pipelines.hpp"
#include "Components/TransparencyComponent.hpp"
#include "Components/ColourComponent.hpp"
#include "../../logging/FatalError.hpp"
#include "../RenderManager.hpp"
#include "../SceneManager.hpp"

#include "../../ECS/World.h"

#include <iostream>


namespace rutils {
    CommandPool createCommandPool(VulkanWindow const& window, VkCommandPoolCreateFlags flags) {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = window.graphicsFamilyIndex;
        poolInfo.flags = flags;

        VkCommandPool cpool = VK_NULL_HANDLE;
        if (auto const res = vkCreateCommandPool(window.device, &poolInfo, nullptr, &cpool); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Unable to create command pool\n"
                "vkCreateCommandPool() returned {}", toString(res)
            );
        }

        return CommandPool(window.device, cpool);
    }

    VkCommandBuffer allocCommandBuffer(VulkanWindow const& window, VkCommandPool pool) {
        VkCommandBufferAllocateInfo cbufInfo{};
        cbufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cbufInfo.commandPool = pool;
        cbufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cbufInfo.commandBufferCount = 1;

        VkCommandBuffer cbuff = VK_NULL_HANDLE;
        if(auto const res = vkAllocateCommandBuffers(window.device, &cbufInfo, &cbuff); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Unable to allocate command buffer\n"
                "vkAllocateCommandBuffers() returned {}", toString(res)
            );
        }

        return cbuff;
    }

    void recordCommands(VkCommandBuffer aCmdBuff, Pipelines const& pipelines, PipelineLayouts const& pipelineLayouts, ImageAndView const& swapchainImage, Image const& aDepthAttach, GBuffers& gbuffers, VkExtent2D const& aImageExtent, 
        VkBuffer aSceneUBO, Kiki::RenderManager::SceneUniform const& aSceneUniform, VkDescriptorSet aSceneDescriptors, VkDescriptorSet deferredLightingDescriptors,	VkDescriptorSet postProcessingDescriptors, VkDescriptorSet noTexture, Kiki::Skybox const& skybox, Image const& postProcessingImage) {
        // Begin recording commands
        VkCommandBufferBeginInfo begInfo{};
        begInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        begInfo.pInheritanceInfo = nullptr;

        if (auto const res = vkBeginCommandBuffer( aCmdBuff, &begInfo ); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Unable to begin recording command buffer\n"
                "vkBeginCommandBuffer() returned {}", toString(res)
            );
        }

        // Upload scene uniforms
        rutils::bufferBarrier(aCmdBuff, aSceneUBO,
            /* Before */
            VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
            VK_ACCESS_2_UNIFORM_READ_BIT,
            /* A f t e r */
            VK_PIPELINE_STAGE_2_CLEAR_BIT,// vkCmdUpdateBuffer() is a ”clear command”
            VK_ACCESS_2_TRANSFER_WRITE_BIT
        );

        vkCmdUpdateBuffer( aCmdBuff, aSceneUBO, 0, sizeof(Kiki::RenderManager::SceneUniform), &aSceneUniform );

        rutils::bufferBarrier(aCmdBuff, aSceneUBO,
            /* Before */
            VK_PIPELINE_STAGE_2_CLEAR_BIT,
            VK_ACCESS_2_TRANSFER_WRITE_BIT,
            /* A f t e r */
            VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
            VK_ACCESS_2_UNIFORM_READ_BIT
        );

        // Barrier: Ensure the color attachment image is in the right layout
        rutils::imageBarrier(aCmdBuff, postProcessingImage.image,
            /* Before */
            VK_PIPELINE_STAGE_2_NONE,
            VK_ACCESS_2_NONE,
            VK_IMAGE_LAYOUT_UNDEFINED,
            /* A f t e r */
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        );

        rutils::imageBarrier(aCmdBuff, aDepthAttach.image,
            /* Before */
            VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
            VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED,
            /* A f t e r */
            VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
            VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            /* What */
            VkImageSubresourceRange{ VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 }
        );

        rutils::imageBarrier(aCmdBuff, gbuffers.textureColour.image,
			// before
			VK_PIPELINE_STAGE_2_NONE,
			VK_ACCESS_2_NONE,
			VK_IMAGE_LAYOUT_UNDEFINED,
			// after
			VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);
	
		rutils::imageBarrier(aCmdBuff, gbuffers.normals.image,
			// before
			VK_PIPELINE_STAGE_2_NONE,
			VK_ACCESS_2_NONE,
			VK_IMAGE_LAYOUT_UNDEFINED,
			// after
			VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);
	
		rutils::imageBarrier(aCmdBuff, gbuffers.roughnessMetalness.image,
			// before
			VK_PIPELINE_STAGE_2_NONE,
			VK_ACCESS_2_NONE,
			VK_IMAGE_LAYOUT_UNDEFINED,
			// after
			VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);

		rutils::imageBarrier(aCmdBuff, gbuffers.worldPos.image,
			// before
			VK_PIPELINE_STAGE_2_NONE,
			VK_ACCESS_2_NONE,
			VK_IMAGE_LAYOUT_UNDEFINED,
			// after
			VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);

		// begin rendering
		VkRenderingAttachmentInfo gBufferAttachments[4]{};

		// texture colour
		gBufferAttachments[0].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		gBufferAttachments[0].imageView = gbuffers.textureColour.view;
		gBufferAttachments[0].imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
		gBufferAttachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		gBufferAttachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		gBufferAttachments[0].clearValue.color.float32[0] = 0.1f;
		gBufferAttachments[0].clearValue.color.float32[1] = 0.1f;
		gBufferAttachments[0].clearValue.color.float32[2] = 0.1f;
		gBufferAttachments[0].clearValue.color.float32[3] = 1.f;

		// normals
		gBufferAttachments[1].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		gBufferAttachments[1].imageView = gbuffers.normals.view;
		gBufferAttachments[1].imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
		gBufferAttachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		gBufferAttachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		gBufferAttachments[1].clearValue.color.float32[0] = 1.f;
		gBufferAttachments[1].clearValue.color.float32[1] = 1.f;
		gBufferAttachments[1].clearValue.color.float32[2] = 1.f;
		gBufferAttachments[1].clearValue.color.float32[3] = 0.f;

		// roughness and metalness
		gBufferAttachments[2].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		gBufferAttachments[2].imageView = gbuffers.roughnessMetalness.view;
		gBufferAttachments[2].imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
		gBufferAttachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		gBufferAttachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		gBufferAttachments[2].clearValue.color.float32[0] = 0.f;
		gBufferAttachments[2].clearValue.color.float32[1] = 0.f;
		gBufferAttachments[2].clearValue.color.float32[2] = 0.f;
		gBufferAttachments[2].clearValue.color.float32[3] = 0.f;

        // world pos
		gBufferAttachments[3].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		gBufferAttachments[3].imageView = gbuffers.worldPos.view;
		gBufferAttachments[3].imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
		gBufferAttachments[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		gBufferAttachments[3].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		gBufferAttachments[3].clearValue.color.float32[0] = 0.f;
		gBufferAttachments[3].clearValue.color.float32[1] = 0.f;
		gBufferAttachments[3].clearValue.color.float32[2] = 0.f;
		gBufferAttachments[3].clearValue.color.float32[3] = 0.f;

        VkRenderingAttachmentInfo depthAttach{};
        depthAttach.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttach.imageView = aDepthAttach.view;
        depthAttach.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        depthAttach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttach.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttach.clearValue.depthStencil.depth = 1.f;
  
        VkRenderingInfo renderInfo{};
        renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderInfo.layerCount = 1;
        renderInfo.renderArea.offset = VkOffset2D{ 0, 0 };
        renderInfo.renderArea.extent = aImageExtent;

        renderInfo.colorAttachmentCount = 4;
        renderInfo.pColorAttachments = gBufferAttachments;
        renderInfo.pDepthAttachment = &depthAttach;

        vkCmdBeginRendering( aCmdBuff, &renderInfo );

        // Begin drawing with our graphics pipeline
        vkCmdBindPipeline( aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.deferred_geometry.handle );


        vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.pbrPipelineLayout.handle, 0, 1, &aSceneDescriptors, 0, nullptr);

        auto& world = World::Get();
        auto& registry = world.Registry();
        auto view = world.Query<TransformComponent, MeshComponent>();

        Kiki::SceneManager& sceneManager = Kiki::SceneManager::get();

        std::vector<entt::entity> transparent;

        // TODO: Update so all objects with same material are drawn at same time to minimise descriptor set bind calls
        for (auto [e, transform, meshComponent] : view.each()) {
            if (sceneManager.validMesh(meshComponent.id)) {
                if (!registry.all_of<TransparencyComponent>(e)) {
                    Kiki::Mesh const& mesh = Kiki::SceneManager::get().getMesh(meshComponent.id);

                    if (registry.all_of<MaterialComponent>(e)) {
                        auto materialComponent = world.GetComponent<MaterialComponent>(e);

                        if (sceneManager.validMaterial(materialComponent->id)) {
                            Kiki::Material const& material = sceneManager.getMaterial(materialComponent->id);
                            vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.pbrPipelineLayout.handle, 1, 1, &material.descriptorSet, 0, nullptr);
                        } else {
                            vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.pbrPipelineLayout.handle, 1, 1, &noTexture, 0, nullptr);
                        }
                    } else {
                        vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.pbrPipelineLayout.handle, 1, 1, &noTexture, 0, nullptr);
                    }

                    glm::vec3 colour;

                    if (registry.all_of<ColourComponent>(e)) {
                        colour = registry.get<ColourComponent>(e).colour;
                    } else {
                        colour = glm::vec3(0.3f, 0.3f, 0.3f);
                    }

                    ObjectData objData = ObjectData(transform.worldMatrix, glm::vec4(colour, 1.0f));

                    // Bind vertex input
                    VkBuffer buffers[3] = { mesh.positions.buffer, mesh.texCoords.buffer, mesh.normals.buffer };
                    VkDeviceSize offsets[3]{};

                    vkCmdBindVertexBuffers(aCmdBuff, 0, 3, buffers, offsets);
                    vkCmdBindIndexBuffer(aCmdBuff, mesh.indices.buffer, 0, VK_INDEX_TYPE_UINT32);

                    vkCmdPushConstants(aCmdBuff, pipelineLayouts.pbrPipelineLayout.handle, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(objData), &objData);

                    // Draw mesh
                    vkCmdDrawIndexed(aCmdBuff, mesh.indexCount, 1, 0, 0, 0);
                } else {
                    transparent.emplace_back(e);
                }
            }
        }

        vkCmdBindPipeline(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.deferred_geometry_alpha.handle);
        vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.pbrPipelineLayout.handle, 0, 1, &aSceneDescriptors, 0, nullptr);

        // TODO: sort transparent objects

        for (auto e : transparent) {
            Kiki::Mesh const& mesh = Kiki::SceneManager::get().getMesh(registry.get<MeshComponent>(e).id);
            TransparencyComponent transparentComponent = registry.get<TransparencyComponent>(e);
            TransformComponent const& transform = registry.get<TransformComponent>(e);

            if (registry.all_of<MaterialComponent>(e)) {
                auto materialComponent = world.GetComponent<MaterialComponent>(e);

                if (sceneManager.validMaterial(materialComponent->id)) {
                    Kiki::Material const& material = sceneManager.getMaterial(materialComponent->id);
                    vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.pbrPipelineLayout.handle, 1, 1, &material.descriptorSet, 0, nullptr);
                } else {
                    vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.pbrPipelineLayout.handle, 1, 1, &noTexture, 0, nullptr);
                }
            } else {
                vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.pbrPipelineLayout.handle, 1, 1, &noTexture, 0, nullptr);
            }

            glm::vec3 colour;

            if (registry.all_of<ColourComponent>(e)) {
                colour = registry.get<ColourComponent>(e).colour;
            } else {
                colour = glm::vec3(0.3f, 0.3f, 0.3f);
            }

            ObjectData objData = ObjectData(transform.worldMatrix, glm::vec4(colour, (1.0f - transparentComponent.transparency)), (transparentComponent.sprite ? 1:0));

            // Bind vertex input
            VkBuffer buffers[3] = { mesh.positions.buffer, mesh.texCoords.buffer, mesh.normals.buffer };
            VkDeviceSize offsets[3]{};

            vkCmdBindVertexBuffers(aCmdBuff, 0, 3, buffers, offsets);
            vkCmdBindIndexBuffer(aCmdBuff, mesh.indices.buffer, 0, VK_INDEX_TYPE_UINT32);

            vkCmdPushConstants(aCmdBuff, pipelineLayouts.pbrPipelineLayout.handle, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(objData), &objData);

            // Draw mesh
            vkCmdDrawIndexed(aCmdBuff, mesh.indexCount, 1, 0, 0, 0);
        }

        // End rendering
        vkCmdEndRendering(aCmdBuff);

		// deferred lighting pass

		// ensure g buffer images are in VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		rutils::imageBarrier(aCmdBuff, gbuffers.textureColour.image,
			// before
			VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			// after
			VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);
	
		rutils::imageBarrier(aCmdBuff, gbuffers.normals.image,
			// before
			VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			// after
			VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);
	
		rutils::imageBarrier(aCmdBuff, gbuffers.roughnessMetalness.image,
			// before
			VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			// after
			VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);

        rutils::imageBarrier(aCmdBuff, gbuffers.worldPos.image,
			// before
			VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			// after
			VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);


		rutils::imageBarrier(aCmdBuff, aDepthAttach.image,
			// before
			VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
			VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
			// after
			VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
			// which part
			VkImageSubresourceRange{VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1}
		);

        VkRenderingAttachmentInfo lightingAttach{};
		lightingAttach.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		lightingAttach.imageView = postProcessingImage.view;
		lightingAttach.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
		lightingAttach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		lightingAttach.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		lightingAttach.clearValue.color.float32[0] = 0.f;
		lightingAttach.clearValue.color.float32[1] = 0.f;
		lightingAttach.clearValue.color.float32[2] = 0.f;
		lightingAttach.clearValue.color.float32[3] = 1.f;

		VkRenderingAttachmentInfo depthAttachLighting{};
		depthAttachLighting.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		depthAttachLighting.imageView = aDepthAttach.view;
		depthAttachLighting.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

		VkRenderingInfo lightingInfo{};
		lightingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		lightingInfo.layerCount = 1;
		lightingInfo.renderArea.offset = VkOffset2D{0, 0};
		lightingInfo.renderArea.extent = VkExtent2D{aImageExtent.width, aImageExtent.height};

		lightingInfo.colorAttachmentCount = 1;
		lightingInfo.pColorAttachments = &lightingAttach;
        lightingInfo.pDepthAttachment = &depthAttachLighting;

		vkCmdBeginRendering(aCmdBuff, &lightingInfo);

		// draw fullscreen quad
		vkCmdBindPipeline(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.deferred_lighting.handle);

		VkDescriptorSet sets[] = {
			aSceneDescriptors,
			deferredLightingDescriptors
		};

		vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.deferredPipelineLayout.handle, 0, 2, sets, 0, nullptr);
		vkCmdDraw(aCmdBuff, 3, 1, 0, 0);

#       ifndef NDEBUG
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), aCmdBuff);
#       endif

		vkCmdEndRendering(aCmdBuff);

        // post processing pass
        // transition the image we just rendered to be sampled
        imageBarrier(aCmdBuff, postProcessingImage.image,
            // before
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            // after
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );

        imageBarrier(aCmdBuff, swapchainImage.image,
            // before
            VK_PIPELINE_STAGE_2_NONE,
            VK_ACCESS_2_NONE,
            VK_IMAGE_LAYOUT_UNDEFINED,
            // after
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        );

        // begin post processing
        VkRenderingAttachmentInfo postColourAttach{};
        postColourAttach.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        postColourAttach.imageView = swapchainImage.view;
        postColourAttach.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        postColourAttach.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        postColourAttach.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        VkRenderingInfo postRenderInfo{};
        postRenderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        postRenderInfo.layerCount = 1;
        postRenderInfo.renderArea.offset = VkOffset2D{0, 0};
        postRenderInfo.renderArea.extent = VkExtent2D{aImageExtent.width, aImageExtent.height};

        postRenderInfo.colorAttachmentCount = 1;
        postRenderInfo.pColorAttachments = &postColourAttach;

        vkCmdBeginRendering(aCmdBuff, &postRenderInfo);

        // draw fullscreen quad with post-processing shader
        vkCmdBindPipeline(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.fxaa.handle);
        vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.postprocessPipelineLayout.handle, 0, 1, &postProcessingDescriptors, 0, nullptr);
        vkCmdDraw(aCmdBuff, 3, 1, 0, 0);

        vkCmdEndRendering(aCmdBuff);

        // end post processing pass

        // Barrier: synchronize with the copy after and transition image is to TRANSFER SRC OPTIMAL
        imageBarrier(aCmdBuff, swapchainImage.image,
            /* Before */
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            /* A f t e r */
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_2_NONE,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        );

        // End command recording
        if (auto const res = vkEndCommandBuffer( aCmdBuff ); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Unable to end recording command buffer\n"
                "vkEndCommandBuffer() returned {}", toString(res)
            );
        }
    }

    void submitCommands(VulkanWindow const& window, VkCommandBuffer aCmdBuff, VkFence aFence, VkSemaphore aWaitSemaphore, VkSemaphore aSignalSemaphore) {
        VkSemaphoreSubmitInfo wait[1]{};
        wait[0].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        wait[0].semaphore = aWaitSemaphore;
        wait[0].stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSemaphoreSubmitInfo signal[1]{};
        signal[0].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        signal[0].semaphore = aSignalSemaphore;
        signal[0].stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkCommandBufferSubmitInfo submit[1]{};
        submit[0].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        submit[0].commandBuffer = aCmdBuff;

        VkSubmitInfo2 submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submitInfo.waitSemaphoreInfoCount = 1;
        submitInfo.pWaitSemaphoreInfos = wait;
        submitInfo.commandBufferInfoCount = 1;
        submitInfo.pCommandBufferInfos = submit;
        submitInfo.signalSemaphoreInfoCount = 1;
        submitInfo.pSignalSemaphoreInfos = signal;

        if (auto const res = vkQueueSubmit2( window.graphicsQueue, 1, &submitInfo, aFence); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Unable to submit command buffer to queue\n"
                "vkQueueSubmit2() returned {}", toString(res)
            );
        }
    }
}