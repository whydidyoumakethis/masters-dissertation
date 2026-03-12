#ifndef KIKI_RENDERER
#define KIKI_RENDERER

#include "../Components/MaterialComponent.hpp"
#include "../Components/MeshComponent.hpp"
#include "utils/VulkanWindow.hpp"
#include "utils/VulkanWrapper.hpp"
#include "utils/Synchronisation.hpp"
#include "utils/Allocator.hpp"
#include "utils/Buffer.hpp"
#include "utils/Texture.hpp"
#include "WindowInfo.hpp"
#include "Camera.hpp"


#include <glm/glm.hpp>
#include <stb_image.h>



#define GLM_ENABLE_EXPERIMENTAL


namespace Kiki {
    enum BlendMode {
        OPAQUE,
        TRANSPARENT
    };
    
    struct Mesh {
        rutils::Buffer positions;
        rutils::Buffer texCoords;
        rutils::Buffer indices;
        std::uint32_t vertexCount;
        std::uint32_t indexCount;
    };

    struct Material {
        rutils::Texture texture;
        VkDescriptorSet descriptorSet;
        BlendMode blendMode;
    };


    class RenderManager {
        private:
        RenderManager() = default;
        ~RenderManager() = default;
        RenderManager(const RenderManager&) = delete;
        RenderManager& operator=(const RenderManager&) = delete;

        bool recreateSwapchain = false;
        bool initialised = false;

        rutils::VulkanWindow window;

        rutils::PipelineLayout pipelineLayout;
        rutils::Pipeline pipeline;
        rutils::CommandPool commandPool;

        rutils::Buffer sceneUBO;

        std::size_t frameIndex = 0;
        std::vector<VkCommandBuffer> commandBuffers;
        std::vector<rutils::Fence> frameDone;
        std::vector<rutils::Semaphore> imageAvailable, renderFinished;
        rutils::DescriptorPool descriptorPool;
        rutils::DescriptorSetLayout objectLayout;
        VkDescriptorSet sceneDescriptors;

        rutils::Allocator allocator;
        rutils::Sampler sampler;

        Mesh tempMesh;
        Material tempMaterial;

        Camera camera; // default cam

        public:
        static RenderManager& get();
        void initialise(WindowInfo info = Kiki::WindowInfo{});

        Mesh allocateMesh(std::vector<float> positions, std::vector<std::uint32_t> indices, std::vector<float> texCoords);
        Material allocateMaterial(stbi_uc* imageData, int baseWidthi, int baseHeighti, BlendMode blendMode);
        
        void draw(MeshComponent meshComponent, MaterialComponent materialComponent, glm::mat4 transformMatrix);
        void nextFrame();
        void shutdown();

        void setCamera(Camera&);

        VkDevice& getDevice() { return window.device; };
        GLFWwindow* getWindow() { return window.window; };
        bool isInitialised() { return initialised; };

        struct SceneUniform {
            glm::mat4 camera;
            glm::mat4 projection;
            glm::mat4 projCam;
        };

        rutils::CommandPool tempTextureCmdPool;

        // We want to use vkCmdUpdateBuffer() to update the contents of our uniform buffers. vkCmdUpdateBuffer()
        // has a number of requirements, including the two below. See
        // https://registry.khronos.org/vulkan/specs/latest/man/html/vkCmdUpdateBuffer.html
        static_assert(sizeof(SceneUniform) <= 65536, "SceneUniform must be less than 65536 bytes for vkCmdUpdateBuffer");
        static_assert(sizeof(SceneUniform) % 4 == 0, "SceneUniform size must be a multiple of 4 bytes");

        private:
        void updateSceneUniforms(SceneUniform& aSceneUniforms, std::uint32_t aFramebufferWidth, std::uint32_t aFramebufferHeight);
    };
}

#endif