#pragma once
#include <print>

namespace Logger {
	inline void Error(const std::string& source, const std::string& message){
		std::print("\033[22;31m[{}]: {}\033[0m\n", source, message);
	}

	inline void Warning(const std::string& source, const std::string& message){
		std::print("\033[22;33m[{}]: {}\033[0m\n", source, message);
	}

	inline void Info(const std::string& source, const std::string& message){
		std::print("\033[22;37m[{}]: {}\033[0m\n", source, message);
	}

	inline void Success(const std::string& source, const std::string& message){
		std::print("\033[22;32m[{}]: {}\033[0m\n", source, message);
	}
};
