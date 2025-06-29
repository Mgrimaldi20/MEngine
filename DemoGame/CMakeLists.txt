# CMakeList.txt : CMake project for MEngine, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_library(DemoGame SHARED)

if(CMAKE_VERSION VERSION_GREATER 3.25)
	set_property(TARGET DemoGame PROPERTY CXX_STANDARD 20)
	set_property(TARGET DemoGame PROPERTY C_STANDARD 17)
endif()

# Common source files, all this code is Operating System independent
target_sources(DemoGame PRIVATE
	"src/game.c"
)

# Specify the .def file for the linker
if(WIN32)
	message(STATUS "Binary output directory set: ${CMAKE_CURRENT_SOURCE_DIR}")
	set_target_properties(DemoGame PROPERTIES
		LINK_FLAGS "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/demogame.def"
	)
endif()

# Set up all the compiler options here
if(MSVC)
	target_compile_options(DemoGame PRIVATE "/W4" "/WX" "/permissive-" "/analyze" "/fp:fast" "/FAs")			# Common options for all build types
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")																		# Debug build compiler options
		target_compile_options(DemoGame PRIVATE "/Zi" "/fsanitize=address" "/Od" "/MDd" "/JMC")
	elseif(CMAKE_BUILD_TYPE STREQUAL "Release")																	# Release build compiler options
		target_compile_options(DemoGame PRIVATE "/O2" "/MD" "/GL" "/Gw")
	endif()

elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_compile_options(DemoGame PRIVATE "-Wall" "-Werror" "-Wpedantic" "-ffast-math" "-fanalyzer")			# Common options for all build types
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")																		# Debug build compiler options
		target_compile_options(DemoGame PRIVATE
			"-g"
			#"-fsanitize=leak"
			#"-fsanitize=undefined"
			#"-fsanitize=address"
			#"-fsanitize=thread"
			"-O0"
		)
		target_link_libraries(DemoGame PRIVATE
			#"-fsanitize=leak"
			#"-fsanitize=undefined"
			#"-fsanitize=address"
			#"-fsanitize=thread"
		)
	elseif(CMAKE_BUILD_TYPE STREQUAL "Release")																	# Release build compiler options
		target_compile_options(DemoGame PRIVATE "-O3" "-flto")
	endif()

elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(DemoGame PRIVATE "-Wall" "-Werror" "-Wpedantic" "-ffast-math" "-fanalyzer")			# Common options for all build types
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")																		# Debug build compiler options
		target_compile_options(DemoGame PRIVATE
			"-g"
			#"-fsanitize=leak"
			#"-fsanitize=undefined"
			#"-fsanitize=address"
			#"-fsanitize=thread"
			"-O0"
		)
		target_link_libraries(DemoGame PRIVATE
			#"-fsanitize=leak"
			#"-fsanitize=undefined"
			#"-fsanitize=address"
			#"-fsanitize=thread"
		)
	elseif(CMAKE_BUILD_TYPE STREQUAL "Release")											# Release build compiler options
		target_compile_options(DemoGame PRIVATE "-O3" "-flto")
	endif()
endif()
