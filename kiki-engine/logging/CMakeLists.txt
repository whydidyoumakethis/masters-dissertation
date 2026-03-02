target_sources(kiki
  PRIVATE
    Logger.cpp
    FatalError.cpp

  PRIVATE
    FILE_SET loggingPrivateHeaders
    TYPE HEADERS
    FILES
      FatalError.hpp

  PUBLIC
    FILE_SET loggingPublicHeaders
    TYPE HEADERS
    FILES 
      Logger.hpp
)