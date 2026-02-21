# Kiki Engine
add_library(kiki)

target_link_libraries(kiki
  PRIVATE
    KikiDeps
)

target_sources(kiki
  PRIVATE
    temp.cpp
  
  PUBLIC
    FILE_SET HEADERS
    FILES 
      temp.h
)

add_subdirectory(renderer)

add_subdirectory(logging)