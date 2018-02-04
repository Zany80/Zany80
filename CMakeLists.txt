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

add_executable(zany80 ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

add_subdirectory(plugins)

set_target_properties(zany80 PROPERTIES LINK_FLAGS "-Wl,--dynamic-list=${ZANY80_SOURCE_DIR}/symbols")
#set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

include_directories(${SFML_INCLUDE_DIR} ${LIBLIB_INCLUDE_DIR})
target_include_directories(zany80 PRIVATE "${ZANY80_BINARY_DIR}" PUBLIC "${ZANY80_SOURCE_DIR}/include")
target_include_directories(zany PRIVATE "${ZANY80_BINARY_DIR}" PUBLIC "${ZANY80_SOURCE_DIR}/include")
target_link_libraries(zany80 ${LIBLIB_LIBRARY} ${SFML_LIBRARIES} ${SFML_DEPENDENCIES} zany)
target_link_libraries(zany ${LIBLIB_LIBRARY} ${SFML_LIBRARIES} ${SFML_DEPENDENCIES})

if(WIN32)
  install(FILES font.png DESTINATION .)
  install (TARGETS zany80 DESTINATION .)
else()
  install(FILES font.png DESTINATION share/zany80)
  install (TARGETS zany80 DESTINATION bin)
endif()

include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${ZANY80_SOURCE_DIR}/COPYING")
set(CPACK_PACKAGE_VERSION_MAJOR ${ZANY80_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${ZANY80_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${ZANY80_VERSION_PATCH})
#set(CPACK_PACKAGE_EXECUTABLES ${executable_name} "icon")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "pixelherodev")
include(CPack)
