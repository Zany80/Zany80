set(CartridgeManager_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/CartridgeManager.cpp
)

add_library(CartridgeManager MODULE ${CartridgeManager_SOURCES})
set_target_properties(CartridgeManager PROPERTIES PREFIX "../")
target_include_directories(CartridgeManager PRIVATE ../include ${CMAKE_SOURCE_DIR}/include)

if (WIN32)
	install(TARGETS CartridgeManager DESTINATION plugins/)
else()
	install(TARGETS CartridgeManager DESTINATION share/zany80/plugins/)
endif()
