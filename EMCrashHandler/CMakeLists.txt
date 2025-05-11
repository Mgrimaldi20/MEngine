# CMakeList.txt : CMake project for MEngine, include source and define
# project specific logic here.
#

# Add source to this project's executable
add_executable(EMCrashHandler)

if(CMAKE_VERSION VERSION_GREATER 3.25)
	set_property(TARGET EMCrashHandler PROPERTY CXX_STANDARD 20)
	set_property(TARGET EMCrashHandler PROPERTY C_STANDARD 23)
endif()

# Define some project macros
if(WIN32)
	target_compile_definitions(EMCrashHandler PRIVATE _CRT_SECURE_NO_WARNINGS)			# Disable some annoying warnings, the CRT secure version are no safer than the normal ones
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	target_compile_definitions(EMCrashHandler PRIVATE MENGINE_DEBUG)
endif()

if(WIN32)
	target_compile_definitions(EMCrashHandler PRIVATE MENGINE_PLATFORM_WINDOWS)
elseif(LINUX)
	target_compile_definitions(EMCrashHandler PRIVATE MENGINE_PLATFORM_LINUX)
elseif(APPLE)
	target_compile_definitions(EMCrashHandler PRIVATE MENGINE_PLATFORM_MACOS)
endif()

# Check the Operating System and include the appropriate file for execution
if(WIN32)
	target_sources(EMCrashHandler PRIVATE
		"src/win32/winmain.c"
	)
elseif(LINUX OR APPLE)
	target_sources(EMCrashHandler PRIVATE
		"src/posix/main.c"
	)
endif()

# Common source files, all this code is Operating System independent
target_sources(EMCrashHandler PRIVATE
	"src/emstatus.h"
)

# Set up all the linker options here
if(WIN32)
	target_compile_definitions(EMCrashHandler PRIVATE UNICODE _UNICODE)				# Make sure Windows uses Unicode

	if(MSVC)
		target_link_options(EMCrashHandler PRIVATE "/SUBSYSTEM:WINDOWS")
	elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		set_target_properties(EMCrashHandler PROPERTIES LINK_FLAGS "-mwindows")
	elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		set_target_properties(EMCrashHandler PROPERTIES LINK_FLAGS "-mwindows")
	endif()
endif()

# Set up all the compiler options here
if(MSVC)
	target_compile_options(EMCrashHandler PRIVATE "/W4" "/WX" "/permissive-" "/analyze" "/fp:fast" "/FAs")		# Common options for all build types
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")																		# Debug build compiler options
		target_compile_options(EMCrashHandler PRIVATE "/Zi" "/fsanitize=address" "/Od" "/MDd" "/JMC")
		target_link_options(EMCrashHandler PRIVATE "/DEBUG")													# Ensure PDB file is generated
	elseif(CMAKE_BUILD_TYPE STREQUAL "Release")																	# Release build compiler options
		target_compile_options(EMCrashHandler PRIVATE "/O2" "/MD" "/GL" "/Gw")
	endif()

elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")																	# CLANG compiler options
	target_compile_options(EMCrashHandler PRIVATE "-Wall" "-Werror" "-Wpedantic" "-ffast-math" "-fanalyzer")	# Common options for all build types
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")																		# Debug build compiler options
		target_compile_options(EMCrashHandler PRIVATE "-g" "-O0")
	elseif(CMAKE_BUILD_TYPE STREQUAL "Release")																	# Release build compiler options
		target_compile_options(EMCrashHandler PRIVATE "-O3" "-flto")
	endif()

elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")																	# GCC compiler options
	target_compile_options(EMCrashHandler PRIVATE "-Wall" "-Werror" "-Wpedantic" "-ffast-math" "-fanalyzer")	# Common options for all build types
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")																		# Debug build compiler options
		target_compile_options(EMCrashHandler PRIVATE "-g" "-O0")
	elseif(CMAKE_BUILD_TYPE STREQUAL "Release")																	# Release build compiler options
		target_compile_options(EMCrashHandler PRIVATE "-O3" "-flto")
	endif()
endif()
