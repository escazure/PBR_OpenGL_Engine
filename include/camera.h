#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glfw/glfw3.h>

#include "input.h"

#define MAX_PITCH 75.0
#define MIN_PITCH -75.0

class Camera {
private:
	glm::vec3 _position;
	glm::vec3 _front;
	glm::vec3 _right;
	glm::vec3 _up;
	glm::vec3 _worldUp;

	float _yaw;
	float _pitch;

	float _speed;
	float _sensitivity;
	float _fieldOfView;

	float _nearPlane;
	float _farPlane;

	void _updateDirection(){
		glm::vec3 newFront;
		float yawRadians = glm::radians(_yaw);
		float pitchRadians = glm::radians(_pitch);

		newFront.x = glm::cos(yawRadians) * glm::cos(pitchRadians);
		newFront.y = glm::sin(pitchRadians);
		newFront.z = glm::sin(yawRadians) * glm::cos(pitchRadians);

		_front = glm::normalize(newFront);
		_right = glm::normalize(glm::cross(_front, _worldUp));
		_up = glm::normalize(glm::cross(_right, _front));
	}

public: 
	Camera(const glm::vec3& position, float yaw, float pitch, float speed, float sensitivity, float fieldOfView, float nearPlane, float farPlane){
		_position = position;
		_yaw = yaw;
		_pitch = pitch;
		_speed = speed;
		_sensitivity = sensitivity;
		_fieldOfView = fieldOfView;
		_nearPlane = nearPlane;
		_farPlane = farPlane;

		_worldUp = {0.0f, 1.0f, 0.0f};
		_updateDirection();
	}

	Camera(const Camera&) = delete;
	Camera& operator=(const Camera&) = delete;

	Camera(Camera&& other){
		_position = other._position;
		_yaw = other._yaw;
		_pitch = other._pitch;
		_speed = other._speed;
		_sensitivity = other._sensitivity;
		_fieldOfView = other._fieldOfView;
		_nearPlane = other._nearPlane;
		_farPlane = other._farPlane;
		_front = other._front;
		_right = other._right;
		_up = other._up;
	}

	Camera& operator=(Camera&& other){
		if(this != &other){
			_position = other._position;
			_yaw = other._yaw;
			_pitch = other._pitch;
			_speed = other._speed;
			_sensitivity = other._sensitivity;
			_fieldOfView = other._fieldOfView;
			_nearPlane = other._nearPlane;
			_farPlane = other._farPlane;
			_front = other._front;
			_right = other._right;
			_up = other._up;
		}
		return *this;	
	}

	void update(float deltaTime){
		// FPS style camera requires zeroing out Y component of front vector, leaving will turn this into Spaceship Style //
		glm::vec3 moveFront = glm::normalize(glm::vec3(_front.x, 0.0f, _front.z));

		if(Input::isKeyDown(GLFW_KEY_W)) _position += moveFront* _speed * deltaTime;
		if(Input::isKeyDown(GLFW_KEY_S)) _position -= moveFront * _speed * deltaTime;
		if(Input::isKeyDown(GLFW_KEY_A)) _position -= _right * _speed * deltaTime;
		if(Input::isKeyDown(GLFW_KEY_D)) _position += _right * _speed * deltaTime;
		if(Input::isKeyDown(GLFW_KEY_SPACE)) _position += _worldUp * _speed * deltaTime;
		if(Input::isKeyDown(GLFW_KEY_LEFT_CONTROL)) _position -= _worldUp * _speed * deltaTime;

		glm::vec2 mouseDelta = Input::getMouseDelta();
		_yaw += mouseDelta.x * _sensitivity;
		_pitch -= mouseDelta.y * _sensitivity;

		if(_pitch > MAX_PITCH) _pitch = MAX_PITCH;
		if(_pitch < MIN_PITCH) _pitch = MIN_PITCH;

		_updateDirection();
	}

	glm::vec3 getPosition(){
		return _position;
	}

	glm::mat4 getViewMatrix(){
		return glm::lookAt(_position, _position + _front, _up);
	}

	glm::mat4 getProjectionMatrix(float aspectRatio){
		return glm::perspective(glm::radians(_fieldOfView), aspectRatio, _nearPlane, _farPlane);
	}
};
