add_subdirectory(tools)

set(SOURCES
	${SOURCES}
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Drawing.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/LuaPlugin.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/LuaAPI.cpp
	PARENT_SCOPE
)

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/default_config.lua"
	"${PROJECT_BINARY_DIR}/default_config.lua"
	COPYONLY
)
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/init_plugins.lua"
	"${PROJECT_BINARY_DIR}/init_plugins.lua"
	COPYONLY
)

install(
	FILES 
		"${CMAKE_CURRENT_SOURCE_DIR}/default_config.lua" 
		"${CMAKE_CURRENT_SOURCE_DIR}/init_plugins.lua"
	DESTINATION .
)

add_subdirectory(global_rt)
