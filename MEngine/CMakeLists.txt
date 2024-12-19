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
elseif(LINUX)
	target_compile_definitions(MEngine PRIVATE MENGINE_PLATFORM_LINUX)
elseif(APPLE)
	target_compile_definitions(MEngine PRIVATE MENGINE_PLATFORM_MACOS)
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
		"src/sys/posix/posixlocal.h"
		"src/sys/posix/main.c"
		"src/sys/posix/wndcb.c"
		"src/sys/posix/sysposix.c"
		"src/sys/posix/posixglwnd.c"		# On POSIX systems using GLFW for the windowing system instead of X11 or Wayland directly
	)

	include_directories(src/sys/posix)
endif()

if(LINUX)
	target_sources(MEngine PRIVATE
		"src/sys/posix/linux/syslinux.c"
	)
elseif(APPLE)
	target_sources(MEngine PRIVATE
		"src/sys/posix/macos/sysmacos.c"
	)
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
elseif(LINUX)
	target_link_libraries(MEngine PRIVATE GL GLU)
elseif(APPLE)
	find_package(OpenGL REQUIRED)
	include_directories(${OPENGL_INCLUDE_DIRS})
	target_link_libraries(MEngine PRIVATE ${OPENGL_LIBRARIES})
	message(STATUS "OpenGL libraries found: ${OPENGL_LIBRARIES}")
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

# Add Zlib-ng to the project
set(WITH_NATIVE_INSTRUCTIONS ON CACHE BOOL "Compiles with full instruction set supported on this host (gcc/clang -march=native)")
set(ZLIBNG_ENABLE_TESTS OFF CACHE BOOL "Test zlib-ng specific API")
set(ZLIB_ENABLE_TESTS OFF CACHE BOOL "Build test binaries")
set(WITH_GTEST OFF CACHE BOOL "Build gtest_zlib")
add_subdirectory("libs/zlib-ng" EXCLUDE_FROM_ALL)
target_link_libraries(MEngine PRIVATE zlib)

# Add the Zlib-ng include and lib directory to the project
target_include_directories(MEngine PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/libs/zlib-ng")
target_include_directories(MEngine PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/libs/zlib-ng")
target_link_directories(MEngine PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/libs/zlib-ng")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/libs/zlib-ng" "${CMAKE_CURRENT_BINARY_DIR}/libs/zlib-ng")

# Add Minizip-ng to the project
set(MZ_FETCH_LIBS OFF CACHE BOOL "Fetch external dependencies")
set(MZ_BZIP2 OFF CACHE BOOL "Enable bzip2 support")
set(MZ_LZMA OFF CACHE BOOL "Enable lzma support")
set(MZ_ZSTD OFF CACHE BOOL "Enable zstd support")
set(MZ_OPENSSL OFF CACHE BOOL "Enable openssl support")
set(MZ_WZAES OFF CACHE BOOL "Enable wzaes support")
set(MZ_PKCRYPT OFF CACHE BOOL "Enable pkcrypt support")
set(MZ_LIBCOMP OFF CACHE BOOL "Enable libcomp support")
set(MZ_LIBBSD OFF CACHE BOOL "Enable libbsd support")
set(MZ_ICONV OFF CACHE BOOL "Enable iconv support")
add_subdirectory("libs/minizip-ng" EXCLUDE_FROM_ALL)
target_link_libraries(MEngine PRIVATE MINIZIP::minizip)
include_directories("libs/minizip-ng")

# Set up all the compiler options here
if(MSVC)
	target_compile_options(MEngine PRIVATE "/W4" "/WX" "/permissive-" "/analyze" "/fp:fast")					# Common options for all build types
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")																		# Debug build compiler options
		target_compile_options(MEngine PRIVATE "/Zi" "/fsanitize=address" "/Od" "/MDd" "/JMC" "/FAs")
	elseif(CMAKE_BUILD_TYPE STREQUAL "Release")																	# Release build compiler options
		target_compile_options(MEngine PRIVATE "/O2" "/MD" "/GL" "/Gw" "/Ot" "/FAs")
	endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_compile_options(MEngine PRIVATE "-Wall" "-Werror" "-Wpedantic" "-ffast-math"
		"--analyze" "-Xanalyzer" "-analyzer-output=html" "-o" "${CMAKE_BINARY_DIR}"
	)																											# Common options for all build types
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")																		# Debug build compiler options
		target_compile_options(MEngine PRIVATE
			"-g"
			"-fanalyzer"
			"-fsanitize=leak"
			"-fsanitize=undefined"
			"-fsanitize=address"
			"-fsanitize=thread"
			"-O0"
		)
		target_link_libraries(MEngine PRIVATE
			"-fsanitize=leak"
			"-fsanitize=undefined"
			"-fsanitize=address"
			"-fsanitize=thread"
		)
	elseif(CMAKE_BUILD_TYPE STREQUAL "Release")																	# Release build compiler options
		target_compile_options(MEngine PRIVATE "-O3" "-flto")
	endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(MEngine PRIVATE "-Wall" "-Werror" "-Wpedantic" "-ffast-math"
		"--analyze" "-Xanalyzer" "-analyzer-output=text"
	)																											# Common options for all build types
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")																		# Debug build compiler options
		target_compile_options(MEngine PRIVATE
			"-g"
			"-fanalyzer"
			"-fsanitize=leak"
			"-fsanitize=undefined"
			"-fsanitize=address"
			"-fsanitize=thread"
			"-O0"
		)
		target_link_libraries(MEngine PRIVATE
			"-fsanitize=leak"
			"-fsanitize=undefined"
			"-fsanitize=address"
			"-fsanitize=thread"
		)
	elseif(CMAKE_BUILD_TYPE STREQUAL "Release")																	# Release build compiler options
		target_compile_options(MEngine PRIVATE "-O3" "-flto")
	endif()
endif()

# Add options for doing a memory check with ASAN
option(MENGINE_CHECK_MEMORY "Uses ASAN to check if there are any memory leaks" OFF)

if(MENGINE_CHECK_MEMORY)
target_compile_definitions(MEngine PRIVATE MemCache_Alloc=malloc)
target_compile_definitions(MEngine PRIVATE MemCache_Free=free)
endif()
