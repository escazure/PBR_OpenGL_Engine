cmake_minimum_required(VERSION 3.16)
project(Engine C CXX)

if(POLICY CMP0072)
    cmake_policy(SET CMP0072 NEW)
endif()

set(CPP_SRCS
	src/engine.cpp
	src/input.cpp
	src/utils.cpp
	src/resourceManager.cpp
)

set(TINY_OBJ_LOADER_SRC external/tinyobjloader/tiny_obj_loader.cpp)
set(TINY_OBJ_LOADER_INCLUDE external/tinyobjloader)

set(GL3W_SRC external/gl3w/gl3w.c)
set(GL3W_INCLUDE external/gl3w)

set(STBI_SRC external/stb_image/stb_image.cpp)
set(STBI_INCLUDE external/stb_image)

add_library(Engine STATIC
	${CPP_SRCS}
	${GL3W_SRC}
	${TINY_OBJ_LOADER_SRC}
	${STBI_SRC}
)

target_include_directories(Engine PUBLIC
	include
	external
	${GL3W_INCLUDE}
	${TINY_OBJ_LOADER_INCLUDE}
	${STBI_INCLUDE}
)

find_library(GLFW_LIBRARY
	NAMES glfw libglfw
	PATHS "${CMAKE_SOURCE_DIR}/libs"
	REQUIRED
)
find_package(OpenGL REQUIRED)

target_link_libraries(Engine PRIVATE
	${GLFW_LIBRARY}
	OpenGL::GL
)

target_compile_features(Engine PUBLIC cxx_std_23)
target_compile_options(Engine PUBLIC -Wno-pragmas)
target_compile_definitions(Engine PUBLIC 
	$<$<CONFIG:Debug>:ENABLE_OPENGL_DEBUG>
)

if(UNIX AND NOT APPLE)
	set_target_properties(Engine PROPERTIES
        BUILD_RPATH "${CMAKE_SOURCE_DIR}/libs"
    )
endif()
