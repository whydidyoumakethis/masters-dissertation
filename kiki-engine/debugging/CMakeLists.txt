target_sources(kiki
  PRIVATE
    DebugCamera.cpp
    DebugInterface.cpp
    
  PUBLIC
    FILE_SET debuggingPublicHeaders
    TYPE HEADERS
    FILES 
      DebugCamera.hpp
      DebugInterface.hpp
)