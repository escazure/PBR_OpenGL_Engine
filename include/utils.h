#pragma once
#include <string>
#include <chrono>

namespace Utils {
	
std::string toLower(const std::string& input);

class Timer {
private:
	std::chrono::steady_clock::time_point _startTime;
	float _elapsedTime = 0.0f;

public:
	void Start();
	void Stop();
	float GetElapsed();
};

}
