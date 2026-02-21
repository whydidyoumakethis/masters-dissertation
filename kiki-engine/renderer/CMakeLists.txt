target_sources(kiki
  PRIVATE
    Renderer.cpp
    utils/Devices.cpp
    utils/ToString.cpp
    utils/VulkanWindow.cpp

  PRIVATE
    FILE_SET privateHeaders
    TYPE HEADERS
    FILES
      utils/Devices.hpp
      utils/ToString.hpp
      utils/VulkanWindow.hpp

  PUBLIC
    FILE_SET publicHeaders
    TYPE HEADERS
    FILES 
      Renderer.hpp
)