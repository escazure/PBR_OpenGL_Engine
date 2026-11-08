#include "utils.h"
#include <cctype>

std::string Utils::toLower(const std::string& input){
	std::string result = input; // Reserve exact same size
	for(char& c : result){
		c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	}
	return result; 
}

