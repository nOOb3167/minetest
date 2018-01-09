cmake_minimum_required(VERSION 2.6)

project(minetest)

INCLUDE(CheckIncludeFiles)

set(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}" CACHE STRING
	"Choose the type of build. Options are: None Debug SemiDebug RelWithDebInfo MinSizeRel."
	FORCE
)

set(ZLIB_DLL "" CACHE FILEPATH "Path to zlib DLL (for installation)")
set(ZLIBWAPI_DLL "" CACHE FILEPATH "Path to zlibwapi DLL")
set(OPENAL_DLL "" CACHE FILEPATH "Path to OpenAL32.dll for installation (optional)")
set(OGG_DLL "" CACHE FILEPATH "Path to libogg.dll for installation (optional)")
set(VORBIS_DLL "" CACHE FILEPATH "Path to libvorbis.dll for installation (optional)")
set(VORBISFILE_DLL "" CACHE FILEPATH "Path to libvorbisfile.dll for installation (optional)")
set(LUA_DLL "" CACHE FILEPATH "Path to lua51.dll for installation (optional) (LuaJIT)")
set(FREETYPE_DLL "" CACHE FILEPATH "Path to freetype dll for installation (optional)")
set(SQLITE3_DLL "" CACHE FILEPATH "Path to sqlite3 dll for installation (optional)")
set(LEVELDB_DLL "" CACHE FILEPATH "Path to leveldb dll for installation (optional)")
# CURL_DLL     - find_package(CURL) will define if found
# IRRLICHT_DLL - find_package(Irrlicht) will define if found
# GETTEXT_DLL - find_package(Gettext) (see macro GETTEXT_HELPER)
# GETTEXT_ICONV_DLL - find_package(Gettext) (see macro GETTEXT_HELPER)

# Windows Specific - instead of finding the library via find_package(ZLIB REQUIRED)
set(ZLIB_INCLUDE_DIR "-NOTFOUND"
		CACHE PATH "Zlib include directory")
set(ZLIB_LIBRARIES "-NOTFOUND"
		CACHE FILEPATH "Path to zlib library (usually zlibwapi.lib)")
# Windows Specific - help CMake find Irrlicht
set(IRRLICHT_SOURCE_DIR "-NOTFOUND"
		CACHE PATH "irrlicht dir")
# Windows Specific - help CMake find Freetype
set(FREETYPE_INCLUDE_DIR_ft2build "-NOTFOUND"
		CACHE PATH "freetype include dir")
set(FREETYPE_INCLUDE_DIR_freetype2 "-NOTFOUND"
		CACHE PATH "freetype include dir")
set(FREETYPE_LIBRARY "-NOTFOUND"
		CACHE FILEPATH "Path to freetype247.lib")

# FIXME: proper FindENet.cmake		
set(ENET_INCLUDE_DIR "-NOTFOUND"
		CACHE PATH "ENet include directory")
set(ENET_LIBRARIES "-NOTFOUND"
		CACHE FILEPATH "Path to ENet library")
set(OPUS_INCLUDE_DIR "-NOTFOUND"
		CACHE PATH "Opus include directory")
set(OPUS_LIBRARIES "-NOTFOUND"
		CACHE FILEPATH "Path to Opus library")

option(APPLY_LOCALE_BLACKLIST "Use a blacklist to avoid broken locales" TRUE)
set(GETTEXT_BLACKLISTED_LOCALES be he ko ky zh_CN zh_TW CACHE STRING "Blacklisted locales that don't work. see issue #4638")

option(ENABLE_CURL "Enable cURL support for fetching media" TRUE)
option(ENABLE_GETTEXT "Use GetText for internationalization" FALSE)
option(ENABLE_FREETYPE "Enable FreeType2 (TrueType fonts and basic unicode support)" TRUE)
option(ENABLE_CURSES "Enable ncurses console" TRUE)
option(ENABLE_SOUND "Enable sound" TRUE)

option(ENABLE_PostgreSQL "Enable PostgreSQL backend" TRUE)
option(ENABLE_LEVELDB "Enable LevelDB backend" TRUE)
option(ENABLE_REDIS "Enable Redis backend" TRUE)
option(ENABLE_SPATIAL "Enable SpatialIndex AreaStore backend" TRUE)

set(USE_CURL FALSE)
set(USE_GETTEXT FALSE)
set(USE_FREETYPE FALSE)
set(USE_CURSES FALSE)
set(USE_SOUND FALSE)

set(USE_PostgreSQL FALSE)
set(USE_LEVELDB FALSE)
set(USE_REDIS FALSE)
set(USE_SPATIAL FALSE)

INCLUDE(GettextHelper)
GETTEXT_HELPER(${ENABLE_GETTEXT} ${GETTEXT_BLACKLISTED_LOCALES})

find_package(CURL)
find_package(Freetype)
find_package(Ncursesw)
find_package(OpenAL)
find_package(Vorbis)

find_package(PostgreSQL)
find_package(LevelDb)
find_package(Redis)
find_package(Spatial)

find_package(SQLite3 REQUIRED)

if(NOT (BUILD_CLIENT OR BUILD_SERVER))
	message(WARNING "Neither BUILD_CLIENT nor BUILD_SERVER is set! Setting BUILD_SERVER=true")
	set(BUILD_SERVER TRUE)
endif()


if(ENABLE_CURL AND CURL_FOUND)
	set(USE_CURL TRUE)
endif()
if(ENABLE_GETTEXT AND GETTEXT_FOUND)
	set(USE_GETTEXT TRUE)
endif()
if(ENABLE_FREETYPE AND FREETYPE_FOUND)
	set(USE_FREETYPE TRUE)
endif()
if(ENABLE_CURSES AND CURSES_FOUND)
	set(USE_CURSES TRUE)
endif()

if(BUILD_CLIENT AND ENABLE_SOUND AND OPENAL_FOUND AND VORBIS_FOUND)
	set(USE_SOUND TRUE)
endif()

if(ENABLE_PostgreSQL AND PostgreSQL_FOUND)
	set(USE_PostgreSQL TRUE)
endif()
if(ENABLE_LEVELDB AND LEVELDB_FOUND)
	set(USE_LEVELDB TRUE)
endif()
if(ENABLE_REDIS AND REDIS_FOUND)
	set(USE_REDIS TRUE)
endif()
if(ENABLE_SPATIAL AND SPATIAL_FOUND)
	set(USE_SPATIAL TRUE)
endif()

message(STATUS "============")
message(STATUS "CURL: ${USE_CURL}")
message(STATUS "GETTEXT: ${USE_GETTEXT}")
message(STATUS "FREETYPE: ${USE_FREETYPE}")
message(STATUS "CURSES: ${USE_CURSES}")
message(STATUS "SOUND: ${USE_SOUND}")
message(STATUS "PostgreSQL: ${USE_PostgreSQL}")
message(STATUS "LEVELDB: ${USE_LEVELDB}")
message(STATUS "REDIS: ${USE_REDIS}")
message(STATUS "SPATIAL: ${USE_SPATIAL}")
message(STATUS "============")

if(NOT USE_CURL)
	if (BUILD_CLIENT)
		message(WARNING "cURL is required to load the server list")
	endif()
	if (BUILD_SERVER)
		message(WARNING "cURL is required to announce to the server list")
	endif()
endif()

if(BUILD_CLIENT AND ENABLE_SOUND AND NOT (OPENAL_FOUND AND VORBIS_FOUND))
	if(NOT OPENAL_FOUND)
		message(WARNING "Sound enabled, but OpenAL not found!")
	endif()
	if (NOT VORBIS_FOUND)
		message(STATUS "Sound enabled, but Vorbis (and/or Ogg) libraries not found!")
	endif()
	message(FATAL_ERROR "Sound enabled, but cannot be used.\n"
		"To continue, either fill in the required paths or disable sound. (-DENABLE_SOUND=0)")
endif()

include(${PROJECT_SOURCE_DIR}/CMakeListsSources.cmake)

if(USE_SOUND)
	list(APPEND client_SRCS ${sound_SRCS})
endif()
if(USE_FREETYPE)
	list(APPEND client_SRCS ${client_irrlicht_changes_freetype_SRCS})
endif()

check_include_files(endian.h HAVE_ENDIAN_H)

configure_file(
	"${PROJECT_SOURCE_DIR}/cmake_config.h.in"
	"${PROJECT_BINARY_DIR}/cmake_config.h"
)

# This gives us the icon and file version information
set(WINRESOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../misc/winresource.rc")
set(MINETEST_EXE_MANIFEST_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../misc/minetest.exe.manifest")
set(extra_windows_SRCS ${WINRESOURCE_FILE} ${MINETEST_EXE_MANIFEST_FILE})

# Add a target that always rebuilds cmake_config_githash.h
add_custom_target(GenerateVersion
	COMMAND ${CMAKE_COMMAND}
	-D "GENERATE_VERSION_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
	-D "GENERATE_VERSION_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
	-D "VERSION_STRING=${VERSION_STRING}"
	-D "DEVELOPMENT_BUILD=${DEVELOPMENT_BUILD}"
	-P "${CMAKE_SOURCE_DIR}/cmake/Modules/GenerateVersion.cmake"
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

macro(FIXME_FILTER_NOTFOUND)
	foreach(VV IN ITEMS ${ARGV})
		if(${VV} MATCHES "-NOTFOUND$")
			unset(${VV})
			unset(${VV} CACHE)
		endif()
	endforeach()
endmacro()

FIXME_FILTER_NOTFOUND(
	CURL_INCLUDE_DIR
	GETTEXT_INCLUDE_DIR
	FREETYPE_INCLUDE_DIRS
	CURSES_INCLUDE_DIRS
	OPENAL_INCLUDE_DIR
	VORBIS_INCLUDE_DIR
	OGG_INCLUDE_DIR
	PostgreSQL_INCLUDE_DIR
	LEVELDB_INCLUDE_DIR
	REDIS_INCLUDE_DIR
	SPATIAL_INCLUDE_DIR
	ENET_INCLUDE_DIR
    OPUS_INCLUDE_DIR
)

set(INCLUDE_DIRECTORIES_EVERYTHING
	${PROJECT_BINARY_DIR}
	${PROJECT_SOURCE_DIR}
	${PROJECT_SOURCE_DIR}/script
	${CMAKE_BUILD_TYPE}
	${CURL_INCLUDE_DIR}
	${GETTEXT_INCLUDE_DIR}
	${FREETYPE_INCLUDE_DIRS}
	${CURSES_INCLUDE_DIRS}
	${OPENAL_INCLUDE_DIR}
	${VORBIS_INCLUDE_DIR}
	${OGG_INCLUDE_DIR}
	${PostgreSQL_INCLUDE_DIR}
	${LEVELDB_INCLUDE_DIR}
	${REDIS_INCLUDE_DIR}
	${SPATIAL_INCLUDE_DIR}
	${ENET_INCLUDE_DIR}
    ${OPUS_INCLUDE_DIR}
	${LUA_INCLUDE_DIR}
	${GMP_INCLUDE_DIR}
	${JSON_INCLUDE_DIR}
	${ZLIB_INCLUDE_DIR}
	${SQLITE3_INCLUDE_DIR}
	${IRRLICHT_INCLUDE_DIR}
	${PNG_INCLUDE_DIR}
	${X11_INCLUDE_DIR}
)

if(BUILD_CLIENT)
	source_group(TREE ${PROJECT_SOURCE_DIR} PREFIX "Source Files" FILES ${client_SRCS})
	add_executable(${PROJECT_NAME} ${client_SRCS} ${extra_windows_SRCS})
	add_dependencies(${PROJECT_NAME} GenerateVersion)
	target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_DIRECTORIES_EVERYTHING})
	target_link_libraries(${PROJECT_NAME} PUBLIC
		${LUA_LIBRARY}
		${GMP_LIBRARY}
		${JSON_LIBRARY}
		${ZLIB_LIBRARIES}
		${SQLITE3_LIBRARY}
		${IRRLICHT_LIBRARY}
		${PNG_LIBRARIES}
		${JPEG_LIBRARIES}
		${BZIP2_LIBRARIES}
		${OPENGL_LIBRARIES}
		${ENET_LIBRARIES} winmm.lib
        ${OPUS_LIBRARIES}
		ws2_32.lib version.lib shlwapi.lib dbghelp.lib
	)
	if(USE_CURL)
		target_link_libraries(${PROJECT_NAME} PUBLIC ${CURL_LIBRARY})
	endif()
	if(USE_GETTEXT)
		target_link_libraries(${PROJECT_NAME} PUBLIC ${GETTEXT_LIBRARY})
	endif()
	if(USE_FREETYPE)
		target_link_libraries(${PROJECT_NAME} PUBLIC ${FREETYPE_LIBRARY})
	endif()
	if (USE_CURSES)
		target_link_libraries(${PROJECT_NAME} PUBLIC ${CURSES_LIBRARIES})
	endif()
	if (USE_SOUND)
		target_link_libraries(${PROJECT_NAME} PUBLIC ${OPENAL_LIBRARY})
		target_link_libraries(${PROJECT_NAME} PUBLIC ${VORBIS_LIBRARIES})
	endif()
	if (USE_POSTGRESQL)
		target_link_libraries(${PROJECT_NAME} PUBLIC ${PostgreSQL_LIBRARY})
	endif()
	if (USE_LEVELDB)
		target_link_libraries(${PROJECT_NAME} PUBLIC ${LEVELDB_LIBRARY})
	endif()
	if (USE_REDIS)
		target_link_libraries(${PROJECT_NAME} PUBLIC ${REDIS_LIBRARY})
	endif()
	if (USE_SPATIAL)
		target_link_libraries(${PROJECT_NAME} PUBLIC ${SPATIAL_LIBRARY})
	endif()
endif(BUILD_CLIENT)


if(BUILD_SERVER)
	source_group(TREE ${PROJECT_SOURCE_DIR} PREFIX "Source Files" FILES ${server_SRCS})
	add_executable(${PROJECT_NAME}server ${server_SRCS} ${extra_windows_SRCS})
	add_dependencies(${PROJECT_NAME}server GenerateVersion)
	target_include_directories(${PROJECT_NAME}server PUBLIC ${INCLUDE_DIRECTORIES_EVERYTHING})
	target_link_libraries(${PROJECT_NAME}server PUBLIC
		${LUA_LIBRARY}
		${GMP_LIBRARY}
		${JSON_LIBRARY}
		${ZLIB_LIBRARIES}
		${SQLITE3_LIBRARY}
		${ENET_LIBRARIES} winmm.lib
        ${OPUS_LIBRARIES}
		ws2_32.lib version.lib shlwapi.lib dbghelp.lib
	)
	if(USE_CURL)
		target_link_libraries(${PROJECT_NAME}server PUBLIC ${CURL_LIBRARY})
	endif()
	if(USE_GETTEXT)
		target_link_libraries(${PROJECT_NAME} PUBLIC ${GETTEXT_LIBRARY})
	endif()
	if (USE_CURSES)
		target_link_libraries(${PROJECT_NAME}server ${CURSES_LIBRARIES})
	endif()
	if (USE_POSTGRESQL)
		target_link_libraries(${PROJECT_NAME}server ${POSTGRESQL_LIBRARY})
	endif()
	if (USE_LEVELDB)
		target_link_libraries(${PROJECT_NAME}server ${LEVELDB_LIBRARY})
	endif()
	if (USE_REDIS)
		target_link_libraries(${PROJECT_NAME}server ${REDIS_LIBRARY})
	endif()
	if (USE_SPATIAL)
		target_link_libraries(${PROJECT_NAME}server ${SPATIAL_LIBRARY})
	endif()
	set_target_properties(${PROJECT_NAME}server PROPERTIES
		COMPILE_DEFINITIONS "SERVER")
endif(BUILD_SERVER)

set_property(TARGET ${PROJECT_NAME} ${PROJECT_NAME}server PROPERTY RUNTIME_OUTPUT_DIRECTORY
	"${CMAKE_SOURCE_DIR}/bin"
)

set(TMP_COMPILE_DEFINITIONS
	WIN32_LEAN_AND_MEAN
	USE_CMAKE_CONFIG_H
	# Surpress some useless warnings
	_CRT_SECURE_NO_DEPRECATE /W1
	# Get M_PI to work
	_USE_MATH_DEFINES
	# Dont define min/max macros in minwindef.h
	NOMINMAX

	$<$<CONFIG:Release>: NDEBUG _HAS_ITERATOR_DEBUGGING=0>
)
target_compile_definitions(${PROJECT_NAME}       PUBLIC ${TMP_COMPILE_DEFINITIONS})
target_compile_definitions(${PROJECT_NAME}server PUBLIC ${TMP_COMPILE_DEFINITIONS})

set(TMP_COMPILE_OPTIONS
	/MP
	$<$<CONFIG:Release>:  /EHa /Ox /GL /FD /MT /GS- /Zi /fp:fast /TP>
	$<$<CONFIG:SemiDebug>: /MDd /Zi /Ob0 /O1 /RTC1>
	$<$<CONFIG:Debug>:     /MDd /Zi /Ob0 /Od /RTC1>
	$<$<EQUAL:${CMAKE_SIZEOF_VOID_P},4>: /arch:SSE>
)
target_compile_options(${PROJECT_NAME}       PUBLIC ${TMP_COMPILE_OPTIONS})
target_compile_options(${PROJECT_NAME}server PUBLIC ${TMP_COMPILE_OPTIONS})

set_property(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY LINK_FLAGS_RELEASE
	"/LTCG /INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
)

install(FILES
	${ZLIB_DLL}
	${ZLIBWAPI_DLL}
	${OPENAL_DLL}
	${OGG_DLL}
	${VORBIS_DLL}
	${VORBISFILE_DLL}
	${LUA_DLL}
	${FREETYPE_DLL}
	${SQLITE3_DLL}
	${LEVELDB_DLL}
	${CURL_DLL}
	${IRRLICHT_DLL}
	DESTINATION ${BINDIR}
)

if(BUILD_CLIENT)
	install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION ${BINDIR}
		LIBRARY DESTINATION ${BINDIR}
		ARCHIVE DESTINATION ${BINDIR}
		BUNDLE DESTINATION .
	)
endif(BUILD_CLIENT)

if(BUILD_SERVER)
	install(TARGETS ${PROJECT_NAME}server DESTINATION ${BINDIR})
endif()
