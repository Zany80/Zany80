cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 11)

# Enable use of VERSION in project() command
cmake_policy(SET CMP0048 NEW)

# Enable debug symbols by default
# must be done before project() statement
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

# Default to static libs
if(NOT DEFINED SFML_STATIC_LIBRARIES)
	set(SFML_STATIC_LIBRARIES FALSE CACHE BOOL "Use static SFML libs" FORCE)
endif()

if(MINGW)
  ENABLE_LANGUAGE(RC)
  set(CMAKE_RC_COMPILE_OBJECT
    "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
  set(ICON_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/misc/icon.rc CACHE FILEPATH "Icon file" FORCE) 
endif(MINGW)

project(ZANY80 VERSION 0.1.0.0 LANGUAGES C CXX)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules" ${CMAKE_MODULE_PATH})

include(git_revision)

configure_file(
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h"
  )

find_package(LIBLIB 0.1 REQUIRED)

find_package(SFML 2 REQUIRED graphics window system )
if(SFML_NOTFOUND)
	message(FATAL_ERROR "SFML not found!")
endif()

add_subdirectory(src)
add_library(zany SHARED ${SOURCES})
add_executable(zany80 ${CMAKE_SOURCE_DIR}/src/main.cpp)

target_include_directories(zany80 PRIVATE ${SFML_INCLUDE_DIR} ${LIBLIB_INCLUDE_DIR})
target_include_directories(zany PRIVATE ${SFML_INCLUDE_DIR} ${LIBLIB_INCLUDE_DIR})
target_include_directories(zany80 PRIVATE "${ZANY80_BINARY_DIR}" PUBLIC "${ZANY80_SOURCE_DIR}/include")
target_include_directories(zany PRIVATE "${ZANY80_BINARY_DIR}" PUBLIC "${ZANY80_SOURCE_DIR}/include")
target_link_libraries(zany80 ${LIBLIB_LIBRARY} ${SFML_LIBRARIES} ${SFML_DEPENDENCIES} zany)
target_link_libraries(zany ${LIBLIB_LIBRARY} ${SFML_LIBRARIES} ${SFML_DEPENDENCIES})

add_subdirectory(plugins)

add_executable(BIOSMerge ${CMAKE_SOURCE_DIR}/tools/BIOSMerge.cpp)

add_custom_target(ZanyBIOS COMMAND make WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/BIOS)
add_dependencies(zany80 ZanyBIOS)

add_custom_command(
	TARGET ZanyBIOS
	POST_BUILD
	COMMAND BIOSMerge
	ARGS
		${CMAKE_BINARY_DIR}/BIOS.rom
		${CMAKE_SOURCE_DIR}/BIOS/bin/bank1.rom
		${CMAKE_SOURCE_DIR}/BIOS/bin/bank2.rom
		${CMAKE_SOURCE_DIR}/BIOS/bin/bank3.rom
	DEPENDS BIOSMerge
	VERBATIM
)

if(WIN32)
	if (NOT DEFINED LIBGCC_LIBRARY)
		set(LIBGCC_LIBRARY "" CACHE FILEPATH "libgcc dll - needed for windows cross-compilation" FORCE)
	endif()
	if (NOT DEFINED LIBSTDCPLUSPLUS_LIBRARY)
		set(LIBSTDCPLUSPLUS_LIBRARY "" CACHE FILEPATH "libstdc++ dll - needed for windows cross-compilation" FORCE)
	endif()
	install(FILES ${LIBLIB_LIBRARY} font.png ${LIBGCC_LIBRARY} ${LIBSTDCPLUSPLUS_LIBRARY} DESTINATION .)
	install (TARGETS zany80 zanylib
		RUNTIME DESTINATION .
		LIBRARY DESTINATION .
	)
	file(GLOB BINARIES misc/win32/*)
	install(PROGRAMS ${BINARIES} DESTINATION plugins/binaries/)
	install(FILES ${CMAKE_BINARY_DIR}/BIOS.rom DESTINATION .)
else()
	install(
		FILES font.png DESTINATION share/zany80
	)
	install (FILES misc/zany80.desktop DESTINATION share/applications)
	install (FILES misc/zany80.png DESTINATION share/pixmaps)
	install (TARGETS zany80 zany
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib
	)
	file(GLOB BINARIES misc/linux/*)
	install(PROGRAMS ${BINARIES} DESTINATION share/zany80/plugins/binaries/)
	install(FILES ${CMAKE_BINARY_DIR}/BIOS.rom DESTINATION share/zany80/)
endif()

include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${ZANY80_SOURCE_DIR}/COPYING")
set(CPACK_PACKAGE_VERSION_MAJOR ${ZANY80_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${ZANY80_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${ZANY80_VERSION_PATCH})
set(CPACK_PACKAGE_EXECUTABLES
	zany80 "Zany80"
)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "pixelherodev")
include(CPack)
