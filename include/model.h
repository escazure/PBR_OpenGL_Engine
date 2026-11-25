#pragma once

#include "gl3w.h"
#include "logger.h"
#include "tiny_obj_loader.h"

#include <vector>
#include <map>
#include <string>
#include <glm/glm.hpp>

class Model {
public: 
	Model(const std::string& objFile, const std::string& mtlFile){
		_VAO = 0;
		_VBO = 0;
		_EBO = 0;
		_instanceVBO = 0;

		_isStatic = false;
		_maxInstances = 0;

		if(_loadModel(objFile, mtlFile)) _setupModel();	
	}

	~Model(){
		if(_VAO) glDeleteVertexArrays(1, &_VAO);
		if(_VBO) glDeleteBuffers(1, &_VBO);
		if(_EBO) glDeleteBuffers(1, &_EBO);
		if(_instanceVBO) glDeleteBuffers(1, &_instanceVBO);
	}

	Model(const Model&) = delete;
	Model& operator=(const Model&) = delete;

	Model(Model&& other) noexcept;
	Model& operator=(Model&& other) noexcept;

	void draw() const {
		glBindVertexArray(_VAO);
		glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(_indices.size()), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}

	void initInstanced(unsigned int maxInstances, bool isStatic, const std::vector<glm::mat4>& transforms = {}){
		if(isStatic && transforms.size() > maxInstances){
			Logger::Warning("Model::initInstanced", "Transforms count exceed maximum instance amount.");
			return;
		}

		if(_instanceVBO) glDeleteBuffers(1, &_instanceVBO);

		_maxInstances = maxInstances;	
		_isStatic = isStatic;
		_staticCount = transforms.size();
		GLsizeiptr bufferSize = isStatic ? transforms.size() * sizeof(glm::mat4) : maxInstances * sizeof(glm::mat4);
		glCreateBuffers(1, &_instanceVBO);
		glNamedBufferData(_instanceVBO, bufferSize, isStatic ? transforms.data() : nullptr, isStatic ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW);

		glVertexArrayVertexBuffer(_VAO, 1, _instanceVBO, 0, sizeof(glm::mat4));
		
		for(int i = 0; i < 4; i++){
			unsigned int location = 3 + i;
			glEnableVertexArrayAttrib(_VAO, location);
			glVertexArrayAttribFormat(_VAO, location, 4, GL_FLOAT, GL_FALSE, i * sizeof(glm::vec4));
			glVertexArrayAttribBinding(_VAO, location, 1);
		}
		glVertexArrayBindingDivisor(_VAO, 1, 1);
	}

	void drawInstanced(const std::vector<glm::mat4>& transforms = {}){
		if(_instanceVBO == 0){
			Logger::Error("Model::drawInstanced", "Instancing not initialized.");
		}
		GLsizei count = 0;
		if(_isStatic) count = _staticCount;
		else {
			if(transforms.empty()){
				Logger::Warning("Model::drawInstanced", "Tried drawing while transforms is empty.");
				return;
			}
			if(transforms.size() > _maxInstances){
				Logger::Warning("Model::drawInstanced", "Transforms count exceed maximum instance amount.");
				return;
			}
			count = static_cast<GLsizei>(transforms.size());
			glNamedBufferSubData(_instanceVBO, 0, count * sizeof(glm::mat4), transforms.data());
		}

		glBindVertexArray(_VAO);
		glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(_indices.size()), GL_UNSIGNED_INT, 0, count);
		glBindVertexArray(0);
	}

private:
	GLuint _VAO;
	GLuint _VBO;
	GLuint _EBO;
	GLuint _instanceVBO;

	GLuint _dataPrecision;

	tinyobj::attrib_t _attrib;	
	std::vector<tinyobj::shape_t> _shapes;
	std::vector<tinyobj::material_t> _materials;

	std::vector<tinyobj::real_t> _vertexData;
	std::vector<unsigned int> _indices;

	unsigned int _maxInstances;
	unsigned int _staticCount;
	bool _isStatic;

	struct CompareIndex {
		bool operator()(const tinyobj::index_t& a, const tinyobj::index_t &b) const {
			if(a.vertex_index != b.vertex_index) return a.vertex_index < b.vertex_index;
			if(a.normal_index != b.normal_index) return a.normal_index < b.normal_index;
			return a.texcoord_index < b.texcoord_index;
		}
	};

	bool _loadModel(const std::string& objFile, const std::string& mtlFile){
		tinyobj::ObjReaderConfig reader_config;
		reader_config.mtl_search_path = mtlFile;

		tinyobj::ObjReader reader;
		if(!reader.ParseFromFile(objFile, reader_config)){
			if(!reader.Error().empty()){
				Logger::Error("TinyObjReader", reader.Error());
			}
			return false;
		}

		if(!reader.Warning().empty())
			Logger::Warning("TinyObjReader", reader.Warning());

		_attrib = reader.GetAttrib();
		_shapes = reader.GetShapes();
		_materials = reader.GetMaterials();

		std::map<tinyobj::index_t, unsigned int, CompareIndex> uniqueVertices;

		for(const auto& shape : _shapes){
            for(const auto& index : shape.mesh.indices){
				auto it = uniqueVertices.find(index);
				if(it != uniqueVertices.end()){
					_indices.push_back(it->second);	
					continue;
				}
				
				unsigned int newIndex = static_cast<unsigned int>(_vertexData.size() / 8);
				uniqueVertices[index] = newIndex;
				_indices.push_back(newIndex);

                if(index.vertex_index >= 0){
                    _vertexData.push_back(_attrib.vertices[3 * index.vertex_index + 0]);
                    _vertexData.push_back(_attrib.vertices[3 * index.vertex_index + 1]);
                    _vertexData.push_back(_attrib.vertices[3 * index.vertex_index + 2]);
                }

                if(index.normal_index >= 0){
                    _vertexData.push_back(_attrib.normals[3 * index.normal_index + 0]);
                    _vertexData.push_back(_attrib.normals[3 * index.normal_index + 1]);
                    _vertexData.push_back(_attrib.normals[3 * index.normal_index + 2]);
                }else{
					_vertexData.push_back(0.0f);
					_vertexData.push_back(0.0f);
					_vertexData.push_back(0.0f);
				}

                if(index.texcoord_index >= 0){
                    _vertexData.push_back(_attrib.texcoords[2 * index.texcoord_index + 0]);
                    _vertexData.push_back(_attrib.texcoords[2 * index.texcoord_index + 1]);
                }else{
					_vertexData.push_back(0.0f);
					_vertexData.push_back(0.0f);
				}
            }
        }

		return true;
	}

	void _setupModel(){
		_dataPrecision = sizeof(tinyobj::real_t) == 4 ? GL_FLOAT : GL_DOUBLE;
		GLsizei stride = 8 * sizeof(tinyobj::real_t);

		glCreateVertexArrays(1, &_VAO);	
		glCreateBuffers(1, &_VBO);
		glCreateBuffers(1, &_EBO);

		glNamedBufferData(_VBO, _vertexData.size() * sizeof(tinyobj::real_t), _vertexData.data(), GL_STATIC_DRAW);
		glNamedBufferData(_EBO, _indices.size() * sizeof(unsigned int), _indices.data(), GL_STATIC_DRAW);

		glVertexArrayVertexBuffer(_VAO, 0, _VBO, 0, stride);
		glVertexArrayElementBuffer(_VAO, _EBO);

		glEnableVertexArrayAttrib(_VAO, 0);
		glVertexArrayAttribFormat(_VAO, 0, 3, _dataPrecision, GL_FALSE, 0);
		glVertexArrayAttribBinding(_VAO, 0, 0);

		glEnableVertexArrayAttrib(_VAO, 1);
		glVertexArrayAttribFormat(_VAO, 1, 3, _dataPrecision, GL_FALSE, 3 * sizeof(tinyobj::real_t));
		glVertexArrayAttribBinding(_VAO, 1, 0);

		glEnableVertexArrayAttrib(_VAO, 2);
		glVertexArrayAttribFormat(_VAO, 2, 2, _dataPrecision, GL_FALSE, 6 * sizeof(tinyobj::real_t));
		glVertexArrayAttribBinding(_VAO, 2, 0);
	}
};
