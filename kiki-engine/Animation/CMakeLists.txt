target_sources(kiki
  PRIVATE
	AnimationLoader.cpp
	Animator.cpp
  PUBLIC
	FILE_SET animationPublicHeaders
	TYPE HEADERS
	FILES 
	  Animation.h
	  Animator.h
	  AnimationLoader.h
	  Skeleton.h
)