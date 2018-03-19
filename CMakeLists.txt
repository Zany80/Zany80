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

project(Zany80 VERSION 0.1.0.0 LANGUAGES C CXX)

if(WIN32)
  ENABLE_LANGUAGE(RC)
  set(CMAKE_RC_COMPILE_OBJECT
    "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
  set(ICON_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/misc/icon.rc) 
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake_modules")

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
add_executable(zany80 ${CMAKE_SOURCE_DIR}/src/main.cpp ${ICON_SOURCE})

target_include_directories(zany80 PRIVATE ${SFML_INCLUDE_DIR} ${LIBLIB_INCLUDE_DIR})
target_include_directories(zany PRIVATE ${SFML_INCLUDE_DIR} ${LIBLIB_INCLUDE_DIR})
target_include_directories(zany80 PRIVATE "${Zany80_BINARY_DIR}" PUBLIC "${Zany80_SOURCE_DIR}/include")
target_include_directories(zany PRIVATE "${Zany80_BINARY_DIR}" PUBLIC "${Zany80_SOURCE_DIR}/include")
target_link_libraries(zany80 ${LIBLIB_LIBRARY} ${SFML_LIBRARIES} ${SFML_DEPENDENCIES} zany)
target_link_libraries(zany ${LIBLIB_LIBRARY} ${SFML_LIBRARIES} ${SFML_DEPENDENCIES})
link_libraries(zany)

add_subdirectory(plugins)

add_executable(BIOSMerge ${CMAKE_SOURCE_DIR}/tools/BIOSMerge.cpp)

add_custom_target(ZanyBIOS COMMAND make WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/BIOS DEPENDS BIOSMerge)
add_dependencies(zany80 ZanyBIOS)

add_custom_command(
	TARGET ZanyBIOS
	POST_BUILD
	COMMAND ${CMAKE_BINARY_DIR}/BIOSMerge
	ARGS
		${CMAKE_BINARY_DIR}/BIOS.rom
		${CMAKE_SOURCE_DIR}/BIOS/bin/bank1.rom
		${CMAKE_SOURCE_DIR}/BIOS/bin/bank2.rom
		${CMAKE_SOURCE_DIR}/BIOS/bin/bank3.rom
	DEPENDS BIOSMerge
	VERBATIM
)

if(WIN32)
	set(CPACK_GENERATOR "NSIS")
	if (NOT DEFINED LIBGCC_LIBRARY)
		set(LIBGCC_LIBRARY "" CACHE FILEPATH "libgcc dll - needed for windows cross-compilation" FORCE)
	endif()
	if (NOT DEFINED LIBSTDCPLUSPLUS_LIBRARY)
		set(LIBSTDCPLUSPLUS_LIBRARY "" CACHE FILEPATH "libstdc++ dll - needed for windows cross-compilation" FORCE)
	endif()
	install(FILES ${LIBLIB_LIBRARY} font.png ${LIBGCC_LIBRARY} ${LIBSTDCPLUSPLUS_LIBRARY} DESTINATION .)
	install (TARGETS zany80 zany
		RUNTIME DESTINATION .
		LIBRARY DESTINATION .
	)
	file(GLOB BINARIES misc/win32/*)
	install(PROGRAMS ${BINARIES} DESTINATION plugins/binaries/)
	install(FILES ${CMAKE_BINARY_DIR}/BIOS.rom DESTINATION .)
else()
	set(CPACK_GENERATOR "TGZ;DEB")
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

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The Zany80 Fantasy Computer")
SET(CPACK_PACKAGE_VENDOR "Sir That Guy")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
set(CPACK_PACKAGE_VERSION_MAJOR ${Zany80_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${Zany80_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${Zany80_VERSION_PATCH})
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "Zany80")
IF(WIN32 AND NOT UNIX)
  # There is a bug in NSIS that does not handle full unix paths properly. Make
  # sure there is at least one set of four (4) backlasshes.
  SET(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}\\\\misc/zany80.bmp")
  SET(CPACK_NSIS_INSTALLED_ICON_NAME "zany80.exe")
  SET(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY} Fantasy Computer")
  SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\zany80.github.io")
  SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\zany80.github.io")
  SET(CPACK_NSIS_CONTACT "pleasantatk@gmail.com")
  SET(CPACK_NSIS_MODIFY_PATH ON)
ENDIF(WIN32 AND NOT UNIX)
set(CPACK_PACKAGE_EXECUTABLES "zany80" "Zany80")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "pixelherodev")
include(CPack)
