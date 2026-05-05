#ifndef KIKIRUNNER_REQUESTLEVELCHANGEEVENT
#define KIKIRUNNER_REQUESTLEVELCHANGEEVENT

#include <filesystem>
#include <vector>

struct RequestLevelChangeEvent {
	std::vector<std::filesystem::path> levelPaths;
};

#endif