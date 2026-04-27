target_sources(kiki
  PRIVATE
    InterfaceSystem.cpp
    TextureManager.cpp
    FontManager.cpp
    
  PUBLIC
    FILE_SET interfacePublicHeaders
    TYPE HEADERS
    FILES 
      InterfaceSystem.hpp
      TextureManager.hpp
      FontManager.hpp   
      events/ButtonClickEvent.hpp
      events/ButtonHoverEvent.hpp   

  PRIVATE
    FILE_SET interfacePrivateHeaders
    TYPE HEADERS
    FILES 
      utils/Font.hpp     
)