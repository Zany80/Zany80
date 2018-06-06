add_subdirectory(tools)

set(SOURCES
	${SOURCES}
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Drawing.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/LuaPlugin.cpp
	PARENT_SCOPE
)

add_subdirectory(global_rt)
