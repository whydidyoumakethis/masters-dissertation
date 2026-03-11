# Kiki Engine
add_library(kiki)

target_sources(kiki
  PRIVATE
    temp.cpp
    GltfLoader/GltfLoader.cpp
  PUBLIC
    FILE_SET HEADERS
    FILES 
      temp.h
      Event/Event.h
      Timer/Timer.h
      ECS/World.h
 "ECS/GameObject.h" "ECS/System.h" "Engine.h"
      
      GltfLoader/GltfLoader.hpp)

add_dependencies(kiki shaders)

add_subdirectory(renderer)

add_subdirectory(logging)

add_subdirectory(input)

add_subdirectory(physics)

add_subdirectory(Components)

target_link_libraries(kiki
  PUBLIC
    KikiDeps
)