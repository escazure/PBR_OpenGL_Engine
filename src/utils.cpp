#include "utils.h"
#include <cctype>
#include <chrono>

std::string Utils::toLower(const std::string& input){
	std::string result = input; // Reserve exact same size
	for(char& c : result){
		c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	}
	return result; 
}

void Utils::Timer::Start(){
	_elapsedTime = 0.0f;
	_startTime = std::chrono::steady_clock::now();
}

void Utils::Timer::Stop(){
	_elapsedTime = std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - _startTime).count();
}

float Utils::Timer::GetElapsed(){
	return _elapsedTime;
}
