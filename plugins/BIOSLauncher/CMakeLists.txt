set(BIOSLauncher_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/BIOSLauncher.cpp
)

add_library(BIOSLauncher MODULE ${BIOSLauncher_SOURCES})
set_target_properties(BIOSLauncher PROPERTIES PREFIX "../")
target_include_directories(BIOSLauncher PRIVATE ../include ${CMAKE_SOURCE_DIR}/include)

if (WIN32)
	install(TARGETS BIOSLauncher DESTINATION plugins/)
else()
	install(TARGETS BIOSLauncher DESTINATION share/zany80/plugins/)
endif()
