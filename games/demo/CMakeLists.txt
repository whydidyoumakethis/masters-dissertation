# Demo game
add_executable(demo)

target_sources(demo
  PRIVATE
    main.cpp
 "system/characterSystem.h" "component/CharacterComponent.h" "component/ThirdPersonCameraComponent.hpp")

target_link_libraries(demo
  PRIVATE
    kiki
)