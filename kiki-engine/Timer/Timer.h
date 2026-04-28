#pragma once
#include <chrono>
class Timer {
	public:
	Timer() : _startTime(std::chrono::steady_clock::now()) {}
	// Resets the timer to zero
	void Reset() {
		_startTime = std::chrono::steady_clock::now();
	}
	// Returns the elapsed time in seconds since the last reset
	float Elapsed() const {
		auto now = std::chrono::steady_clock::now();
		return std::chrono::duration_cast<std::chrono::duration<float>>(now - _startTime).count();
	}
	float Tick() {
		auto now = std::chrono::steady_clock::now();
		float deltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(now - _lastTickTime).count();
		_lastTickTime = now;
		return deltaTime;
	}
		
	private:
	std::chrono::steady_clock::time_point _startTime;
	std::chrono::steady_clock::time_point _lastTickTime;

};