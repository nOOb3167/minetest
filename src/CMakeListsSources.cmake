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
	script/lua_api/l_http.cpp
	script/lua_api/l_http.h
	script/lua_api/l_internal.h
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
	script/lua_api/l_settings.cpp
	script/lua_api/l_settings.h
	script/lua_api/l_storage.cpp
	script/lua_api/l_storage.h
	script/lua_api/l_util.cpp
	script/lua_api/l_util.h
	script/lua_api/l_vmanip.cpp
	script/lua_api/l_vmanip.h
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
	script/lua_api/l_camera.cpp
	script/lua_api/l_camera.h
	script/lua_api/l_client.cpp
	script/lua_api/l_client.h
	script/lua_api/l_localplayer.cpp
	script/lua_api/l_localplayer.h
	script/lua_api/l_mainmenu.cpp
	script/lua_api/l_mainmenu.h
	script/lua_api/l_minimap.cpp
	script/lua_api/l_minimap.h
	script/lua_api/l_storage.cpp
	script/lua_api/l_storage.h
	script/lua_api/l_sound.cpp
	script/lua_api/l_sound.h
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

set(server_SRCS
	${common_SRCS}
	main.cpp
)
