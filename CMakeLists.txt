cmake_minimum_required(VERSION 2.8.12)

# define constants
add_definitions(-DGAME_VERSION="2.0.0")
add_definitions("-DGLM_FORCE_RADIANS")
add_definitions("-DGLM_ENABLE_EXPERIMENTAL")
add_definitions("-DAE_NO_NETWORK")

# projects
project(emptyclip)

# add extra find modules
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

# mingw
if(WIN32)
	set(EXTRA_LIBS ${EXTRA_LIBS} winmm ws2_32)

	# disable console window
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-subsystem,windows")
endif()

# enable editor
if("$ENV{ENABLE_EDITOR}")
	add_definitions("-DENABLE_EDITOR")
endif()

# compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-result -Wno-switch -Wno-unused-parameter -pedantic -std=c++17")
if("${CMAKE_HOST_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse -mfpmath=sse")
endif()

# set gl preference
if(NOT DEFINED OpenGL_GL_PREFERENCE)
	set(OpenGL_GL_PREFERENCE GLVND)
endif()

# find libraries
find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(OpenAL REQUIRED)
find_package(Vorbis REQUIRED)
find_package(Ogg REQUIRED)
find_package(Freetype REQUIRED)
find_package(ZLIB REQUIRED)
find_package(SQLite REQUIRED)
find_package(Threads REQUIRED)

# set include directories
include_directories("src")
include_directories("ext")
include_directories("ext/ae")
include_directories(${SDL2_INCLUDE_DIR})
include_directories(${SDL2_IMAGE_INCLUDE_DIR})
include_directories(${OPENAL_INCLUDE_DIR})
include_directories(${VORBIS_INCLUDE_DIR})
include_directories(${OGG_INCLUDE_DIR})
include_directories(${FREETYPE_INCLUDE_DIRS})
include_directories(${SQLITE_INCLUDE_DIR})
include_directories(${ZLIB_INCLUDE_DIRS})

# add source code
file(GLOB_RECURSE SRC_ALL
	src/*.cpp
	src/*.h
	ext/*.cpp
	ext/*.h
	ext/*.hpp
	deployment/resource.rc
)

# build binary
add_executable(${CMAKE_PROJECT_NAME} ${SRC_ALL})

# set output path
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/working/)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${CMAKE_PROJECT_NAME}_debug)
endif()

# add command to build data
if(EXISTS "${PROJECT_SOURCE_DIR}/assets/source/process.sh" AND NOT WIN32)

	# pack data
	file(GLOB SOURCE_OGGS "${PROJECT_SOURCE_DIR}/assets/source/sounds/*.ogg")
	add_custom_command(
		OUTPUT "${PROJECT_SOURCE_DIR}/working/data/sounds"
		COMMAND "${PROJECT_SOURCE_DIR}/assets/source/process.sh"
		WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/assets/source/"
		DEPENDS "${SOURCE_OGGS}"
	)
	add_custom_target(data ALL DEPENDS "${PROJECT_SOURCE_DIR}/working/data/sounds")

	# build stats
	file(GLOB SOURCE_TSVS "${PROJECT_SOURCE_DIR}/assets/source/stats/*.tsv")
	add_custom_command(
		OUTPUT "${PROJECT_SOURCE_DIR}/working/data/stats.db"
		COMMAND "${PROJECT_SOURCE_DIR}/assets/source/build_stats.sh"
		WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/assets/source/"
		DEPENDS "${SOURCE_TSVS}"
	)
	add_custom_target(stats ALL DEPENDS "${PROJECT_SOURCE_DIR}/working/data/stats.db")
endif()

# add custom target to generate version.h
add_custom_target(
	version
	${CMAKE_COMMAND}
	-D SRC=${PROJECT_SOURCE_DIR}/cmake/version.h.in
	-D DST=${PROJECT_SOURCE_DIR}/src/version.h
	-P ${PROJECT_SOURCE_DIR}/cmake/version.cmake
)
add_dependencies(${CMAKE_PROJECT_NAME} version)

# link libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
	${OPENGL_LIBRARIES}
	${SDL2_LIBRARY}
	${SDL2_IMAGE_LIBRARIES}
	${OPENAL_LIBRARY}
	${VORBIS_LIBRARIES}
	${OGG_LIBRARIES}
	${LUA_LIBRARIES}
	${FREETYPE_LIBRARIES}
	${SQLITE_LIBRARIES}
	${ZLIB_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
	${EXTRA_LIBS}
)

# install
if(WIN32)
else()

	# linux installation
	install(TARGETS ${CMAKE_PROJECT_NAME} RUNTIME DESTINATION share/games/${CMAKE_PROJECT_NAME})
	install(DIRECTORY ${PROJECT_SOURCE_DIR}/working/ DESTINATION share/games/${CMAKE_PROJECT_NAME} PATTERN working/${CMAKE_PROJECT_NAME} EXCLUDE PATTERN "working/*")
	install(FILES ${PROJECT_SOURCE_DIR}/deployment/${CMAKE_PROJECT_NAME}.png DESTINATION share/icons/hicolor/128x128/apps)
	install(FILES ${PROJECT_SOURCE_DIR}/deployment/${CMAKE_PROJECT_NAME}.desktop DESTINATION share/applications)
	install(FILES ${PROJECT_SOURCE_DIR}/deployment/${CMAKE_PROJECT_NAME}.xml DESTINATION share/metainfo)
	install(FILES ${PROJECT_SOURCE_DIR}/CHANGELOG DESTINATION share/doc/${CMAKE_PROJECT_NAME})
	install(FILES ${PROJECT_SOURCE_DIR}/LICENSE DESTINATION share/doc/${CMAKE_PROJECT_NAME})
	install(FILES ${PROJECT_SOURCE_DIR}/README DESTINATION share/doc/${CMAKE_PROJECT_NAME})

	# generate the script to launch the program
	configure_file(${PROJECT_SOURCE_DIR}/deployment/${CMAKE_PROJECT_NAME} ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles)
	install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${CMAKE_PROJECT_NAME} DESTINATION bin)
endif()
