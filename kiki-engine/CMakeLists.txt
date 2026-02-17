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