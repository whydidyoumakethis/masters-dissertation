target_sources(kiki
  PRIVATE
    RenderManager.cpp
    SceneManager.cpp
    utils/VMA.cpp
    utils/Devices.cpp
    utils/ToString.cpp
    utils/VulkanWindow.cpp
    utils/Pipelines.cpp
    utils/Shaders.cpp
    utils/Commands.cpp
    utils/Synchronisation.cpp
    utils/Allocator.cpp
    utils/Buffer.cpp
    utils/Descriptors.cpp
    utils/Texture.cpp

  PRIVATE
    FILE_SET rendererPrivateHeaders
    TYPE HEADERS
    FILES
      utils/Devices.hpp
      utils/ToString.hpp
      utils/VulkanWindow.hpp
      utils/VulkanWrapper.hpp
      utils/Pipelines.hpp
      utils/Shaders.hpp
      utils/Commands.hpp
      utils/Synchronisation.hpp
      utils/Buffer.hpp
      utils/Allocator.hpp
      utils/Descriptors.hpp
      utils/Texture.hpp

  PUBLIC
    FILE_SET rendererPublicHeaders
    TYPE HEADERS
    FILES 
      RenderManager.hpp
      SceneManager.hpp
      WindowInfo.hpp
      Camera.hpp
)