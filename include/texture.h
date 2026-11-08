#pragma once
#include "gl3w.h"
#include <vector>

#include "logger.h"

class Texture {
private:
	GLuint _id;
	GLuint _target;
	GLuint _internalFormat;
	GLuint _pixelFormat;
	unsigned int _mipMapLevels;
	unsigned int _width;
	unsigned int _height;
	unsigned int _depth;

	void _specifyPixelFormat(){
		switch(_internalFormat){
			case GL_DEPTH_COMPONENT32F: case GL_DEPTH_COMPONENT24:
				_pixelFormat = GL_DEPTH_COMPONENT; break;
			case GL_R32F: case GL_R16F: case GL_R8:
				_pixelFormat = GL_RED; break;
			case GL_RG32F: case GL_RG16F: case GL_RG8:
				_pixelFormat = GL_RG; break;
			case GL_RGB32F: case GL_RGB16F: case GL_RGB8: case GL_SRGB8:
				_pixelFormat = GL_RGB; break;
			case GL_RGBA32F: case GL_RGBA16F: case GL_RGBA8: case GL_SRGB8_ALPHA8:
				_pixelFormat = GL_RGBA; break;
		}
	}

	void _specifyTextureStorage(){
		switch(_target){
			case GL_TEXTURE_1D:
				glTextureStorage1D(_id, _mipMapLevels, _internalFormat, _width);
				break;
			case GL_TEXTURE_2D:
			case GL_TEXTURE_1D_ARRAY:
			case GL_TEXTURE_RECTANGLE:
			case GL_TEXTURE_CUBE_MAP:
				glTextureStorage2D(_id, _mipMapLevels, _internalFormat, _width, _height);
				break;
			case GL_TEXTURE_3D:
			case GL_TEXTURE_2D_ARRAY:
			case GL_TEXTURE_CUBE_MAP_ARRAY:
				glTextureStorage3D(_id, _mipMapLevels, _internalFormat, _width, _height, _depth);
				break;
		}	
	}

	GLuint _getPixelDataType() const {
		switch(_internalFormat){
			case GL_R32F: case GL_RG32F: case GL_RGB32F: case GL_RGBA32F:
			case GL_R16F: case GL_RG16F: case GL_RGB16F: case GL_RGBA16F:
				return GL_FLOAT;
			default:
				return GL_UNSIGNED_BYTE;
		}
	}

public:
	Texture(GLuint target, GLuint internalFormat, unsigned int mipMapLevels, unsigned int width, unsigned int height = 1, unsigned int depth = 1){
		_target = target;	
		_internalFormat = internalFormat;
		_mipMapLevels = mipMapLevels;
		_width = width;
		_height = height;
		_depth = depth;

		glCreateTextures(_target, 1, &_id);
		_specifyTextureStorage();
		_specifyPixelFormat();
	}

	~Texture(){
		glDeleteTextures(1, &_id);
	}

	Texture(const Texture&) = delete;
	Texture& operator=(const Texture&) = delete;

	Texture(Texture&& other) noexcept {
		_id = other._id;	
		_target = other._target;
		_internalFormat = other._internalFormat;
		_pixelFormat = other._pixelFormat;
		_mipMapLevels = other._mipMapLevels;
		_width = other._width;
		_height = other._height;
		_depth = other._depth;

		other._id = 0;
	}

	Texture& operator=(Texture&& other) noexcept {
		if(this != &other){
			if(_id != 0)
                glDeleteTextures(1, &_id);

			_id = other._id;
			_target = other._target;
			_internalFormat = other._internalFormat;
			_pixelFormat = other._pixelFormat;
			_mipMapLevels = other._mipMapLevels;
			_width = other._width;
			_height = other._height;
			_depth = other._depth;
			other._id = 0;
		}
		return *this;
	}

	void setWrap(GLuint wrapS, GLuint wrapT, GLuint wrapR = GL_REPEAT){
		glTextureParameteri(_id, GL_TEXTURE_WRAP_S, wrapS);	
		glTextureParameteri(_id, GL_TEXTURE_WRAP_T, wrapT);	
		if(_target == GL_TEXTURE_3D || _target == GL_TEXTURE_CUBE_MAP || _target == GL_TEXTURE_2D_ARRAY)
			glTextureParameteri(_id, GL_TEXTURE_WRAP_R, wrapR);
	}

	void setFilter(GLuint minFilter, GLuint magFilter){
		glTextureParameteri(_id, GL_TEXTURE_MIN_FILTER, minFilter);
		glTextureParameteri(_id, GL_TEXTURE_MAG_FILTER, magFilter);
	}

	void generateMipmaps(){
		if(_mipMapLevels <= 1){
			Logger::Error("Texture::generateMipmaps", "Trying to generate mipmaps for less then 2 mipmap levels");
			return;
		}
		glGenerateTextureMipmap(_id);
	}

	void bind(unsigned int unit) const {
		glBindTextureUnit(unit, _id);
	}

	void bindAsImage(unsigned int unit, unsigned int mipMapLevel = 0) const {
		GLboolean layered = (_target == GL_TEXTURE_CUBE_MAP || _target == GL_TEXTURE_3D || _target == GL_TEXTURE_2D_ARRAY) ? GL_TRUE : GL_FALSE;
		glBindImageTexture(unit, _id, mipMapLevel, layered, 0, GL_WRITE_ONLY, _internalFormat);
	}

	void attach(GLuint fbo, GLuint attachment, unsigned int mipMapLevel = 0) const {
		glNamedFramebufferTexture(fbo, attachment, _id, mipMapLevel);
	}

	void resize(unsigned int mipMapLevels, unsigned int width, unsigned int height = 1, unsigned int depth = 1){
		if(_id != 0) glDeleteTextures(1, &_id);

		_mipMapLevels = mipMapLevels;
		_width = width;
		_height = height;
		_depth = depth;

		glCreateTextures(_target, 1, &_id);
		_specifyTextureStorage();
		_specifyPixelFormat();
	}

	unsigned int getChannelNumber(){
		switch(_pixelFormat){
			case GL_DEPTH_COMPONENT:
			case GL_RED:
				return 1;
			case GL_RG:
				return 2;
			case GL_RGB:
				return 3;
			case GL_RGBA:
				return 4;
			default:
				return 0;
		}
	}

	void loadPixelsSingle(unsigned int width, unsigned int height, const void* data, unsigned int level = 0, unsigned int xoffset = 0, unsigned int yoffset = 0){
		if(_target == GL_TEXTURE_3D || _target == GL_TEXTURE_2D_ARRAY){
            glTextureSubImage3D(_id, level, xoffset, yoffset, 0, width, height, _depth, _pixelFormat, _getPixelDataType(), data);
        } else{
            glTextureSubImage2D(_id, level, xoffset, yoffset, width, height, _pixelFormat, _getPixelDataType(), data);    
        }
	}

	void loadPixelsMultiple(const std::vector<const void*>& faceDataSources, unsigned int width, unsigned int height, unsigned int level = 0){
		if(_target != GL_TEXTURE_CUBE_MAP && _target != GL_TEXTURE_3D && _target != GL_TEXTURE_2D_ARRAY){
			Logger::Error("Texture", "Using loadPixelsMultiple on non-layered texture type.");
			return;
		}
		unsigned int maxLayers = (_target == GL_TEXTURE_CUBE_MAP) ? 6 : _depth;

		for(unsigned int i = 0; i < maxLayers; i++){
			if(i < faceDataSources.size() && faceDataSources[i] != nullptr)
				glTextureSubImage3D(_id, level, 0, 0, i, width, height, 1, _pixelFormat, _getPixelDataType(), faceDataSources[i]);
		}
	}
};
