# Kiki Engine
add_library(kiki)

target_sources(kiki
  PRIVATE
    temp.cpp
    
  PUBLIC
    FILE_SET HEADERS
    FILES 
      temp.h
      Event/Event.h
      kiki.h "ImGui/ImGui.h" "SceneManager/SceneManager.h" "Timer/Timer.h")

target_link_libraries(kiki
  PUBLIC
    KikiDeps
)