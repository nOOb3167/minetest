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

check_include_files(endian.h HAVE_ENDIAN_H)

configure_file(
	"${PROJECT_SOURCE_DIR}/cmake_config.h.in"
	"${PROJECT_BINARY_DIR}/cmake_config.h"
)

set(common_SRCS
	database/database.cpp
	database/database.h
	database/database-dummy.cpp
	database/database-dummy.h
	database/database-files.cpp
	database/database-files.h
	database/database-leveldb.cpp
	database/database-leveldb.h
	database/database-postgresql.cpp
	database/database-postgresql.h
	database/database-redis.cpp
	database/database-redis.h
	database/database-sqlite3.cpp
	database/database-sqlite3.h
	mapgen/cavegen.cpp
	mapgen/cavegen.h
	mapgen/dungeongen.cpp
	mapgen/dungeongen.h
	mapgen/mapgen_carpathian.cpp
	mapgen/mapgen_carpathian.h
	mapgen/mapgen.cpp
	mapgen/mapgen.h
	mapgen/mapgen_flat.cpp
	mapgen/mapgen_flat.h
	mapgen/mapgen_fractal.cpp
	mapgen/mapgen_fractal.h
	mapgen/mapgen_singlenode.cpp
	mapgen/mapgen_singlenode.h
	mapgen/mapgen_v5.cpp
	mapgen/mapgen_v5.h
	mapgen/mapgen_v6.cpp
	mapgen/mapgen_v6.h
	mapgen/mapgen_v7.cpp
	mapgen/mapgen_v7.h
	mapgen/mapgen_valleys.cpp
	mapgen/mapgen_valleys.h
	mapgen/mg_biome.cpp
	mapgen/mg_biome.h
	mapgen/mg_decoration.cpp
	mapgen/mg_decoration.h
	mapgen/mg_ore.cpp
	mapgen/mg_ore.h
	mapgen/mg_schematic.cpp
	mapgen/mg_schematic.h
	mapgen/treegen.cpp
	mapgen/treegen.h
	network/address.cpp
	network/address.h
	network/connection.cpp
	network/connection.h
	network/connectionthreads.cpp
	network/connectionthreads.h
	network/networkpacket.cpp
	network/networkpacket.h
	network/serverpackethandler.cpp
	network/serveropcodes.cpp
	network/serveropcodes.h
	network/socket.cpp
	network/socket.h
	script/common/c_content.cpp
	script/common/c_content.h
	script/common/c_converter.cpp
	script/common/c_converter.h
	script/common/c_types.cpp
	script/common/c_types.h
	script/common/c_internal.cpp
	script/common/c_internal.h
	script/cpp_api/s_async.cpp
	script/cpp_api/s_async.h
	script/cpp_api/s_base.cpp
	script/cpp_api/s_base.h
	script/cpp_api/s_entity.cpp
	script/cpp_api/s_entity.h
	script/cpp_api/s_env.cpp
	script/cpp_api/s_env.h
	script/cpp_api/s_inventory.cpp
	script/cpp_api/s_inventory.h
	script/cpp_api/s_item.cpp
	script/cpp_api/s_item.h
	script/cpp_api/s_modchannels.cpp
	script/cpp_api/s_modchannels.h
	script/cpp_api/s_node.cpp
	script/cpp_api/s_node.h
	script/cpp_api/s_nodemeta.cpp
	script/cpp_api/s_nodemeta.h
	script/cpp_api/s_player.cpp
	script/cpp_api/s_player.h
	script/cpp_api/s_security.cpp
	script/cpp_api/s_security.h
	script/cpp_api/s_server.cpp
	script/cpp_api/s_server.h
	script/lua_api/l_areastore.cpp
	script/lua_api/l_areastore.h
	script/lua_api/l_base.cpp
	script/lua_api/l_base.h
	script/lua_api/l_craft.cpp
	script/lua_api/l_craft.h
	script/lua_api/l_env.cpp
	script/lua_api/l_env.h
	script/lua_api/l_inventory.cpp
	script/lua_api/l_inventory.h
	script/lua_api/l_item.cpp
	script/lua_api/l_item.h
	script/lua_api/l_itemstackmeta.cpp
	script/lua_api/l_itemstackmeta.h
	script/lua_api/l_mapgen.cpp
	script/lua_api/l_mapgen.h
	script/lua_api/l_metadata.cpp
	script/lua_api/l_metadata.h
	script/lua_api/l_modchannels.cpp
	script/lua_api/l_modchannels.h
	script/lua_api/l_nodemeta.cpp
	script/lua_api/l_nodemeta.h
	script/lua_api/l_nodetimer.cpp
	script/lua_api/l_nodetimer.h
	script/lua_api/l_noise.cpp
	script/lua_api/l_noise.h
	script/lua_api/l_object.cpp
	script/lua_api/l_object.h
	script/lua_api/l_particles.cpp
	script/lua_api/l_particles.h
	script/lua_api/l_rollback.cpp
	script/lua_api/l_rollback.h
	script/lua_api/l_server.cpp
	script/lua_api/l_server.h
	script/lua_api/l_storage.cpp
	script/lua_api/l_storage.h
	script/lua_api/l_util.cpp
	script/lua_api/l_util.h
	script/lua_api/l_vmanip.cpp
	script/lua_api/l_vmanip.h
	script/lua_api/l_settings.cpp
	script/lua_api/l_settings.h
	script/lua_api/l_http.cpp
	script/lua_api/l_http.h
	script/scripting_server.cpp
	script/scripting_server.h
	threading/event.cpp
	threading/event.h
	threading/thread.cpp
	threading/thread.h
	threading/semaphore.cpp
	threading/semaphore.h
	util/areastore.cpp
	util/areastore.h
	util/auth.cpp
	util/auth.h
	util/base64.cpp
	util/base64.h
	util/directiontables.cpp
	util/directiontables.h
	util/enriched_string.cpp
	util/enriched_string.h
	util/numeric.cpp
	util/numeric.h
	util/pointedthing.cpp
	util/pointedthing.h
	util/serialize.cpp
	util/serialize.h
	util/sha1.cpp
	util/sha1.h
	util/sha256.c
	util/string.cpp
	util/string.h
	util/srp.cpp
	util/srp.h
	util/timetaker.cpp
	util/timetaker.h
	unittest/test.cpp
	unittest/test.h
	unittest/test_areastore.cpp
	unittest/test_collision.cpp
	unittest/test_compression.cpp
	unittest/test_connection.cpp
	unittest/test_filepath.cpp
	unittest/test_inventory.cpp
	unittest/test_map_settings_manager.cpp
	unittest/test_mapnode.cpp
	unittest/test_modchannels.cpp
	unittest/test_nodedef.cpp
	unittest/test_noderesolver.cpp
	unittest/test_noise.cpp
	unittest/test_objdef.cpp
	unittest/test_player.cpp
	unittest/test_profiler.cpp
	unittest/test_random.cpp
	unittest/test_schematic.cpp
	unittest/test_serialization.cpp
	unittest/test_settings.cpp
	unittest/test_socket.cpp
	unittest/test_threading.cpp
	unittest/test_utilities.cpp
	unittest/test_voxelalgorithms.cpp
	unittest/test_voxelmanipulator.cpp
	ban.cpp
	ban.h
	chat.cpp
	chat.h
	clientiface.cpp
	clientiface.h
	collision.cpp
	collision.h
	content_mapnode.cpp
	content_mapnode.h
	content_nodemeta.cpp
	content_nodemeta.h
	content_sao.cpp
	content_sao.h
	convert_json.cpp
	convert_json.h
	craftdef.cpp
	craftdef.h
	debug.cpp
	debug.h
	defaultsettings.cpp
	defaultsettings.h
	emerge.cpp
	emerge.h
	environment.cpp
	environment.h
	face_position_cache.cpp
	face_position_cache.h
	filesys.cpp
	filesys.h
	genericobject.cpp
	genericobject.h
	gettext.cpp
	gettext.h
	httpfetch.cpp
	httpfetch.h
	inventory.cpp
	inventory.h
	inventorymanager.cpp
	inventorymanager.h
	itemdef.cpp
	itemdef.h
	itemstackmetadata.cpp
	itemstackmetadata.h
	light.cpp
	light.h
	log.cpp
	log.h
	map.cpp
	map.h
	map_settings_manager.cpp
	map_settings_manager.h
	mapblock.cpp
	mapblock.h
	mapnode.cpp
	mapnode.h
	mapsector.cpp
	mapsector.h
	metadata.cpp
	metadata.h
	modchannels.cpp
	modchannels.h
	mods.cpp
	mods.h
	nameidmapping.cpp
	nameidmapping.h
	nodedef.cpp
	nodedef.h
	nodemetadata.cpp
	nodemetadata.h
	nodetimer.cpp
	nodetimer.h
	noise.cpp
	noise.h
	objdef.cpp
	objdef.h
	object_properties.cpp
	object_properties.h
	pathfinder.cpp
	pathfinder.h
	player.cpp
	player.h
	porting.cpp
	porting.h
	profiler.cpp
	profiler.h
	quicktune.cpp
	quicktune.h
	raycast.cpp
	raycast.h
	reflowscan.cpp
	reflowscan.h
	remoteplayer.cpp
	remoteplayer.h
	rollback.cpp
	rollback.h
	rollback_interface.cpp
	rollback_interface.h
	serialization.cpp
	serialization.h
	server.cpp
	server.h
	serverenvironment.cpp
	serverenvironment.h
	serverlist.cpp
	serverlist.h
	serverobject.cpp
	serverobject.h
	settings.cpp
	settings.h
	sound.cpp
	sound.h
	staticobject.cpp
	staticobject.h
	subgame.cpp
	subgame.h
	terminal_chat_console.cpp
	terminal_chat_console.h
	tileanimation.cpp
	tileanimation.h
	tool.cpp
	tool.h
	translation.cpp
	translation.h
	version.cpp
	version.h
	voxel.cpp
	voxel.h
	voxelalgorithms.cpp
	voxelalgorithms.h
)

set(sound_SRCS
	sound_openal.cpp
	sound_openal.h
)

set(client_irrlicht_changes_freetype_SRCS
	irrlicht_changes/CGUITTFont.cpp
	irrlicht_changes/CGUITTFont.h
)

set(client_SRCS
	${common_SRCS}
	client/render/anaglyph.cpp
	client/render/anaglyph.h
	client/render/core.cpp
	client/render/core.h
	client/render/factory.cpp
	client/render/factory.h
	client/render/interlaced.cpp
	client/render/interlaced.h
	client/render/pageflip.cpp
	client/render/pageflip.h
	client/render/plain.cpp
	client/render/plain.h
	client/render/sidebyside.cpp
	client/render/sidebyside.h
	client/render/stereo.cpp
	client/render/stereo.h
	client/renderingengine.cpp
	client/renderingengine.h
	client/vserv/vserv_clnt.cpp
	client/vserv/vserv_clnt.h
	client/vserv/vserv_clnt_helpers.h
	client/vserv/vserv_clnt_iface.h
	client/vserv/vserv_clnt_test.cpp
	client/vserv/vserv_helpers.cpp
	client/vserv/vserv_helpers.h
	client/vserv/vserv_log.h
	client/vserv/vserv_misc.h
	client/vserv/vserv_misc.cpp
	client/vserv/vserv_openal_include.h
	client/vserv/vserv_pinger.cpp
	client/vserv/vserv_pinger.h
	client/vserv/vserv_playback.cpp
	client/vserv/vserv_playback.h
	client/vserv/vserv_record.cpp
	client/vserv/vserv_record.h
	client/clientlauncher.cpp
	client/clientlauncher.h
	client/inputhandler.cpp
	client/inputhandler.h
	client/tile.cpp
	client/tile.h
	client/joystick_controller.cpp
	client/joystick_controller.h
	gui/guiChatConsole.cpp
	gui/guiChatConsole.h
	gui/guiEditBoxWithScrollbar.cpp
	gui/guiEditBoxWithScrollbar.h
	gui/guiEngine.cpp
	gui/guiEngine.h
	gui/guiFormSpecMenu.cpp
	gui/guiFormSpecMenu.h
	gui/guiKeyChangeMenu.cpp
	gui/guiKeyChangeMenu.h
	gui/guiPasswordChange.cpp
	gui/guiPasswordChange.h
	gui/guiPathSelectMenu.cpp
	gui/guiPathSelectMenu.h
	gui/guiTable.cpp
	gui/guiTable.h
	gui/guiVolumeChange.cpp
	gui/guiVolumeChange.h
	gui/intlGUIEditBox.cpp
	gui/intlGUIEditBox.h
	irrlicht_changes/static_text.cpp
	irrlicht_changes/static_text.h
	network/clientopcodes.cpp
	network/clientopcodes.h
	network/clientpackethandler.cpp
	script/cpp_api/s_client.cpp
	script/cpp_api/s_client.h
	script/cpp_api/s_mainmenu.cpp
	script/cpp_api/s_mainmenu.h
	script/lua_api/l_client.cpp
	script/lua_api/l_client.h
	script/lua_api/l_mainmenu.cpp
	script/lua_api/l_mainmenu.h
	script/lua_api/l_minimap.cpp
	script/lua_api/l_minimap.h
	script/lua_api/l_storage.cpp
	script/lua_api/l_storage.h
	script/lua_api/l_sound.cpp
	script/lua_api/l_sound.h
	script/lua_api/l_localplayer.cpp
	script/lua_api/l_localplayer.h
	script/lua_api/l_camera.cpp
	script/lua_api/l_camera.h
	script/scripting_mainmenu.cpp
	script/scripting_mainmenu.h
	script/scripting_client.cpp
	script/scripting_client.h
	unittest/test_keycode.cpp
	camera.cpp
	camera.h
	client.cpp
	client.h
	clientenvironment.cpp
	clientenvironment.h
	clientmap.cpp
	clientmap.h
	clientmedia.cpp
	clientmedia.h
	clientobject.cpp
	clientobject.h
	clouds.cpp
	clouds.h
	content_cao.cpp
	content_cao.h
	content_cso.cpp
	content_cso.h
	content_mapblock.cpp
	content_mapblock.h
	convert_json.cpp
	convert_json.h
	filecache.cpp
	filecache.h
	fontengine.cpp
	fontengine.h
	game.cpp
	game.h
	guiscalingfilter.cpp
	guiscalingfilter.h
	hud.cpp
	hud.h
	imagefilters.cpp
	imagefilters.h
	keycode.cpp
	keycode.h
	localplayer.cpp
	localplayer.h
	main.cpp
	mapblock_mesh.cpp
	mapblock_mesh.h
	mesh.cpp
	mesh.h
	mesh_generator_thread.cpp
	mesh_generator_thread.h
	minimap.cpp
	minimap.h
	particles.cpp
	particles.h
	shader.cpp
	shader.h
	sky.cpp
	sky.h
	wieldmesh.cpp
	wieldmesh.h
)
if(USE_SOUND)
	list(APPEND client_SRCS ${sound_SRCS})
endif()
if(USE_FREETYPE)
	list(APPEND client_SRCS ${client_irrlicht_changes_freetype_SRCS})
endif()
list(SORT client_SRCS)

set(server_SRCS
	${common_SRCS}
	main.cpp
)
list(SORT server_SRCS)

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

set_property(TARGET ${PROJECT_NAME} ${PROJECT_NAME}server APPEND_STRING PROPERTY COMPILE_DEFINITIONS
	/D USE_CMAKE_CONFIG_H
	# Surpress some useless warnings
	/D _CRT_SECURE_NO_DEPRECATE /W1
	# Get M_PI to work
	/D _USE_MATH_DEFINES
	# Dont define min/max macros in minwindef.h
	/D NOMINMAX
)
set_property(TARGET ${PROJECT_NAME} ${PROJECT_NAME}server APPEND_STRING PROPERTY COMPILE_OPTIONS
	/D WIN32_LEAN_AND_MEAN /MP
	$<$<CONFIG:Release>:   /EHa /Ox /GL /FD /MT /GS- /Zi /fp:fast /D NDEBUG /D _HAS_ITERATOR_DEBUGGING=0 /TP>
	$<$<CONFIG:SemiDebug>: /MDd /Zi /Ob0 /O1 /RTC1>
	$<$<CONFIG:Debug>:     /MDd /Zi /Ob0 /Od /RTC1>
	$<$<EQUAL:${CMAKE_SIZEOF_VOID_P},4>: /arch:SSE>
)
set_property(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY LINK_FLAGS_RELEASE
	/LTCG /INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF
	/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup
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
