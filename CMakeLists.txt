# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required(VERSION 3.16)

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

# Create a main output directory under the build directory where all exe and dll files will be placed.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
message(STATUS "Binary output directory set: ${CMAKE_BINARY_DIR}")
set_target_properties(MEngine PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_target_properties(DemoGame PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

############################################################################################################
############################################## Install Targets #############################################
############################################################################################################
# Install the executable and DLLs to the bin directory
install(TARGETS MEngine DemoGame
	RUNTIME DESTINATION MEngine/bin
)

# Install header files to the sdk directory, preserving the directory structure
install(FILES
	"${CMAKE_SOURCE_DIR}/MEngine/src/mservices.h"
	DESTINATION MEngine/sdk
)
