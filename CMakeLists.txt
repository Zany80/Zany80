cmake_minimum_required(VERSION 3.1)

set(FIPS_EMSCRIPTEN_USE_CWRAP ON CACHE BOOL "" FORCE)

# Load in FIPS
get_filename_component(FIPS_ROOT_DIR "../fips" ABSOLUTE)
include("${FIPS_ROOT_DIR}/cmake/fips.cmake")

macro(configure_msvc_runtime)
  if(MSVC)

    # Default to statically-linked runtime.
    if("${MSVC_RUNTIME}" STREQUAL "")
      set(MSVC_RUNTIME "dynamic")
    endif()

    # Set compiler options.
    set(variables
      CMAKE_C_FLAGS_DEBUG
      CMAKE_C_FLAGS_MINSIZEREL
      CMAKE_C_FLAGS_RELEASE
      CMAKE_C_FLAGS_RELWITHDEBINFO
      CMAKE_CXX_FLAGS_DEBUG
      CMAKE_CXX_FLAGS_MINSIZEREL
      CMAKE_CXX_FLAGS_RELEASE
      CMAKE_CXX_FLAGS_RELWITHDEBINFO
    )
    if(${MSVC_RUNTIME} STREQUAL "static")
      message(STATUS
        "MSVC -> forcing use of statically-linked runtime."
      )
      foreach(variable ${variables})
        if(${variable} MATCHES "/MD")
          string(REGEX REPLACE "/MD" "/MT" ${variable} "${${variable}}")
        endif()
      endforeach()
    else()
      message(STATUS
        "MSVC -> forcing use of dynamically-linked runtime."
      )
      foreach(variable ${variables})
        if(${variable} MATCHES "/MT")
          string(REGEX REPLACE "/MT" "/MD" ${variable} "${${variable}}")
        endif()
      endforeach()
    endif()
  endif()
endmacro()

fips_setup(PROJECT Zany80)
# Needed to define a version
project(Zany80 VERSION 0.1.1.0)
set(VERSION_PREFIX "")
set(VERSION_SUFFIX "")

# TODO: get this working on native Windows compilation
# TODO: and also mingw cross-compilation
if(FIPS_WINDOWS)
  configure_msvc_runtime()
  ENABLE_LANGUAGE(RC)
  set(CMAKE_RC_COMPILE_OBJECT
    "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
  set(ICON_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/icon.rc)
elseif(FIPS_OSX)
	message(WARNING "OS X not currently supported")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -undefined dynamic_lookup")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -undefined dynamic_lookup")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -undefined dynamic_lookup")
endif()

fips_include_directories("${PROJECT_SOURCE_DIR}/headers" "${PROJECT_SOURCE_DIR}/headers/Zany80/3rd-party" "${PROJECT_BINARY_DIR}")

configure_file("${PROJECT_SOURCE_DIR}/src/config.h.in" "${PROJECT_BINARY_DIR}/config.h")
fips_begin_app(zany80 cmdline)
fips_deps(Gfx IMUI lib imguitexteditor)
if (FIPS_EMSCRIPTEN)
	fips_deps(HttpFS)
	#~ simple_shell)
	#~  z80cpp_core scas_plugin editor display
else()
	fips_deps(LocalFS)
endif()
fips_src(src/)
fips_dir(backend/)
if (FIPS_WINDOWS)
	fips_files(windows.c)
elseif(FIPS_POSIX)
	fips_files(posix.c)
endif()
if (ORYOL_OPENGL AND (FIPS_MACOS OR FIPS_WINDOWS OR FIPS_LINUX))
	fips_files(glfw.cc)
elseif(ORYOL_D3D11)
	fips_files(d3d11.cc)
elseif(FIPS_EMSCRIPTEN)
    fips_files(emsc.cc)
endif()
fips_end_app()
target_compile_definitions(zany80 PRIVATE ZANY_EXPORT=1)
set_property(TARGET zany80 PROPERTY ENABLE_EXPORTS TRUE)

fips_add_subdirectory(plugins)

fips_finish()

if(FIPS_WINDOWS)
	set(CPACK_GENERATOR "NSIS")
	install (TARGETS zany80
		RUNTIME DESTINATION .
		LIBRARY DESTINATION .
	)
	install (FILES misc/stdlib.o DESTINATION lib)
elseif(FIPS_LINUX)
	set(CPACK_GENERATOR "TGZ;DEB")
	install (FILES misc/zany80.desktop DESTINATION share/applications)
	install (FILES misc/zany80.png DESTINATION share/pixmaps)
	install (FILES misc/stdlib.o DESTINATION share/zany80/lib)
	install (TARGETS zany80
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib
	)
elseif (FIPS_OSX)
	set(CPACK_GENERATOR "Bundle")
	set(CPACK_BUNDLE_NAME "Zany80")
	set(CPACK_BUNDLE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/misc/zany80.png")
	set(CPACK_BUNDLE_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/misc/zany80.plist")
	install(TARGETS zany80 DESTINATION .)
endif()

install(DIRECTORY ${CMAKE_SOURCE_DIR}/headers/ DESTINATION include)

if (DEFINED EXTRA_INSTALL_DIR)
	install(DIRECTORY "${EXTRA_INSTALL_DIR}/" DESTINATION .)
endif()

include(InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The Zany80 Fantasy Computer")
set(CPACK_PACKAGE_VENDOR "Noam Preil")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
set(CPACK_PACKAGE_FILE_NAME "Zany80-${VERSION_PREFIX}${PROJECT_VERSION}${VERSION_SUFFIX}")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Zany80")
if (FIPS_WINDOWS)
	set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}\\\\misc\\\\zany80.png")
	set(CPACK_NSIS_INSTALLED_ICON_NAME "\\\\zany80.exe")
	set(CPACK_NSIS_DISPLAY_NAME "Zany80 Fantasy Computer")
	set(CPACK_NSIS_PACKAGE_NAME "Zany80 Fantasy Computer")
	set(CPACK_NSIS_HELP_LINK "http:\\\\\\\\zany80.github.io")
	set(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\zany80.github.io")
	set(CPACK_NSIS_CONTACT "pleasantatk@gmail.com")
	set(CPACK_NSIS_MODIFY_PATH "OFF")
	set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL "ON")
	set(CPACK_NSIS_EXECUTABLES_DIRECTORY "./")
	set(CPACK_NSIS_MUI_FINISHPAGE_RUN "\\\\zany80.exe")
	set(CPACK_NSIS_HELP_LINK "https://zany80.github.io")
	set(CPACK_NSIS_URL_INFO_ABOUT "https://zany80.github.io")
	set(CPACK_NSIS_CONTACT "Feel free to send me a message at pleasantatk@gmail.com for assistance!")
	set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}\\\\misc\\\\zany.ico")
	set(CPACK_PACKAGE_EXECUTABLES "Zany80;Zany80")
else()
	set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/misc/zany80.png")
endif()
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "pixelherodev")
include(CPack)
