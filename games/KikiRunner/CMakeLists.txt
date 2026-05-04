# Kiki Runner game
add_executable(KikiRunner)

target_sources(KikiRunner
  PRIVATE
    main.cpp

    PUBLIC
      FILE_SET kikiRunnerPublicHeaders
      TYPE HEADERS
      FILES
        systems/UISystem.hpp
        systems/LevelSystem.hpp
        systems/CharacterSystem.h
        systems/GoalTriggerSystem.h
        systems/ThirdPersonCameraSystem.h
        systems/TimeLimitSystem.h
        events/LevelLoadedEvent.hpp
        events/RequestLevelChangeEvent.hpp
        events/TimerTriggerEvent.h
        events/ResetLevelEvent.hpp
        events/RespawnCharacterEvent.hpp
        events/ObjectiveAchievedEvent.hpp
        events/ResetThirdPersonCameraEvent.hpp
        components/CharacterComponent.h
        components/ThirdPersonCameraComponent.hpp
)

target_link_libraries(KikiRunner
  PRIVATE
    kiki
)