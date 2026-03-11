target_sources(kiki
  PRIVATE
    InputSystem.cpp
    InputManager.cpp

  PUBLIC
    FILE_SET inputPublicHeaders
    TYPE HEADERS
    FILES
      InputSystem.hpp
      InputManager.hpp
)