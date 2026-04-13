target_sources(kiki
  PRIVATE
    DebugCamera.cpp
    DebugInterface.cpp
    windows/EntityViewer.cpp
    
  PUBLIC
    FILE_SET debuggingPublicHeaders
    TYPE HEADERS
    FILES 
      DebugCamera.hpp
      DebugInterface.hpp

  PRIVATE
    FILE_SET debuggingPrivateHeaders
    TYPE HEADERS
    FILES 
      windows/EntityViewer.hpp
      windows/Log.hpp
)