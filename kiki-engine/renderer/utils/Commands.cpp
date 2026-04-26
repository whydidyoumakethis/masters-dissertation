#include "Commands.hpp"

#include "Synchronisation.hpp"
#include "ToString.hpp"
#include "Pipelines.hpp"
#include "Components/TransparencyComponent.hpp"
#include "Components/ColourComponent.hpp"
#include "Components/RoughnessMetallicFactorComponent.hpp"
#include "../../logging/FatalError.hpp"
#include "../RenderManager.hpp"
#include "../SceneManager.hpp"

#include "../../ECS/World.h"

#include <iostream>
#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>

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

    void recordCommands(
        VkCommandBuffer aCmdBuff,
# ifdef TRACY_VK_ENABLE
        TracyVkCtx tracyVkCtx,
# endif
        Pipelines const& pipelines,
        PipelineLayouts const& pipelineLayouts,
        ImageAndView const& swapchainImage,
        Image const& aDepthAttach,
        GBuffers& gbuffers,
        VkExtent2D const& aImageExtent, 
        VkBuffer aSceneUBO,
        Kiki::RenderManager::SceneUniform const& aSceneUniform,
        VkDescriptorSet aSceneDescriptors,
        VkDescriptorSet deferredLightingDescriptors,
        VkDescriptorSet fxaaDescriptors,
        VkDescriptorSet ssrDescriptors,
        VkDescriptorSet noTexture,
        Kiki::Skybox const& skybox,
        Image const& doneLightingImage,
        Image const& doneSSRImage
    ) {
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

        {
            ZoneScopedN("Uploading scene UBO");

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
        }

        {
            ZoneScopedN("Initial image barriers");

            // Barrier: Ensure the color attachment image is in the right layout
            rutils::imageBarrier(aCmdBuff, doneLightingImage.image,
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

            rutils::imageBarrier(aCmdBuff, gbuffers.mappedNormals.image,
                // before
                VK_PIPELINE_STAGE_2_NONE,
                VK_ACCESS_2_NONE,
                VK_IMAGE_LAYOUT_UNDEFINED,
                // after
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            );
        }
        

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

        // mapped normals
		gBufferAttachments[3].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		gBufferAttachments[3].imageView = gbuffers.mappedNormals.view;
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

        {
            ZoneScopedN("Recording g-buffer pass");

            #ifdef TRACY_VK_ENABLE
            TracyVkZone(tracyVkCtx, aCmdBuff, "G-buffer pass");
            #endif

            vkCmdBeginRendering( aCmdBuff, &renderInfo );

            std::vector<entt::entity> transparent;

            auto& world = World::Get();
            auto& registry = world.Registry();
            auto view = world.Query<TransformComponent, MeshComponent>();
            Kiki::SceneManager& sceneManager = Kiki::SceneManager::get();

            {
                ZoneScopedN("Recording opaque draws");

                // Begin drawing with our graphics pipeline
                vkCmdBindPipeline( aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.deferred_geometry.handle );

                vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.pbrPipelineLayout.handle, 0, 1, &aSceneDescriptors, 0, nullptr);

                // TODO: Update so all objects with same material are drawn at same time to minimise descriptor set bind calls
                for (auto [e, transform, meshComponent] : view.each()) {
                    if (sceneManager.validMesh(meshComponent.id)) {
                        if (!registry.all_of<TransparencyComponent>(e)) {
                            Kiki::Mesh const& mesh = Kiki::SceneManager::get().getMesh(meshComponent.id);

                            glm::vec4 flags;
                            flags.x = 0.f; // sprite
                            flags.y = 1.f; // useTexture
                            flags.z = 1.f; // roughnessFactor
                            flags.w = 1.f; // metalnessFactor

                            if (registry.all_of<MaterialComponent>(e)) {
                                auto materialComponent = world.GetComponent<MaterialComponent>(e);

                                if (sceneManager.validMaterial(materialComponent->id)) {
                                    Kiki::Material const& material = sceneManager.getMaterial(materialComponent->id);

                                    // if material doesn't have a texture, use base colour instead
                                    if (material.hasTexture == false) {
                                        flags.y = 0.f;
                                    }

                                    vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.pbrPipelineLayout.handle, 1, 1, &material.descriptorSet, 0, nullptr);
                                } else {
                                    flags.y = 0.f;
                                    vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.pbrPipelineLayout.handle, 1, 1, &noTexture, 0, nullptr);
                                }
                            } else {
                                flags.y = 0.f;
                                vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.pbrPipelineLayout.handle, 1, 1, &noTexture, 0, nullptr);
                            }

                            glm::vec3 colour;

                            if (registry.all_of<ColourComponent>(e)) {
                                colour = registry.get<ColourComponent>(e).colour;
                            } else {
                                colour = glm::vec3(0.3f, 0.3f, 0.3f);
                            }

                            glm::vec2 roughnessMetalnessFactors;

                            if (registry.all_of<RoughnessMetallicFactorComponent>(e)) {
                                roughnessMetalnessFactors = registry.get<RoughnessMetallicFactorComponent>(e).roughnessMetallicFactors;
                                flags.z = roughnessMetalnessFactors.r;
                                flags.w = roughnessMetalnessFactors.g;
                            }
                            else {
                                roughnessMetalnessFactors = glm::vec2(1.f, 1.f);
                            }

                            ObjectData objData = ObjectData(transform.worldMatrix, glm::vec4(colour, 1.0f), flags);

                            // Bind vertex input
                            VkBuffer buffers[4] = { mesh.positions.buffer, mesh.texCoords.buffer, mesh.normals.buffer, mesh.tangents.buffer };
                            VkDeviceSize offsets[4]{};

                            vkCmdBindVertexBuffers(aCmdBuff, 0, 4, buffers, offsets);
                            vkCmdBindIndexBuffer(aCmdBuff, mesh.indices.buffer, 0, VK_INDEX_TYPE_UINT32);

                            vkCmdPushConstants(aCmdBuff, pipelineLayouts.pbrPipelineLayout.handle, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(objData), &objData);

                            // Draw mesh
                            vkCmdDrawIndexed(aCmdBuff, mesh.indexCount, 1, 0, 0, 0);
                        } else {
                            transparent.emplace_back(e);
                        }
                    }
                }
            }

            {
                ZoneScopedN("Recording transparent draws");

                vkCmdBindPipeline(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.deferred_geometry_alpha.handle);
                vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.pbrPipelineLayout.handle, 0, 1, &aSceneDescriptors, 0, nullptr);

                // TODO: sort transparent objects

                for (auto e : transparent) {
                    Kiki::Mesh const& mesh = Kiki::SceneManager::get().getMesh(registry.get<MeshComponent>(e).id);
                    TransparencyComponent transparentComponent = registry.get<TransparencyComponent>(e);
                    TransformComponent const& transform = registry.get<TransformComponent>(e);

                    glm::vec4 flags;
                    flags.x = 0.f; // sprite
                    flags.y = 1.f; // useTexture
                    flags.z = 1.f; // roughnessFactor
                    flags.w = 1.f; // metalnessFactor

                    if (registry.all_of<MaterialComponent>(e)) {
                        auto materialComponent = world.GetComponent<MaterialComponent>(e);

                        if (sceneManager.validMaterial(materialComponent->id)) {
                            Kiki::Material const& material = sceneManager.getMaterial(materialComponent->id);

                            // if material doesn't have a texture, use base colour instead
                            if (material.hasTexture == false) {
                                flags.y = 0.f;
                            }

                            vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.pbrPipelineLayout.handle, 1, 1, &material.descriptorSet, 0, nullptr);
                        } else {
                            flags.y = 0.f;
                            vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.pbrPipelineLayout.handle, 1, 1, &noTexture, 0, nullptr);
                        }
                    } else {
                        flags.y = 0.f;
                        vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.pbrPipelineLayout.handle, 1, 1, &noTexture, 0, nullptr);
                    }

                    glm::vec3 colour;

                    if (registry.all_of<ColourComponent>(e)) {
                        colour = registry.get<ColourComponent>(e).colour;
                    } else {
                        colour = glm::vec3(0.3f, 0.3f, 0.3f);
                    }

                    if (transparentComponent.sprite) {
                        flags.x = 1.f;
                    }


                    glm::vec2 roughnessMetalnessFactors;

                    if (registry.all_of<RoughnessMetallicFactorComponent>(e)) {
                        roughnessMetalnessFactors = registry.get<RoughnessMetallicFactorComponent>(e).roughnessMetallicFactors;
                        flags.z = roughnessMetalnessFactors.r;
                        flags.w = roughnessMetalnessFactors.g;
                    }
                    else {
                        roughnessMetalnessFactors = glm::vec2(1.f, 1.f);
                    }

                    ObjectData objData = ObjectData(transform.worldMatrix, glm::vec4(colour, (1.0f - transparentComponent.transparency)), flags);

                    // Bind vertex input
                    VkBuffer buffers[4] = { mesh.positions.buffer, mesh.texCoords.buffer, mesh.normals.buffer, mesh.tangents.buffer };
                    VkDeviceSize offsets[4]{};

                    vkCmdBindVertexBuffers(aCmdBuff, 0, 4, buffers, offsets);
                    vkCmdBindIndexBuffer(aCmdBuff, mesh.indices.buffer, 0, VK_INDEX_TYPE_UINT32);

                    vkCmdPushConstants(aCmdBuff, pipelineLayouts.pbrPipelineLayout.handle, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(objData), &objData);

                    // Draw mesh
                    vkCmdDrawIndexed(aCmdBuff, mesh.indexCount, 1, 0, 0, 0);
                }

                // End rendering
                vkCmdEndRendering(aCmdBuff);
            }
        }

		// deferred lighting pass

        {
            ZoneScopedN("G-buffer pass to lighting pass barriers");

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

            rutils::imageBarrier(aCmdBuff, gbuffers.mappedNormals.image,
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
        }

        VkRenderingAttachmentInfo lightingAttach{};
		lightingAttach.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		lightingAttach.imageView = doneLightingImage.view;
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

        {
            ZoneScopedN("Recording deferred lighting pass");

            #ifdef TRACY_VK_ENABLE
            TracyVkZone(tracyVkCtx, aCmdBuff, "Deferred lighting pass");
            #endif

            vkCmdBeginRendering(aCmdBuff, &lightingInfo);

            // draw fullscreen quad
            vkCmdBindPipeline(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.deferred_lighting.handle);

            VkDescriptorSet sets[] = {
                aSceneDescriptors,
                deferredLightingDescriptors
            };

            vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.deferredPipelineLayout.handle, 0, 2, sets, 0, nullptr);
            vkCmdDraw(aCmdBuff, 3, 1, 0, 0);

            vkCmdEndRendering(aCmdBuff);
        }

        {
            ZoneScopedN("Recording SSR pass");

            #ifdef TRACY_VK_ENABLE
            TracyVkZone(tracyVkCtx, aCmdBuff, "SSR pass");
            #endif

            // begin ssr pass
            // transition the image we just rendered to be sampled
            imageBarrier(aCmdBuff, doneLightingImage.image,
                // before
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                // after
                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            );

            imageBarrier(aCmdBuff, doneSSRImage.image,
                // before
                VK_PIPELINE_STAGE_2_NONE,
                VK_ACCESS_2_NONE,
                VK_IMAGE_LAYOUT_UNDEFINED,
                // after
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            );

            VkRenderingAttachmentInfo ssrColourAttach{};
            ssrColourAttach.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            ssrColourAttach.imageView = doneSSRImage.view;
            ssrColourAttach.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            ssrColourAttach.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            ssrColourAttach.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

            VkRenderingInfo ssrRenderInfo{};
            ssrRenderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
            ssrRenderInfo.layerCount = 1;
            ssrRenderInfo.renderArea.offset = VkOffset2D{0, 0};
            ssrRenderInfo.renderArea.extent = VkExtent2D{aImageExtent.width, aImageExtent.height};

            ssrRenderInfo.colorAttachmentCount = 1;
            ssrRenderInfo.pColorAttachments = &ssrColourAttach;

            vkCmdBeginRendering(aCmdBuff, &ssrRenderInfo);

            VkDescriptorSet ssrSets[] = {
                aSceneDescriptors,
                ssrDescriptors
            };

            // draw fullscreen quad with post-processing shader
            vkCmdBindPipeline(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.ssr.handle);
            vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.postprocessPipelineLayout.handle, 0, 2, ssrSets, 0, nullptr);

            SSRSettings ssrSettings;
            ssrSettings.settings.x = 128; // maxSteps
            ssrSettings.settings.y = 6; // binarySteps
            ssrSettings.settings.z = 0.1f; // stepSize
            ssrSettings.settings.w = 0.2f; // thicknessTolerance

            // TODO: debug, could change the ssr settings here :)

            vkCmdPushConstants(aCmdBuff, pipelineLayouts.postprocessPipelineLayout.handle, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ssrSettings), &ssrSettings);

            vkCmdDraw(aCmdBuff, 3, 1, 0, 0);

            vkCmdEndRendering(aCmdBuff);
            // end ssr pass
        }

        {
            ZoneScopedN("Recording FXAA and ImGui pass");

            #ifdef TRACY_VK_ENABLE
            TracyVkZone(tracyVkCtx, aCmdBuff, "FXAA and ImGui pass");
            #endif

            // begin fxaa pass
            // transition the image we just rendered to be sampled
            imageBarrier(aCmdBuff, doneSSRImage.image,
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

            VkRenderingAttachmentInfo fxaaColourAttach{};
            fxaaColourAttach.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            fxaaColourAttach.imageView = swapchainImage.view;
            fxaaColourAttach.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            fxaaColourAttach.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            fxaaColourAttach.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

            VkRenderingInfo fxaaRenderInfo{};
            fxaaRenderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
            fxaaRenderInfo.layerCount = 1;
            fxaaRenderInfo.renderArea.offset = VkOffset2D{0, 0};
            fxaaRenderInfo.renderArea.extent = VkExtent2D{aImageExtent.width, aImageExtent.height};

            fxaaRenderInfo.colorAttachmentCount = 1;
            fxaaRenderInfo.pColorAttachments = &fxaaColourAttach;

            vkCmdBeginRendering(aCmdBuff, &fxaaRenderInfo);

            VkDescriptorSet fxaaSets[] = {
                aSceneDescriptors,
                fxaaDescriptors
            };

            // draw fullscreen quad with post-processing shader
            vkCmdBindPipeline(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.fxaa.handle);
            vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.postprocessPipelineLayout.handle, 0, 2, fxaaSets, 0, nullptr);
            vkCmdDraw(aCmdBuff, 3, 1, 0, 0);

            // begin imgui pass
    #       ifndef NDEBUG
            {
                ZoneScopedN("Recording ImGui pass");

                #ifdef TRACY_VK_ENABLE
                TracyVkZone(tracyVkCtx, aCmdBuff, "ImGui pass")
                #endif
                ImGui::Render();
                ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), aCmdBuff);

            }
    #       endif
            // end imgui pass

            vkCmdEndRendering(aCmdBuff);
            // end fxaa pass
        }

        // TODO: ui pass should go here, after fxaa :)

        {
            ZoneScopedN("Presentation barrier");

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
        }

        #ifdef TRACY_VK_ENABLE
        TracyVkCollect(tracyVkCtx, aCmdBuff);
        #endif
        
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