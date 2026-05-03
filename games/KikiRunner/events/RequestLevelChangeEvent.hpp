#ifndef KIKIRUNNER_REQUESTLEVELCHANGEEVENT
#define KIKIRUNNER_REQUESTLEVELCHANGEEVENT

#include <filesystem>

struct RequestLevelChangeEvent {
	std::filesystem::path levelPath;
};

#endif