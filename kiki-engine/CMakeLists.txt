# Kiki Engine
add_library(kiki)

target_sources(kiki
  PRIVATE
    temp.cpp
    renderer/render.cpp
    utils/*.cpp
  
  PUBLIC
    FILE_SET HEADERS
    FILES 
      temp.h
      renderer/render.hpp
      utils/.hpp
)