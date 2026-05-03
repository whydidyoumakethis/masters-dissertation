# Kiki Runner game
add_executable(KikiRunner)

target_sources(KikiRunner
  PRIVATE
    main.cpp
    systems/UISystem.hpp
    systems/LevelSystem.hpp
    events/LevelLoadedEvent.hpp
    events/RequestLevelChangeEvent.hpp
)
target_link_libraries(KikiRunner
  PRIVATE
    kiki
)