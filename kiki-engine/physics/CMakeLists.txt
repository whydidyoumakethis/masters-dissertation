target_sources(kiki
  PRIVATE
	PhysicsManager.cpp
	PhysicsSystem.cpp
	PhysicsContactListener.cpp
	PhysicsDebugRenderer.cpp

  PUBLIC
	FILE_SET physicsPublicHeaders
	TYPE HEADERS
	FILES 
	  PhysicsManager.hpp
	  PhysicsUtils.hpp
	  PhysicsComponents.hpp
	  PhysicsSystem.hpp
	  PhysicsContactListener.hpp
	  PhysicsDebugRenderer.hpp
)