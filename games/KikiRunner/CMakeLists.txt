# Kiki Runner game
add_executable(KikiRunner)

target_sources(KikiRunner
  PRIVATE
    main.cpp
    systems/UISystem.hpp
)
target_link_libraries(KikiRunner
  PRIVATE
    kiki
)