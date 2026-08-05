#pragma once
#include <chrono>
class Timer {
	public:

	enum class Mode {
			RealTime,
			Fixed
	};

	void Pause() {
		if (_paused) return;

		_pauseTime = std::chrono::steady_clock::now();
		_paused = true;
	}

	void Resume() {
		if (!_paused) return;

		const auto now = std::chrono::steady_clock::now();

		if (_mode == Mode::RealTime) {
			// Prevent Elapsed() from including the paused duration.
			_startTime += now - _pauseTime;

			// Prevent the next Tick() from returning the whole pause.
			_lastTickTime = now;
		}

		_paused = false;
	}

	static Timer& get() {
		static Timer instance;
		return instance;
	}

	void UseFixedTime(float fixedDelta) {
		_mode = Mode::Fixed;
		_fixedDelta = fixedDelta;
		_fixedElapsed = 0.0f;
		_paused = false;
	}

	float Step() {
		if (_mode != Mode::Fixed) {
			return 0.0f;
		}

		_fixedElapsed += _fixedDelta;
		return _fixedDelta;
	}

	void UseRealTime() {
		_mode = Mode::RealTime;

		const auto now = std::chrono::steady_clock::now();
		_startTime = now;
		_lastTickTime = now;
		_paused = false;
	}

	// Resets the timer to zero
	void Reset() {
		_fixedElapsed = 0.0f;

		_startTime = _paused
			? _pauseTime
			: std::chrono::steady_clock::now();
	}
	// Returns the elapsed time in seconds since the last reset
	float Elapsed() const {
		if (_mode == Mode::Fixed) {
			return _fixedElapsed;
		}
		const auto now = _paused
			? _pauseTime
			: std::chrono::steady_clock::now();
		return std::chrono::duration_cast<std::chrono::duration<float>>(now - _startTime).count();
	}
	float Tick() {
		if (_paused) {
			return 0.0f;
		}
		if (_mode == Mode::Fixed) {
			_fixedElapsed += _fixedDelta;
			return _fixedDelta;
		}
		auto now = std::chrono::steady_clock::now();
		float deltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(now - _lastTickTime).count();
		_lastTickTime = now;
		return deltaTime;
	}
		
	private:
		Timer() {
			const auto now = std::chrono::steady_clock::now();
			_startTime = now;
			_lastTickTime = now;
		}
		Mode _mode = Mode::RealTime;
		float _fixedDelta = 1.f / 60.f;
		float _fixedElapsed = 0.f;
		bool _paused = false;
		std::chrono::steady_clock::time_point _pauseTime;
		std::chrono::steady_clock::time_point _startTime;
		std::chrono::steady_clock::time_point _lastTickTime;

};