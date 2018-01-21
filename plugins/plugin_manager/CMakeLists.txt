set(plugin_manager_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/plugin_manager.cpp
)

add_library(plugin_manager SHARED ${plugin_manager_SOURCES})
set_target_properties(plugin_manager PROPERTIES PREFIX "../")
target_include_directories(plugin_manager PRIVATE includes)
if (WIN32)
	install(TARGETS plugin_manager DESTINATION plugins)
else()
	install(TARGETS plugin_manager DESTINATION share/zany80/plugins)
endif()
