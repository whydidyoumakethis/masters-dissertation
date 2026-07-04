# Kiki Runner game
add_executable(demo)

target_sources(demo
  PRIVATE
    main.cpp

  PUBLIC
    FILE_SET demoPublicHeaders
    TYPE HEADERS
    FILES
      DemoCameraPathSystem.hpp
)

target_link_libraries(demo
  PRIVATE
    kiki
)
