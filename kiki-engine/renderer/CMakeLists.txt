target_sources(kiki
  PRIVATE
    Renderer.cpp
    utils/Devices.cpp
    utils/ToString.cpp
    utils/VulkanWindow.cpp
    utils/Pipelines.cpp
    utils/Shaders.cpp

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

  PUBLIC
    FILE_SET rendererPublicHeaders
    TYPE HEADERS
    FILES 
      Renderer.hpp
)