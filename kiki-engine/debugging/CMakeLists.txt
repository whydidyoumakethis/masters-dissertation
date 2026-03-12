target_sources(kiki
  PRIVATE
    DebugCamera.cpp
    
  PUBLIC
    FILE_SET debuggingPublicHeaders
    TYPE HEADERS
    FILES 
      DebugCamera.hpp
)