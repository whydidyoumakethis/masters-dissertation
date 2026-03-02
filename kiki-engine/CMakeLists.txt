# Kiki Engine
add_library(kiki)

target_sources(kiki
  PRIVATE
    temp.cpp
    
  PUBLIC
    FILE_SET HEADERS
    FILES 
      temp.h
)

add_dependencies(kiki shaders)

add_subdirectory(renderer)

add_subdirectory(logging)

add_subdirectory(input)
target_link_libraries(kiki
  PUBLIC
    KikiDeps
)