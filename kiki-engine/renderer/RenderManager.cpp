#include "RenderManager.hpp"

#include "utils/VulkanWindow.hpp"
#include "utils/VulkanWrapper.hpp"
#include "utils/Pipelines.hpp"
#include "utils/Commands.hpp"
#include "utils/Synchronisation.hpp"
#include "utils/ToString.hpp"
#include "utils/Descriptors.hpp"
#include "utils/Image.hpp"
#include "../logging/FatalError.hpp"
#include "SceneManager.hpp"

#include <iostream>
#include <spdlog/spdlog.h>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui_impl_glfw.h>


namespace Kiki {
    RenderManager& RenderManager::get() {
        static RenderManager instance;
        return instance;
    }

    void RenderManager::initialise(WindowInfo info) {
        if (!initialised) {
            initialised = true;

            // Create window
            window = rutils::makeVulkanWindow(info);

            // Initialise resources
            sceneLayout = rutils::createSceneDescriptorLayout(window);
            materialLayout = rutils::createMaterialDescriptorLayout(window);
            gBufferLayout = rutils::createGBufferDescriptorLayout(window);
            cubemapLayout = rutils::createCubemapDescriptorLayout(window);
            
            allocator = rutils::createAllocator(window);

            sampler = rutils::createSampler(window);

            sceneUBO = rutils::createBuffer(
                allocator,
                sizeof(SceneUniform),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                0,
                VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
            );

            pipelineLayouts.pbrPipelineLayout = rutils::createPipelineLayout(window, sceneLayout.handle, materialLayout.handle);
            pipelineLayouts.deferredPipelineLayout = rutils::createPipelineLayout(window, sceneLayout.handle, gBufferLayout.handle);
            pipelineLayouts.skyboxPipelineLayout = rutils::createPipelineLayout(window, sceneLayout.handle, cubemapLayout.handle);

            pipelines = rutils::createAllPipelines(window, pipelineLayouts);
            commandPool = rutils::createCommandPool(window, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
            tempTextureCmdPool = rutils::createCommandPool(window, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);

            descriptorPool = rutils::createDescriptorPool(window);

            depthBuffer = rutils::createDepthBuffer(window, allocator);

            gbuffers = rutils::createAllGBufferImages(window, allocator);

            createSkybox(skybox.paths);

            deferredLightingDescriptors = rutils::allocDescSet(window, descriptorPool.handle, gBufferLayout.handle);
            initialiseDeferredLightingDescriptorSet(window, gbuffers, depthBuffer, sampler, deferredLightingDescriptors, skybox.cubemap, skybox.sampler);


            sceneDescriptors = rutils::allocDescSet(window, descriptorPool.handle, sceneLayout.handle );

            VkWriteDescriptorSet sceneDesc[1]{};

            VkDescriptorBufferInfo sceneUboInfo{};
            sceneUboInfo.buffer = sceneUBO.buffer;
            sceneUboInfo.range = VK_WHOLE_SIZE;

            sceneDesc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            sceneDesc[0].dstSet = sceneDescriptors;
            sceneDesc[0].dstBinding = 0;
            sceneDesc[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            sceneDesc[0].descriptorCount = 1;
            sceneDesc[0].pBufferInfo = &sceneUboInfo;


            constexpr auto sceneNumSets = sizeof(sceneDesc)/sizeof(sceneDesc[0]);
            vkUpdateDescriptorSets( window.device, sceneNumSets, sceneDesc, 0, nullptr );


            for (std::size_t i = 0; i < window.swapImages.size(); i++) {
                commandBuffers.emplace_back(rutils::allocCommandBuffer(window, commandPool.handle));
                frameDone.emplace_back(rutils::createFence(window.device, VK_FENCE_CREATE_SIGNALED_BIT));
                imageAvailable.emplace_back(rutils::createSemaphore(window.device));
                renderFinished.emplace_back(rutils::createSemaphore(window.device));
            }

            // Create null texture
            stbi_set_flip_vertically_on_load(1);

            int width, height, c;

            stbi_uc* emptyTexture = stbi_load((std::filesystem::path(PROJECT_ROOT_PATH) / "assets/empty.png").string().c_str(), &width, &height, &c, 4 /* want 4 c h a n n e l s = RGBA */);

            noTexture = rutils::loadImageTexture(emptyTexture, width, height, window, tempTextureCmdPool.handle, allocator);
        
            noTextureDst = rutils::allocDescSet(window, descriptorPool.handle, materialLayout.handle);

            VkWriteDescriptorSet desc[2]{};

            VkDescriptorImageInfo textureInfo{};
            textureInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            textureInfo.imageView = noTexture.view;
            textureInfo.sampler = sampler.handle;

            // empty colour
            desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            desc[0].dstSet = noTextureDst;
            desc[0].dstBinding = 0;
            desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            desc[0].descriptorCount = 1;
            desc[0].pImageInfo = &textureInfo;

            // empty roughness + metalness
            desc[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            desc[1].dstSet = noTextureDst;
            desc[1].dstBinding = 1;
            desc[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            desc[1].descriptorCount = 1;
            desc[1].pImageInfo = &textureInfo;

            constexpr auto numSets = sizeof(desc)/sizeof(desc[0]);
            vkUpdateDescriptorSets(window.device, numSets, desc, 0, nullptr);
        }
    }

    void RenderManager::nextFrame() {
        // glfwPollEvents(); called in input manager
        
        if (recreateSwapchain) {
            // We need to destroy several objects, which may still be in use by the GPU. Therefore, first wait for the GPU
            // to finish processing.
            vkDeviceWaitIdle(window.device);

            // Recreate resources
            rutils::recreateSwapchain(window);

            pipelines = rutils::createAllPipelines(window, pipelineLayouts);

            depthBuffer = rutils::createDepthBuffer(window, allocator);
            
            gbuffers = rutils::createAllGBufferImages(window, allocator);
            initialiseDeferredLightingDescriptorSet(window, gbuffers, depthBuffer, sampler, deferredLightingDescriptors, skybox.cubemap, skybox.sampler);

            recreateSwapchain = false;
        }

        // Advance to next frame
        frameIndex++;
        frameIndex %= commandBuffers.size();

        // Make sure that the frame resources are no longer in use
        assert(frameIndex < frameDone.size());

        if (auto const res = vkWaitForFences(window.device, 1, &frameDone[frameIndex].handle, VK_TRUE, std::numeric_limits<std::uint64_t>::max()); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Unable to wait for frame fence {}\n"
                "vkWaitForFences() returned {}", frameIndex, rutils::toString(res)
            );
        }

        // Acquire next swap chain image
        assert(frameIndex < imageAvailable.size());

        std::uint32_t imageIndex = 0;
        auto const acquireRes = vkAcquireNextImageKHR(
            window.device,
            window.swapchain,
            std::numeric_limits<std::uint64_t>::max(),
            imageAvailable[frameIndex].handle,
            VK_NULL_HANDLE,
            &imageIndex
        );

        if (VK_ERROR_OUT_OF_DATE_KHR == acquireRes) {
            // This occurs e.g., when the window has been resized. In this case we need to recreate the swap chain to
            // match the new dimensions. Any resources that directly depend on the swap chain need to be recreated
            // as well. While rare, re-creating the swap chain may give us a different image format, which we should
            // handle appropriately.
            //
            // In both cases, we set the flag that the swap chain has to be re-created and jump to the top of the loop.
            // Technically, with the VK SUBOPTIMAL KHR return code, we could continue rendering with the
            // current swap chain (unlike VK ERROR OUT OF DATE KHR, which does require us to recreate the
            // swap chain).

            recreateSwapchain = true;

            // We won’t render a frame this time around. Consequently, no commands were submitted for execution
            // and the associated fence won’t be signalled. Stepping back one frame avoids this problem.
            --frameIndex;
            frameIndex %= commandBuffers.size();

#           ifndef NDEBUG
            ImGui::EndFrame();
#           endif

            return;
        }

        if (acquireRes == VK_SUBOPTIMAL_KHR) {
            recreateSwapchain = true;
        }
        else if (VK_SUCCESS != acquireRes) {
            throw Kiki::FatalError( "Unable to acquire next swapchain image\n"
                "vkAcquireNextImageKHR() returned {}", rutils::toString(acquireRes)
            );
        }

        // Reset fence
        // Do this only after AcquireNextImage(), so that we can wait on the same fence again in case the swapchain
        // had to be re-created.
        
        if(auto const res = vkResetFences( window.device, 1, &frameDone[frameIndex].handle); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Unable to reset frame fence {}\n"
                "vkResetFences() returned {}", frameIndex, rutils::toString(res)
            );
        }

        // Record and submit commands for this frame
        // Prepare data for this frame
        SceneUniform sceneUniforms{};
        updateSceneUniforms(sceneUniforms, window.swapchainExtent.width, window.swapchainExtent.height);

        // std::cout << sceneUniforms.lightColour.r << std::endl;
    
        assert(std::size_t(imageIndex) < window.swapImages.size());

        rutils::ImageAndView colorTarget;
        colorTarget.image = window.swapImages[imageIndex];
        colorTarget.view = window.swapViews[imageIndex];

        assert(std::size_t(frameIndex) < commandBuffers.size());

        rutils::recordCommands(
            commandBuffers[frameIndex],
            pipelines,
            pipelineLayouts,
            colorTarget,
            depthBuffer,
            gbuffers,
            window.swapchainExtent,
            sceneUBO.buffer,
            sceneUniforms,
            sceneDescriptors,
            deferredLightingDescriptors,
            noTextureDst,
            skybox
        );

        assert(std::size_t(frameIndex) < renderFinished.size());

        rutils::submitCommands(
            window,
            commandBuffers[frameIndex],
            frameDone[frameIndex].handle,
            imageAvailable[frameIndex].handle,
            renderFinished[frameIndex].handle
        );

        // Present the results
        // https://registry.khronos.org/vulkan/specs/latest/man/html/VkPresentInfoKHR.html
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &renderFinished[frameIndex].handle;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &window.swapchain;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr;

        auto const presentRes = vkQueuePresentKHR(window.presentQueue, &presentInfo);

        if (VK_SUBOPTIMAL_KHR == presentRes || VK_ERROR_OUT_OF_DATE_KHR == presentRes) {
            recreateSwapchain = true;
        }
        else if (VK_SUCCESS != presentRes) {
            throw Kiki::FatalError( "Unable present swapchain image {}\n"
                "vkQueuePresentKHR() returned {}", imageIndex, rutils::toString(presentRes)
            );
        }
    }

    Mesh RenderManager::allocateMesh(std::vector<float> positions, std::vector<std::uint32_t> indices, std::vector<float> normals, std::vector<float> texCoords) {
        int posSize = positions.size() * sizeof(float);
        int indSize = indices.size() * sizeof (std::uint32_t);
        int texSize = texCoords.size() * sizeof(float);
        int normalsSize = normals.size() * sizeof(float);

        // Create on GPU vertex buffer
        rutils::Buffer vertexPosGPU = rutils::createBuffer(
            allocator,
            posSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            0, // no additional VmaAllocationCreateFlags
            VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE // or just VMA MEMORY USAGE AUTO
        );

        // Create on GPU index buffer
        rutils::Buffer indexGPU = rutils::createBuffer(
            allocator,
            indSize,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            0, // no additional VmaAllocationCreateFlags
            VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE // or just VMA MEMORY USAGE AUTO
        );

        // Create on GPU tex coords buffer
        rutils::Buffer texCoordsGPU = rutils::createBuffer(
            allocator,
            texSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            0, // no additional VmaAllocationCreateFlags
            VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE // or just VMA MEMORY USAGE AUTO
        );

        // Create on GPU normal buffer
        rutils::Buffer normalsGPU = rutils::createBuffer(
            allocator,
            normalsSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            0, // no additional VmaAllocationCreateFlags
            VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE // or just VMA MEMORY USAGE AUTO
        );

        // Create staging buffers
        rutils::Buffer posStaging = rutils::createBuffer(
            allocator,
            posSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
        );

        rutils::Buffer indStaging = rutils::createBuffer(
            allocator,
            indSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
        );

        rutils::Buffer texStaging = rutils::createBuffer(
            allocator,
            texSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
        );

        rutils::Buffer normalsStaging = rutils::createBuffer(
            allocator,
            normalsSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
        );

        void* posPtr = nullptr;
        if (auto const res = vmaMapMemory(allocator.allocator, posStaging.allocation, &posPtr); VK_SUCCESS != res) {
            throw FatalError( "Mapping memory for writing\n"
                "vmaMapMemory() returned {}", rutils::toString(res)
            );
        }
        std::memcpy(posPtr, positions.data(), posSize);
        vmaUnmapMemory(allocator.allocator, posStaging.allocation);

        void* indPtr = nullptr;
        if (auto const res = vmaMapMemory(allocator.allocator, indStaging.allocation, &indPtr); VK_SUCCESS != res) {
            throw FatalError( "Mapping memory for writing\n"
                "vmaMapMemory() returned {}", rutils::toString(res)
            );
        }
        std::memcpy(indPtr, indices.data(), indSize);
        vmaUnmapMemory(allocator.allocator, indStaging.allocation);

        void* texPtr = nullptr;
        if (auto const res = vmaMapMemory(allocator.allocator, texStaging.allocation, &texPtr); VK_SUCCESS != res) {
            throw FatalError( "Mapping memory for writing\n"
                "vmaMapMemory() returned {}", rutils::toString(res)
            );
        }
        std::memcpy(texPtr, texCoords.data(), texSize);
        vmaUnmapMemory(allocator.allocator, texStaging.allocation);

        void* normalsPtr = nullptr;
        if (auto const res = vmaMapMemory(allocator.allocator, normalsStaging.allocation, &normalsPtr); VK_SUCCESS != res) {
            throw FatalError( "Mapping memory for writing\n"
                "vmaMapMemory() returned {}", rutils::toString(res)
            );
        }
        std::memcpy(normalsPtr, normals.data(), normalsSize);
        vmaUnmapMemory(allocator.allocator, normalsStaging.allocation);

        // We need to ensure that the Vulkan resources are alive until all the transfers have completed. For simplicity,
        // we will just wait for the operations to complete with a fence. A more complex solution might want to queue
        // transfers, let these take place in the background while performing other tasks.
        rutils::Fence uploadComplete = rutils::createFence(window.device);

        // Queue data uploads from staging buffers to the final buffers.
        // This uses a separate command pool for simplicity.
        rutils::CommandPool uploadPool = rutils::createCommandPool(window);
        VkCommandBuffer uploadCmd = rutils::allocCommandBuffer(window, uploadPool.handle);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;

        if (auto const res = vkBeginCommandBuffer(uploadCmd, &beginInfo); VK_SUCCESS != res) {
            throw FatalError( "Beginning command buffer recording\n"
                "vkBeginCommandBuffer() returned {}", rutils::toString(res)
            );
        }

        VkBufferCopy pcopy{};
        pcopy.size = posSize;

        vkCmdCopyBuffer(uploadCmd, posStaging.buffer, vertexPosGPU.buffer, 1, &pcopy);

        rutils::bufferBarrier(uploadCmd, vertexPosGPU.buffer,
            /* Before */
            VK_PIPELINE_STAGE_2_COPY_BIT,
            VK_ACCESS_2_TRANSFER_WRITE_BIT,
            /* A f t e r */
            VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT,
            VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT
        );

        VkBufferCopy icopy{};
        icopy.size = indSize;

        vkCmdCopyBuffer(uploadCmd, indStaging.buffer, indexGPU.buffer, 1, &icopy);

        rutils::bufferBarrier(uploadCmd, indexGPU.buffer,
            /* Before */
            VK_PIPELINE_STAGE_2_COPY_BIT,
            VK_ACCESS_2_TRANSFER_WRITE_BIT,
            /* A f t e r */
            VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
            VK_ACCESS_2_INDEX_READ_BIT
        );

        VkBufferCopy tcopy{};
        tcopy.size = texSize;

        vkCmdCopyBuffer(uploadCmd, texStaging.buffer, texCoordsGPU.buffer, 1, &tcopy);

        rutils::bufferBarrier(uploadCmd, texCoordsGPU.buffer,
            /* Before */
            VK_PIPELINE_STAGE_2_COPY_BIT,
            VK_ACCESS_2_TRANSFER_WRITE_BIT,
            /* A f t e r */
            VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT,
            VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT
        );

        VkBufferCopy ncopy{};
        ncopy.size = normalsSize;

        vkCmdCopyBuffer(uploadCmd, normalsStaging.buffer, normalsGPU.buffer, 1, &ncopy);

        rutils::bufferBarrier(uploadCmd, normalsGPU.buffer,
            /* Before */
            VK_PIPELINE_STAGE_2_COPY_BIT,
            VK_ACCESS_2_TRANSFER_WRITE_BIT,
            /* A f t e r */
            VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT,
            VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT
        );

    
        if (auto const res = vkEndCommandBuffer(uploadCmd); VK_SUCCESS != res) {
            throw FatalError( "Ending command buffer recording\n"
                "vkEndCommandBuffer() returned {}", rutils::toString(res)
            );
        }

        // Submit transfer commands
        VkCommandBufferSubmitInfo submit[1]{};
        submit[0].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        submit[0].commandBuffer = uploadCmd;

        VkSubmitInfo2 submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submitInfo.commandBufferInfoCount = 1;
        submitInfo.pCommandBufferInfos = submit;

        if (auto const res = vkQueueSubmit2(window.graphicsQueue, 1, &submitInfo, uploadComplete.handle); VK_SUCCESS != res) {
            throw FatalError( "Unable to submit command buffer to queue\n"
                "vkQueueSubmit2() returned {}", rutils::toString(res)
            );
        }

        // Wait for commands to finish before we destroy the temporary resources required for the transfers (staging
        // buffers, command pool, ...)
        //
        // The code doesn’t destory the resources implicitly – the resources are destroyed by the destructors of the
        // labutils wrappers for the various objects once we leave the function’s scope.
        if (auto const res = vkWaitForFences(window.device, 1, &uploadComplete.handle, VK_TRUE, std::numeric_limits<std::uint64_t>::max()); VK_SUCCESS != res) {
            throw FatalError( "Waiting for upload to complete\n"
                "vkWaitForFences() returned {}", rutils::toString(res)
            );
        }

        return Mesh(std::move(vertexPosGPU), std::move(texCoordsGPU), std::move(normalsGPU), std::move(indexGPU), positions.size() / 3, indices.size());
    }

    Mesh RenderManager::allocateSkyboxMesh(std::vector<float> positions, std::vector<std::uint32_t> indices) {
        int posSize = positions.size() * sizeof(float);
        int indSize = indices.size() * sizeof (std::uint32_t);

        // Create on GPU vertex buffer
        rutils::Buffer vertexPosGPU = rutils::createBuffer(
            allocator,
            posSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            0, // no additional VmaAllocationCreateFlags
            VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE // or just VMA MEMORY USAGE AUTO
        );

        // Create on GPU index buffer
        rutils::Buffer indexGPU = rutils::createBuffer(
            allocator,
            indSize,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            0, // no additional VmaAllocationCreateFlags
            VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE // or just VMA MEMORY USAGE AUTO
        );

        // Create staging buffers
        rutils::Buffer posStaging = rutils::createBuffer(
            allocator,
            posSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
        );

        rutils::Buffer indStaging = rutils::createBuffer(
            allocator,
            indSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
        );

        void* posPtr = nullptr;
        if (auto const res = vmaMapMemory(allocator.allocator, posStaging.allocation, &posPtr); VK_SUCCESS != res) {
            throw FatalError( "Mapping memory for writing\n"
                "vmaMapMemory() returned {}", rutils::toString(res)
            );
        }
        std::memcpy(posPtr, positions.data(), posSize);
        vmaUnmapMemory(allocator.allocator, posStaging.allocation);

        void* indPtr = nullptr;
        if (auto const res = vmaMapMemory(allocator.allocator, indStaging.allocation, &indPtr); VK_SUCCESS != res) {
            throw FatalError( "Mapping memory for writing\n"
                "vmaMapMemory() returned {}", rutils::toString(res)
            );
        }
        std::memcpy(indPtr, indices.data(), indSize);
        vmaUnmapMemory(allocator.allocator, indStaging.allocation);

        // We need to ensure that the Vulkan resources are alive until all the transfers have completed. For simplicity,
        // we will just wait for the operations to complete with a fence. A more complex solution might want to queue
        // transfers, let these take place in the background while performing other tasks.
        rutils::Fence uploadComplete = rutils::createFence(window.device);

        // Queue data uploads from staging buffers to the final buffers.
        // This uses a separate command pool for simplicity.
        rutils::CommandPool uploadPool = rutils::createCommandPool(window);
        VkCommandBuffer uploadCmd = rutils::allocCommandBuffer(window, uploadPool.handle);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;

        if (auto const res = vkBeginCommandBuffer(uploadCmd, &beginInfo); VK_SUCCESS != res) {
            throw FatalError( "Beginning command buffer recording\n"
                "vkBeginCommandBuffer() returned {}", rutils::toString(res)
            );
        }

        VkBufferCopy pcopy{};
        pcopy.size = posSize;

        vkCmdCopyBuffer(uploadCmd, posStaging.buffer, vertexPosGPU.buffer, 1, &pcopy);

        rutils::bufferBarrier(uploadCmd, vertexPosGPU.buffer,
            /* Before */
            VK_PIPELINE_STAGE_2_COPY_BIT,
            VK_ACCESS_2_TRANSFER_WRITE_BIT,
            /* After */
            VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT,
            VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT
        );

        VkBufferCopy icopy{};
        icopy.size = indSize;

        vkCmdCopyBuffer(uploadCmd, indStaging.buffer, indexGPU.buffer, 1, &icopy);

        rutils::bufferBarrier(uploadCmd, indexGPU.buffer,
            /* Before */
            VK_PIPELINE_STAGE_2_COPY_BIT,
            VK_ACCESS_2_TRANSFER_WRITE_BIT,
            /* After */
            VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
            VK_ACCESS_2_INDEX_READ_BIT
        );
    
        if (auto const res = vkEndCommandBuffer(uploadCmd); VK_SUCCESS != res) {
            throw FatalError( "Ending command buffer recording\n"
                "vkEndCommandBuffer() returned {}", rutils::toString(res)
            );
        }

        // Submit transfer commands
        VkCommandBufferSubmitInfo submit[1]{};
        submit[0].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        submit[0].commandBuffer = uploadCmd;

        VkSubmitInfo2 submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submitInfo.commandBufferInfoCount = 1;
        submitInfo.pCommandBufferInfos = submit;

        if (auto const res = vkQueueSubmit2(window.graphicsQueue, 1, &submitInfo, uploadComplete.handle); VK_SUCCESS != res) {
            throw FatalError( "Unable to submit command buffer to queue\n"
                "vkQueueSubmit2() returned {}", rutils::toString(res)
            );
        }

        // Wait for commands to finish before we destroy the temporary resources required for the transfers (staging
        // buffers, command pool, ...)
        //
        // The code doesn’t destory the resources implicitly – the resources are destroyed by the destructors of the
        // labutils wrappers for the various objects once we leave the function’s scope.
        if (auto const res = vkWaitForFences(window.device, 1, &uploadComplete.handle, VK_TRUE, std::numeric_limits<std::uint64_t>::max()); VK_SUCCESS != res) {
            throw FatalError( "Waiting for upload to complete\n"
                "vkWaitForFences() returned {}", rutils::toString(res)
            );
        }

        Mesh mesh;
        mesh.positions = std::move(vertexPosGPU);
        mesh.indices = std::move(indexGPU);
        mesh.indexCount = indices.size();
        mesh.vertexCount = positions.size() / 3;

        return mesh;
    }

    void RenderManager::recreatePipelines() {
        vkDeviceWaitIdle(window.device);
        pipelines = rutils::createAllPipelines(window, pipelineLayouts);
    }

    Material RenderManager::allocateMaterial(const Mtexture& textureData) {
        rutils::Image texture = rutils::loadImageTexture(textureData.rawDataPtr, textureData.width, textureData.height, window, tempTextureCmdPool.handle, allocator);
        rutils::Image roughnessMetalness = rutils::loadImageTexture(textureData.roughness, textureData.width, textureData.height, window, tempTextureCmdPool.handle, allocator);

        VkDescriptorImageInfo textureInfo[2]{};

        // base colour
        textureInfo[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        textureInfo[0].imageView = texture.view;
        textureInfo[0].sampler = sampler.handle;

        // roughness and metalness
        textureInfo[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        textureInfo[1].imageView = roughnessMetalness.view;
        textureInfo[1].sampler = sampler.handle;

        VkDescriptorSet descriptorSet = rutils::allocDescSet(window, descriptorPool.handle, materialLayout.handle);

        VkWriteDescriptorSet desc[2]{};
        desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        desc[0].dstSet = descriptorSet;
        desc[0].dstBinding = 0;
        desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        desc[0].descriptorCount = 1;
        desc[0].pImageInfo = &textureInfo[0];

        desc[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        desc[1].dstSet = descriptorSet;
        desc[1].dstBinding = 1;
        desc[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        desc[1].descriptorCount = 1;
        desc[1].pImageInfo = &textureInfo[1];

        constexpr auto numSets = sizeof(desc) / sizeof(desc[0]);
        vkUpdateDescriptorSets( window.device, numSets, desc, 0, nullptr );

        return Material(std::move(texture), std::move(roughnessMetalness), std::move(descriptorSet));
    }

    void RenderManager::updateSceneUniforms(SceneUniform& aSceneUniforms, std::uint32_t aFramebufferWidth, std::uint32_t aFramebufferHeight) {
        float const aspect = aFramebufferWidth / float(aFramebufferHeight);
		CameraComponent camComp;
        TransformComponent transformComp;

        if (debugCam.enabled) {
            if (registry.valid(debugCam.camera)) {
                if (registry.all_of<CameraComponent>(debugCam.camera))
                    camComp = registry.get<CameraComponent>(debugCam.camera);
                if (registry.all_of<TransformComponent>(debugCam.camera))
                    transformComp = registry.get<TransformComponent>(debugCam.camera);
            } else {
                debugCam.reset();
            }
        } else {
            auto cameras = world.Query<CameraComponent>();

            for (auto cam : cameras) {
                auto comp = registry.get<CameraComponent>(cam);

                if (comp.isMain) {
                    camComp = comp;
                    if (registry.all_of<TransformComponent>(cam))
                        transformComp = registry.get<TransformComponent>(cam);
                    break;
                }
            }
        }

        aSceneUniforms.projection = glm::perspectiveRH_ZO(
            glm::radians(camComp.fov), // fov
            aspect,
            camComp.nearPlane, // near
            camComp.farPlane // far
        );

        aSceneUniforms.projection[1][1] *= -1.f; // mirror Y axis

        aSceneUniforms.camera = glm::inverse(transformComp.worldMatrix); 

        aSceneUniforms.projCam = aSceneUniforms.projection * aSceneUniforms.camera;

        aSceneUniforms.lightPos = glm::vec4(0.f, 5.f, 0.f, 0.f);
        aSceneUniforms.lightColour = glm::vec4(1.f, 1.f, 1.f, 1.f);
        aSceneUniforms.cameraPos = glm::vec4(transformComp.position, 0.f);
    }

    void RenderManager::setDebugInterfaceInit(ImGui_ImplVulkan_InitInfo& info) {
        info.Instance = window.instance;
        info.PhysicalDevice = window.physicalDevice;
        info.Device = window.device;
        info.QueueFamily = window.presentFamilyIndex;
        info.Queue = window.presentQueue;
        info.PipelineCache = nullptr;
        info.DescriptorPool = descriptorPool.handle;
        info.MinImageCount = 2;
        info.ImageCount = commandBuffers.size();
        info.Allocator = nullptr;
        info.UseDynamicRendering = true;
        info.PipelineInfoMain.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR };
        info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &window.swapchainFormat;
        info.PipelineInfoMain.RenderPass = nullptr;
        info.PipelineInfoMain.Subpass = 0;
        info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        info.CheckVkResultFn = [](VkResult err) {
            if (err == 0)
                return;
            spdlog::error("DebugInterface Vulkan Error: {}", rutils::toString(err));
            if (err < 0) 
                abort();
        };
    }

    void RenderManager::setCustomSkybox(
        std::filesystem::path right,
        std::filesystem::path left,
        std::filesystem::path up,
        std::filesystem::path down,
        std::filesystem::path front,
        std::filesystem::path back
    ) {
        skybox.paths.right = right;
        skybox.paths.left = left;
        skybox.paths.top = up;
        skybox.paths.bottom = down;
        skybox.paths.front = front;
        skybox.paths.back = back;

        createSkybox(skybox.paths);
        initialiseDeferredLightingDescriptorSet(window, gbuffers, depthBuffer, sampler, deferredLightingDescriptors, skybox.cubemap, skybox.sampler);
    }

    void RenderManager::createSkybox(const rutils::CubemapPaths& paths) {
        skybox.descriptorSet = rutils::allocDescSet(window, descriptorPool.handle, cubemapLayout.handle);

        std::array<stbi_uc*, 6> faces;
        int w, h, c;
        stbi_set_flip_vertically_on_load(false);
        faces[0] = stbi_load(paths.right.string().c_str(), &w, &h, &c, 4);
        faces[1] = stbi_load(paths.left.string().c_str(), &w, &h, &c, 4);
        faces[2] = stbi_load(paths.top.string().c_str(), &w, &h, &c, 4);
        faces[3] = stbi_load(paths.bottom.string().c_str(), &w, &h, &c, 4);
        faces[4] = stbi_load(paths.front.string().c_str(), &w, &h, &c, 4);
        faces[5] = stbi_load(paths.back.string().c_str(), &w, &h, &c, 4);

        skybox.cubemap = rutils::loadCubemapTexture(faces, w, h, window, tempTextureCmdPool.handle, allocator);
        skybox.sampler = rutils::createSampler(window, true);

        for (auto* face : faces) {
            stbi_image_free(face);
        }

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = skybox.cubemap.view;
        imageInfo.sampler = skybox.sampler.handle;

        VkWriteDescriptorSet desc{};
        desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        desc.dstSet = skybox.descriptorSet;
        desc.dstBinding = 0;
        desc.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        desc.descriptorCount = 1;
        desc.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(window.device, 1, &desc, 0, nullptr);

        std::vector<float> skyboxVertices = {
            -1, -1, -1,
             1, -1, -1,
             1,  1, -1,
            -1,  1, -1,
            -1, -1,  1,
             1, -1,  1,
             1,  1,  1,
            -1,  1,  1,
        };

        std::vector<uint32_t> skyboxIndices = {
            0, 1, 2, 2, 3, 0, // back
            5, 4, 7, 7, 6, 5, // front
            4, 0, 3, 3, 7, 4, // left
            1, 5, 6, 6, 2, 1, // right
            4, 5, 1, 1, 0, 4, // bottom
            3, 2, 6, 6, 7, 3 // top
        };

        skybox.mesh = allocateSkyboxMesh(skyboxVertices, skyboxIndices);
    }

    void RenderManager::shutdown() {
        vkDeviceWaitIdle(window.device);

        SceneManager::get().shutdown();

        #ifndef NDEBUG
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        #endif

        skybox = {};

        noTextureDst = {};
        noTexture = {};

        gbuffers = {};
        depthBuffer = {};

        tempTextureCmdPool = {};

        renderFinished.clear();
        imageAvailable.clear();
        frameDone.clear();

        if (!commandBuffers.empty()) {
            vkFreeCommandBuffers(
                window.device,
                commandPool.handle,
                commandBuffers.size(),
                commandBuffers.data()
            );
            commandBuffers.clear();
        }

        commandPool = {};

        pipelines.pbr = {};
        pipelines.pbr_alpha = {};
        pipelines.deferred_geometry = {};
        pipelines.deferred_geometry_alpha = {};
        pipelines.deferred_lighting = {};

        pipelineLayouts.pbrPipelineLayout = {};
        pipelineLayouts.deferredPipelineLayout = {};
        pipelineLayouts.skyboxPipelineLayout = {};

        depthBuffer = {};
        sceneUBO = {};

        gBufferLayout = {};
        sceneLayout = {};
        materialLayout = {};
        cubemapLayout = {};

        descriptorPool = {};
        sceneDescriptors = {};

        sampler = {};
        allocator = {};

        window = {};
    }
}
