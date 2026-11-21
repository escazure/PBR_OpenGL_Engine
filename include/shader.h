#pragma once

#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "logger.h"

class Shader {
private:
	GLuint id;

public:
	Shader(const std::string& computePath){
		std::string computeCode;
		std::ifstream shaderFile;

		shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		try {
			shaderFile.open(computePath.c_str());
			std::stringstream stream;

			stream << shaderFile.rdbuf();
			shaderFile.close();

			computeCode = stream.str();
		} catch(std::ifstream::failure e){
			Logger::Error("Shader", std::string("Failed to load compute shader at path: ") + computePath);
			return;
		}
		const char* code = computeCode.c_str();

		GLuint shader;
		int success;
		char infoLog[512];

		shader = glCreateShader(GL_COMPUTE_SHADER);
		glShaderSource(shader, 1, &code, NULL);
		glCompileShader(shader);
		
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if(!success){
			glGetShaderInfoLog(shader, 512, NULL, infoLog);	
			Logger::Error("Shader", std::string("Compute shader compilation failed: ") + std::string(infoLog));
			glDeleteShader(shader);
			return;
		}

		id = glCreateProgram();
		glAttachShader(id, shader);
		glLinkProgram(id);

		glGetProgramiv(id, GL_LINK_STATUS, &success);
		if(!success){
			glGetProgramInfoLog(id, 512, NULL, infoLog);
			Logger::Error("Shader", std::string("Shader program linking failed: ") + std::string(infoLog));
			glDeleteShader(shader);
			return;
		}

		glDeleteShader(shader);
	}

	Shader(const std::string& vertexPath, const std::string& fragmentPath){
		std::string vertexCode;
		std::string fragmentCode;
		std::ifstream vertexFile;
		std::ifstream fragmentFile;

		vertexFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
		fragmentFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);

		try {
			vertexFile.open(vertexPath.c_str());
			fragmentFile.open(fragmentPath.c_str());
			std::stringstream vertexStream, fragmentStream;

			vertexStream << vertexFile.rdbuf();
			fragmentStream << fragmentFile.rdbuf();

			vertexFile.close();
			fragmentFile.close();

			vertexCode= vertexStream.str();
			fragmentCode = fragmentStream.str();
		}
		catch(std::ifstream::failure e){
			Logger::Error("Shader", std::string("Failed to load vertex/fragment shader at path: ") + vertexPath + std::string(" | ") + fragmentPath);
			return;
		}
		const char* vCode = vertexCode.c_str();
		const char* fCode = fragmentCode.c_str();

		GLuint vertex, fragment;
		int success;
		char infoLog[512];

		vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &vCode, NULL);
		glCompileShader(vertex);
		
		glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
		if(!success){
			glGetShaderInfoLog(vertex, 512, NULL, infoLog);	
			Logger::Error("Shader", std::string("Vertex shader compilation failed: ") + std::string(infoLog));
			glDeleteShader(vertex);
			return;
		}

		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &fCode, NULL);
		glCompileShader(fragment);
		
		glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
		if(!success){
			glGetShaderInfoLog(fragment, 512, NULL, infoLog);	
			Logger::Error("Shader", std::string("Fragment shader compilation failed: ") + std::string(infoLog));
			glDeleteShader(vertex);
			glDeleteShader(fragment);
			return;
		}

		id = glCreateProgram();
		glAttachShader(id, vertex);
		glAttachShader(id, fragment);
		glLinkProgram(id);

		glGetProgramiv(id, GL_LINK_STATUS, &success);
		if(!success){
			glGetProgramInfoLog(id, 512, NULL, infoLog);
			Logger::Error("Shader", std::string("Shader program linking failed: ") + std::string(infoLog));
			glDeleteShader(vertex);
			glDeleteShader(fragment);
			return;
		}

		glDeleteShader(vertex);
		glDeleteShader(fragment);
	}

	void use(){
		glUseProgram(id);
	}

	void setBool(const std::string &name, bool value) const {
		glUniform1i(glGetUniformLocation(id, name.c_str()), (int)value);
	}

	void setInt(const std::string &name, int value) const {
		glUniform1i(glGetUniformLocation(id, name.c_str()), value);
	}

	void setFloat(const std::string &name, float value) const {
		glUniform1f(glGetUniformLocation(id, name.c_str()), value);
	}

	void setMat4(const std::string &name, const glm::mat4 &mat) const {
		glUniformMatrix4fv(glGetUniformLocation(id, name.c_str()), 1, GL_FALSE, &mat[0][0]);
	}

	void setVec3(const std::string &name, const glm::vec3 &vec) const {
		glUniform3f(glGetUniformLocation(id, name.c_str()), vec.x, vec.y, vec.z);
	}

	void setVec4(const std::string &name, const glm::vec4 &vec) const {
		glUniform4f(glGetUniformLocation(id, name.c_str()), vec.x, vec.y, vec.z, vec.w);
	}
};
