# Kiki Engine
add_library(kiki)

target_sources(kiki
  PRIVATE
  PUBLIC
    FILE_SET HEADERS
    FILES 
      Event/Event.h
      Timer/Timer.h
      ECS/World.h
 "ECS/GameObject.h" "ECS/System.h" "Engine.h"
      
      GltfLoader/GltfLoaderAssimp.h
      MessageCenter.h
)

add_dependencies(kiki shaders)

add_subdirectory(renderer)

add_subdirectory(logging)

add_subdirectory(input)

add_subdirectory(physics)

add_subdirectory(Components)

add_subdirectory(debugging)

add_subdirectory(interface)

add_subdirectory(Animation)

target_link_libraries(kiki
  PUBLIC
    KikiDeps
)