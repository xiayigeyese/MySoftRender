cmake_minimum_required(VERSION 3.1)

project(SoftRender)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_BINARY_DIR ${CMAKE_BUILD_DIR})
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

# src
file(GLOB_RECURSE SRCS
		"${PROJECT_SOURCE_DIR}/src/*.cpp"
		"${PROJECT_SOURCE_DIR}/src/*.h"
		"${PROJECT_SOURCE_DIR}/3rdParty/tgaimage/*.cpp"
		"${PROJECT_SOURCE_DIR}/3rdParty/imgui/src/*.cpp")

list(APPEND SRCS "${PROJECT_SOURCE_DIR}/3rdParty/glad/src/glad.c")		
		
add_executable(SoftRender ${SRCS})

FOREACH(_SRC IN ITEMS ${SRCS})
    GET_FILENAME_COMPONENT(SRC "${_SRC}" PATH)
    STRING(REPLACE "${PROJECT_SOURCE_DIR}/src/include" "include" _GRP_PATH "${SRC}")
    STRING(REPLACE "${PROJECT_SOURCE_DIR}/src/src" "src" _GRP_PATH "${_GRP_PATH}")
	STRING(REPLACE "${PROJECT_SOURCE_DIR}/3rdParty/tgaimage/src" "src/tgaimage" _GRP_PATH "${_GRP_PATH}")
    STRING(REPLACE "${PROJECT_SOURCE_DIR}/3rdParty/imgui/src" "src/imgui" _GRP_PATH "${_GRP_PATH}")
	STRING(REPLACE "${PROJECT_SOURCE_DIR}/3rdParty/glad/src" "src/glad" _GRP_PATH "${_GRP_PATH}")
	STRING(REPLACE "/" "\\" _GRP_PATH "${_GRP_PATH}")
    SOURCE_GROUP("${_GRP_PATH}" FILES "${_SRC}")
ENDFOREACH()

target_include_directories(SoftRender PUBLIC "${PROJECT_SOURCE_DIR}/src/include"
                                             "${PROJECT_SOURCE_DIR}/3rdParty/glfw/include"
										     "${PROJECT_SOURCE_DIR}/3rdParty/glad/include"
											 "${PROJECT_SOURCE_DIR}/3rdParty/imgui/include"
											 "${PROJECT_SOURCE_DIR}/3rdParty/glm/include"
											 "${PROJECT_SOURCE_DIR}/3rdParty/tgaimage/include")

# lib
target_link_directories(SoftRender PUBLIC "${PROJECT_SOURCE_DIR}/3rdParty/glfw/lib")		
target_link_libraries(SoftRender glfw3)
										   
# set workDir
if(MSVC)
	set_target_properties(SoftRender PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()	