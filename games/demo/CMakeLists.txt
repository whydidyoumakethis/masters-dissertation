# Demo game
add_executable(demo)

target_sources(demo
  PRIVATE
    main.cpp
 "system/characterSystem.h" "component/CharacterComponent.h" "component/ThirdPersonCameraComponent.hpp" "system/ThirdPersonCameraSystem.h" "system/GoalTriggerSystem.h" "system/TimeLimitSystem.h")

target_link_libraries(demo
  PRIVATE
    kiki
)