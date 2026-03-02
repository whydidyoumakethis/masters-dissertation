target_sources(kiki
  PRIVATE
    InputManager.cpp

  PUBLIC
    FILE_SET inputPublicHeaders
    TYPE HEADERS
    FILES 
      InputManager.hpp
)