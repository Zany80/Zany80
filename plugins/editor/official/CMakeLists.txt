set(official_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

add_library(official MODULE ${official_SOURCES})
set_target_properties(official PROPERTIES PREFIX "../")
target_include_directories(official PRIVATE ../../include ${CMAKE_SOURCE_DIR}/include ./include)

if (WIN32)
	install(TARGETS official DESTINATION plugins/editor)
else()
	install(TARGETS official DESTINATION share/zany80/plugins/editor)
endif()
