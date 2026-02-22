# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(_sdkconfig_defaults "${CMAKE_SOURCE_DIR}/sdkconfig.defaults")

if(DEFINED IDF_TARGET)
	set(_idf_target_name "${IDF_TARGET}")
elseif(DEFINED ENV{IDF_TARGET})
	set(_idf_target_name "$ENV{IDF_TARGET}")
endif()

if(DEFINED _idf_target_name)
	set(_target_defaults "${CMAKE_SOURCE_DIR}/sdkconfig.defaults.${_idf_target_name}")
	if(EXISTS "${_target_defaults}")
		list(APPEND _sdkconfig_defaults "${_target_defaults}")
		message(STATUS "Using target-specific defaults: ${_target_defaults}")
	endif()
endif()

set(SDKCONFIG_DEFAULTS "${_sdkconfig_defaults}")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32-web)
