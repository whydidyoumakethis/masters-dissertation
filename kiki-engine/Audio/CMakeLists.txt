target_sources(kiki
  PRIVATE
	MiniaudioImpl.cpp
	
  PUBLIC
	FILE_SET audioPublicHeaders
	TYPE HEADERS
	FILES 
	  AudioSystem.h
	  AudioManager.h
	  
	  
)