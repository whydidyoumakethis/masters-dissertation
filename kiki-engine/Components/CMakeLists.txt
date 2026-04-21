target_sources(kiki
  PUBLIC
    FILE_SET componentsPublicHeaders
    TYPE HEADERS
    FILES 
      MaterialComponent.hpp
      MeshComponent.hpp
      TransparencyComponent.hpp
      ColourComponent.hpp
      CameraComponent.h
      DebugComponent.hpp
      MiscComponent.hpp
)