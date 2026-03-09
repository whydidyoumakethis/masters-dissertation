target_sources(kiki
  PRIVATE
    RenderManager.cpp
    utils/Devices.cpp
    utils/ToString.cpp
    utils/VulkanWindow.cpp
    utils/Pipelines.cpp
    utils/Shaders.cpp
    utils/Commands.cpp
    utils/Synchronisation.cpp

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

  PUBLIC
    FILE_SET rendererPublicHeaders
    TYPE HEADERS
    FILES 
      RenderManager.hpp
      WindowInfo.hpp
)