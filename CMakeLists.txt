# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required(VERSION 3.25)

# Enable Hot Reload for MSVC compilers if supported.
if(POLICY CMP0141)
	cmake_policy(SET CMP0141 NEW)
	# set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project("MEngine")

# Include sub-projects.
add_subdirectory("MEngine")

# Include the demo game project.
add_subdirectory("DemoGame")

# Set the shared library prefix to an empty string for DemoGame
set_target_properties(DemoGame PROPERTIES PREFIX "")

# Add options for selecting the compiler
option(MENGINE_USE_CLANG "Use Clang compiler" OFF)
option(MENGINE_USE_GCC "Use GCC compiler" OFF)
option(MENGINE_USE_MSVC "Use MSVC compiler" OFF)

# Set the compiler based on the options, defaults should be set in the CMakePresets.json file
if(MENGINE_USE_CLANG)
	if(WIN32)
		set(CMAKE_C_COMPILER "clang-cl")
		set(CMAKE_CXX_COMPILER "clang-cl")
	else()
		set(CMAKE_C_COMPILER "clang")
		set(CMAKE_CXX_COMPILER "clang++")
	endif()
elseif(MENGINE_USE_GCC)
	set(CMAKE_C_COMPILER "gcc")
	set(CMAKE_CXX_COMPILER "g++")
elseif(MENGINE_USE_MSVC)
	set(CMAKE_C_COMPILER "cl")
	set(CMAKE_CXX_COMPILER "cl")
endif()

# Create a main output directory under the build directory where all exe and dll files will be placed
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
message(STATUS "Binary output directory set: ${CMAKE_BINARY_DIR}")
set_target_properties(MEngine PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_target_properties(DemoGame PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_target_properties(DemoGame PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

############################################################################################################
############################################## Install Targets #############################################
############################################################################################################
# Install the executable and DLLs to the bin directory
if(WIN32)
	install(TARGETS MEngine DemoGame
		RUNTIME DESTINATION MEngine/bin
	)
endif()

# Install the shared library to the bin directory, Linux doesnt do this by default
if(UNIX)
	install(TARGETS MEngine DemoGame
		LIBRARY DESTINATION MEngine/bin
		RUNTIME DESTINATION MEngine/bin
	)
endif()

# Install header files to the sdk directory, preserving the directory structure
install(FILES
	"${CMAKE_SOURCE_DIR}/MEngine/src/mservices.h"
	DESTINATION MEngine/sdk
)
