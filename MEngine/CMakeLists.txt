# CMakeList.txt : CMake project for MEngine, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable(MEngine)

if(CMAKE_VERSION VERSION_GREATER 3.25)
	set_property(TARGET MEngine PROPERTY CXX_STANDARD 20)
	set_property(TARGET MEngine PROPERTY C_STANDARD 17)
endif()

# Define some project macros
if(WIN32)
	target_compile_definitions(MEngine PRIVATE _CRT_SECURE_NO_WARNINGS)			# Disable some annoying warnings, the CRT secure version are no safer than the normal ones
endif()

# Define global macros for the project
target_compile_definitions(MEngine PRIVATE MENGINE_VERSION=1)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	target_compile_definitions(MEngine PRIVATE MENGINE_DEBUG)
endif()

if(WIN32)
	target_compile_definitions(MEngine PRIVATE MENGINE_PLATFORM_WINDOWS)
elseif(LINUX AND APPLE)
	target_compile_definitions(MEngine PRIVATE MENGINE_PLATFORM_UNIX)
endif()

# Check the Operating System and include the appropriate file for execution
if(WIN32)
	target_sources(MEngine PRIVATE
		"src/sys/win32/winlocal.h"
		"src/sys/win32/wglext.h"
		"src/sys/win32/winmain.c"
		"src/sys/win32/wndproc.c"
		"src/sys/win32/syswin.c"
		"src/sys/win32/glwnd.c"
	)
elseif(LINUX OR APPLE)
	target_sources(MEngine PRIVATE
		"src/sys/linux/linuxlocal.h"
		"src/sys/linux/main.c"
		"src/sys/linux/wndcb.c"
		"src/sys/linux/syslinux.c"
		"src/sys/linux/linuxglwnd.c"		# On Linux systems using GLFW for the windowing system instead of X11 or Wayland, should be POSIX portable
	)	# Apple is not really Linux, but its same code anyway... mostly (looking at you C11 threads)
endif()

# Common source files, all this code is Operating System independent
target_sources(MEngine PRIVATE
	"src/mservices.h"
	"src/sys/sys.h"
	"src/common/common.h"
	"src/common/common.c"
	"src/common/log.c"
	"src/common/cvar.c"
	"src/common/memory.c"
	"src/common/filesystem.c"
	"src/renderer/renderer.h"
	"src/renderer/rendererinit.c"
	"src/renderer/renderer.c"
)

include_directories(src)														# This is useful so I can just #include framework without worrying about relative pathing

# Set up all the linker options here
if(WIN32)
	target_compile_definitions(MEngine PRIVATE UNICODE _UNICODE)				# Make sure Windows uses Unicode

	if(MSVC)
		target_link_options(MEngine PRIVATE "/SUBSYSTEM:WINDOWS")
	elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		set_target_properties(MEngine PROPERTIES LINK_FLAGS "-mwindows")
	elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		set_target_properties(MEngine PROPERTIES LINK_FLAGS "-mwindows")
	endif()
endif()

# Add the OpenGL GLU library to the project and link to target
if(WIN32)
	target_link_libraries(MEngine PRIVATE opengl32 glu32)
elseif(LINUX OR APPLE)
	target_link_libraries(MEngine PRIVATE GL GLU)
endif()

if(LINUX OR APPLE)		# Only add GLFW to the project on a Unix system, Windows will use Win32 API
	# Add the GLFW library to the project and link to target
	set(GLFW_LIBRARY_TYPE STATIC CACHE STRING "Build as static lib")
	set(GLFW_INSTALL OFF CACHE BOOL "Generate installation target")					# Don't care about this, just turn it off'
	set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW example programs")		# Turn all this garbage off, build just the Lib and nothing else
	set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW test programs")
	set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation")
	add_subdirectory("libs/glfw" EXCLUDE_FROM_ALL)
	target_link_libraries(MEngine PRIVATE glfw)
	include_directories(libs/glfw/include)											# Just add the include path for ease of use
endif()

# Add Zlib to the project
add_subdirectory("libs/zlib" EXCLUDE_FROM_ALL)
target_link_libraries(MEngine PRIVATE zlibstatic)

# Add LibZip to the project
message(STATUS "Setting up LibZip for the project")
message(STATUS "MEngine binary directory = ${CMAKE_CURRENT_BINARY_DIR}")

# This is a hack to get the Zlib library path, I don't know how to get it from the Zlib target
if(WIN32)
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		set(ZLIB_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/libs/zlib/zlibstaticd.lib")
	elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
		set(ZLIB_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/libs/zlib/zlibstatic.lib")
	endif()
elseif(LINUX OR APPLE)
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		set(ZLIB_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/libs/zlib/zlibstaticd.a")
	elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
		set(ZLIB_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/libs/zlib/zlibstatic.a")
	endif()
endif()

set(ZLIB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/zlib")

# Include Zlib directories
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/libs/zlib" "${CMAKE_CURRENT_BINARY_DIR}/libs/zlib")

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries")
set(LIBZIP_DO_INSTALL OFF CACHE BOOL "Generate installation target")
add_subdirectory("libs/libzip" EXCLUDE_FROM_ALL)
target_link_libraries(MEngine PRIVATE libzip::zip)
include_directories(libs/libzip/lib)												# Just add the include path for ease of use

# Set up all the compiler options here
if(MSVC)
	target_compile_options(MEngine PRIVATE "/W4" "/WX" "/permissive-" "/analyze" "/fp:fast")				# Common options for all build types
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")																	# Debug build compiler options
		target_compile_options(MEngine PRIVATE "/Zi" "/fsanitize=address" "/Od" "/MDd" "/JMC" "/FAs")
	elseif(CMAKE_BUILD_TYPE STREQUAL "Release")																# Release build compiler options
		target_compile_options(MEngine PRIVATE "/O2" "/MD" "/GL" "/Gw" "/Ot" "/FAs")
	endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_compile_options(MEngine PRIVATE "-W4" "-Werror" "-Wpedantic" "-ffast-math")						# Common options for all build types
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")																	# Debug build compiler options
		target_compile_options(MEngine PRIVATE "-g" "-fsanitize=address" "-O0" "-S")
	elseif(CMAKE_BUILD_TYPE STREQUAL "Release")																# Release build compiler options
		target_compile_options(MEngine PRIVATE "-O3" "-flto" "-S")
	endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(MEngine PRIVATE "-W4" "-Werror" "-Wpedantic" "-ffast-math")						# Common options for all build types
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")																	# Debug build compiler options
		target_compile_options(MEngine PRIVATE "-g" "-fsanitize=address" "-O0" "-S")
	elseif(CMAKE_BUILD_TYPE STREQUAL "Release")																# Release build compiler options
		target_compile_options(MEngine PRIVATE "-O3" "-flto" "-S")
	endif()
endif()

# Add options for doing a memory check with ASAN
option(CHECK_MEMORY "Uses ASAN to check if there are any memory leaks" OFF)

if(CHECK_MEMORY)
target_compile_definitions(MEngine PRIVATE MemCache_Alloc=malloc)
target_compile_definitions(MEngine PRIVATE MemCache_Free=free)
endif()
