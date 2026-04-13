target_sources(kiki
  PRIVATE
    DebugCamera.cpp
    DebugInterface.cpp
    DebugSystem.cpp
    windows/EntityViewer.cpp
    
  PUBLIC
    FILE_SET debuggingPublicHeaders
    TYPE HEADERS
    FILES 
      DebugCamera.hpp
      DebugInterface.hpp
      DebugSystem.hpp

  PRIVATE
    FILE_SET debuggingPrivateHeaders
    TYPE HEADERS
    FILES 
      windows/EntityViewer.hpp
      windows/Log.hpp
)