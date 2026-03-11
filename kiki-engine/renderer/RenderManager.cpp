#include "RenderManager.hpp"

#include "utils/VulkanWindow.hpp"
#include "utils/VulkanWrapper.hpp"
#include "utils/Pipelines.hpp"
#include "utils/Commands.hpp"
#include "utils/Synchronisation.hpp"
#include "utils/ToString.hpp"
#include "utils/Descriptors.hpp"
#include "utils/Texture.hpp"
#include "../logging/FatalError.hpp"
#include "MaterialManager.hpp"

#include <iostream>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>


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
            rutils::DescriptorSetLayout sceneLayout = rutils::createSceneDescriptorLayout(window);
            objectLayout = rutils::createObjectDescriptorLayout(window);
            allocator = rutils::createAllocator(window);

            sampler = rutils::createSampler(window);

            sceneUBO = rutils::createBuffer(
                allocator,
                sizeof(SceneUniform),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                0,
                VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
            );

            pipelineLayout = rutils::createPipelineLayout(window, sceneLayout.handle, objectLayout.handle);
            pipeline = rutils::createPipeline(window, pipelineLayout.handle);
            commandPool = rutils::createCommandPool(window, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

            descriptorPool = rutils::createDescriptorPool(window);

            sceneDescriptors = rutils::allocDescSet(window, descriptorPool.handle, sceneLayout.handle );

            VkWriteDescriptorSet desc[1]{};

            VkDescriptorBufferInfo sceneUboInfo{};
            sceneUboInfo.buffer = sceneUBO.buffer;
            sceneUboInfo.range = VK_WHOLE_SIZE;

            desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            desc[0].dstSet = sceneDescriptors;
            desc[0].dstBinding = 0;
            desc[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            desc[0].descriptorCount = 1;
            desc[0].pBufferInfo = &sceneUboInfo;


            constexpr auto numSets = sizeof(desc)/sizeof(desc[0]);
            vkUpdateDescriptorSets( window.device, numSets, desc, 0, nullptr );


            for (std::size_t i = 0; i < window.swapImages.size(); i++) {
                commandBuffers.emplace_back(rutils::allocCommandBuffer(window, commandPool.handle));
                frameDone.emplace_back(rutils::createFence(window.device, VK_FENCE_CREATE_SIGNALED_BIT));
                imageAvailable.emplace_back(rutils::createSemaphore(window.device));
                renderFinished.emplace_back(rutils::createSemaphore(window.device));
            }

            std::vector<float> p = {
                -1.f, 0.f, -6.f, // v0
                -1.f, 0.f, +6.f, // v1
                +1.f, 0.f, +6.f, // v2

                -1.f, 0.f, -6.f, // v0
                +1.f, 0.f, +6.f, // v2
                +1.f, 0.f, -6.f // v3
            };
            std::vector<float> c = {
                0.f, -6.f, // t0
                0.f, +6.f, // t1
                1.f, +6.f, // t2

                0.f, -6.f, // t0
                1.f, +6.f, // t2
                1.f, -6.f // t3
            };

            
            tempMesh = RenderManager::get().allocateMesh(p, c);

            tempTextureCmdPool = rutils::createCommandPool(window, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
            
            MaterialManager::get().createMaterial(std::filesystem::path(PROJECT_ROOT_PATH) / "games/demo/assets/asphalt.png", BlendMode::OPAQUE);
            
        }
    }

    void RenderManager::draw(MeshComponent meshComponent, MaterialComponent materialComponent, glm::mat4 transformMatrix) {

    }

    void RenderManager::nextFrame() {
        glfwPollEvents();

        if (recreateSwapchain) {
            // We need to destroy several objects, which may still be in use by the GPU. Therefore, first wait for the GPU
            // to finish processing.
            vkDeviceWaitIdle(window.device);

            // Recreate resources
            rutils::recreateSwapchain(window);

            pipeline = rutils::createPipeline(window, pipelineLayout.handle);
            
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

        if (VK_SUBOPTIMAL_KHR == acquireRes || VK_ERROR_OUT_OF_DATE_KHR == acquireRes) {
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

            return;
        }

        if (VK_SUCCESS != acquireRes) {
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

        assert(std::size_t(imageIndex) < window.swapImages.size());

        rutils::ImageAndView colorTarget;
        colorTarget.image = window.swapImages[imageIndex];
        colorTarget.view = window.swapViews[imageIndex];

        assert(std::size_t(frameIndex) < commandBuffers.size());

        rutils::recordCommands(
            commandBuffers[frameIndex],
            pipeline.handle,
            colorTarget,
            window.swapchainExtent,
            tempMesh.positions.buffer,
            tempMesh.texCoords.buffer,
            tempMesh.vertexCount,
            sceneUBO.buffer,
            sceneUniforms,
            pipelineLayout.handle,
            sceneDescriptors,
            MaterialManager::get().getMaterial(0).descriptorSet
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

    Mesh RenderManager::allocateMesh(std::vector<float> positions, std::vector<float> texCoords) {
        int posSize = positions.size() * sizeof(float);
        int texSize = texCoords.size() * sizeof(float);

        // Create on GPU vertex buffer
        rutils::Buffer vertexPosGPU = rutils::createBuffer(
            allocator,
            posSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
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

        // Create staging buffers
        rutils::Buffer posStaging = rutils::createBuffer(
            allocator,
            posSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
        );

        rutils::Buffer texStaging = rutils::createBuffer(
            allocator,
            texSize,
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

        void* texPtr = nullptr;
        if (auto const res = vmaMapMemory(allocator.allocator, texStaging.allocation, &texPtr); VK_SUCCESS != res) {
            throw FatalError( "Mapping memory for writing\n"
                "vmaMapMemory() returned {}", rutils::toString(res)
            );
        }
        std::memcpy(texPtr, texCoords.data(), texSize);
        vmaUnmapMemory(allocator.allocator, texStaging.allocation);

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

        return Mesh(std::move(vertexPosGPU), std::move(texCoordsGPU), positions.size());
    }

    Material RenderManager::allocateMaterial(std::filesystem::path texturePath, BlendMode blendMode) {
        rutils::Texture texture = rutils::loadImageTexture(texturePath.c_str(), window, tempTextureCmdPool.handle, allocator);
        
        VkDescriptorSet descriptorSet = rutils::allocDescSet( window, descriptorPool.handle, objectLayout.handle );

        {
            VkWriteDescriptorSet desc[1]{};

            VkDescriptorImageInfo textureInfo{};
            textureInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            textureInfo.imageView = texture.view;
            textureInfo.sampler = sampler.handle;

            desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            desc[0].dstSet = descriptorSet;
            desc[0].dstBinding = 0;
            desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            desc[0].descriptorCount = 1;
            desc[0].pImageInfo = &textureInfo;

            constexpr auto numSets = sizeof(desc)/sizeof(desc[0]);
            vkUpdateDescriptorSets( window.device, numSets, desc, 0, nullptr );
        }

        return Material(std::move(texture), std::move(descriptorSet), blendMode);
    }

    void RenderManager::shutdown() {
        vkDeviceWaitIdle(window.device);

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
        pipeline = {};
        pipelineLayout = {};

        window = {};
    }

    void RenderManager::updateSceneUniforms(SceneUniform& aSceneUniforms, std::uint32_t aFramebufferWidth, std::uint32_t aFramebufferHeight) {
        float const aspect = aFramebufferWidth / float(aFramebufferHeight);

        aSceneUniforms.projection = glm::perspectiveRH_ZO(
            glm::radians(60.0f), // fov
            aspect,
            0.1f, // near
            100.0f // far
        );

        aSceneUniforms.projection[1][1] *= -1.f; // mirror Y axis

        aSceneUniforms.camera = glm::translate(glm::vec3( 0.f, -0.3f, -1.f));

        aSceneUniforms.projCam = aSceneUniforms.projection * aSceneUniforms.camera;
    }
}
