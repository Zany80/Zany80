cmake_minimum_required(VERSION 2.8.5)

project(scas C)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -std=c99 -D_XOPEN_SOURCE=700")

if(WIN32)
    set(CMAKE_C_FLAGS "-Wl,--allow-multiple-definition")
endif()

if (NOT DEFINED CMAKE_BUILD_TYPE)                                                                              
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type (Debug or Release)" FORCE)                           
endif()

if(EMSCRIPTEN)
    set(CMAKE_EXE_LINKER_FLAGS "-O2")
endif()

if (NOT DEFINED SCAS_DOCUMENTATION)
	set(SCAS_DOCUMENTATION FALSE CACHE BOOL "Whether to build the documentation")
endif()

if (NOT DEFINED SCAS_LIBRARY)
	set(SCAS_LIBRARY FALSE CACHE BOOL "Whether to build libscas")
endif()

add_executable(generate_tables ${PROJECT_SOURCE_DIR}/tables/generate.c)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tables/z80.c ${CMAKE_BINARY_DIR}/tables/z80.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tables/z80.tab generate_tables
    COMMAND generate_tables ${CMAKE_CURRENT_SOURCE_DIR}/tables/z80.tab ${CMAKE_CURRENT_BINARY_DIR}/tables/z80.c ${CMAKE_BINARY_DIR}/tables/generated.h)

FILE(GLOB common ${PROJECT_SOURCE_DIR}/common/*.c)
FILE(GLOB assembler ${PROJECT_SOURCE_DIR}/assembler/*.c)
FILE(GLOB linker ${PROJECT_SOURCE_DIR}/linker/*.c)
FILE(GLOB scas ${PROJECT_SOURCE_DIR}/scas/*.c)
FILE(GLOB scdump ${PROJECT_SOURCE_DIR}/scdump/*.c)
FILE(GLOB scwrap ${PROJECT_SOURCE_DIR}/scwrap/*.c)

add_executable(scas
    ${common}
    ${assembler}
    ${linker}
    ${scas}
    ${PROJECT_BINARY_DIR}/tables/z80.c
)

include_directories(scas
    ${CMAKE_SOURCE_DIR}/include/
    ${CMAKE_SOURCE_DIR}/scas/
    ${CMAKE_BINARY_DIR}/tables/
)

if (SCAS_LIBRARY)
    add_library(libscas SHARED
        ${common}
        ${assembler}
        ${linker}
        ${PROJECT_BINARY_DIR}/tables/z80.c
        ${PROJECT_SOURCE_DIR}/runtime.c)

    SET_TARGET_PROPERTIES(libscas
      PROPERTIES OUTPUT_NAME scas)
    INSTALL(
        TARGETS libscas
        LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    )
endif()

add_executable(scdump
    ${common}
    ${scdump}
)

add_executable(scwrap
    ${common}
    ${scwrap}
)

if(SCAS_DOCUMENTATION)

	ADD_CUSTOM_TARGET(
		man ALL
		DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/bin/scas.1
		DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/bin/scdump.1
		DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/bin/scwrap.1
	)

	ADD_CUSTOM_COMMAND(
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/scas.1.txt
		COMMAND a2x --no-xmllint --doctype manpage --format manpage ${CMAKE_CURRENT_SOURCE_DIR}/scas.1.txt
		COMMAND test "${CMAKE_CURRENT_SOURCE_DIR}" != "${CMAKE_CURRENT_BINARY_DIR}/bin" && mv -f ${CMAKE_CURRENT_SOURCE_DIR}/scas.1 ${CMAKE_CURRENT_BINARY_DIR}/bin || true
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bin/scas.1
		VERBATIM
	)
	set_directory_properties(${CMAKE_CURRENT_BINARY_DIR} ADDITIONAL_MAKE_CLEAN_FILES scas.1)
	ADD_CUSTOM_COMMAND(
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/scdump.1.txt
		COMMAND a2x --no-xmllint --doctype manpage --format manpage ${CMAKE_CURRENT_SOURCE_DIR}/scdump.1.txt
		COMMAND test "${CMAKE_CURRENT_SOURCE_DIR}" != "${CMAKE_CURRENT_BINARY_DIR}/bin" && mv -f ${CMAKE_CURRENT_SOURCE_DIR}/scdump.1 ${CMAKE_CURRENT_BINARY_DIR}/bin || true
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bin/scdump.1
		VERBATIM
	)
	set_directory_properties(${CMAKE_CURRENT_BINARY_DIR} ADDITIONAL_MAKE_CLEAN_FILES scdump.1)
	ADD_CUSTOM_COMMAND(
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/scwrap.1.txt
		COMMAND a2x --no-xmllint --doctype manpage --format manpage ${CMAKE_CURRENT_SOURCE_DIR}/scwrap.1.txt
		COMMAND test "${CMAKE_CURRENT_SOURCE_DIR}" != "${CMAKE_CURRENT_BINARY_DIR}/bin" && mv -f ${CMAKE_CURRENT_SOURCE_DIR}/scwrap.1 ${CMAKE_CURRENT_BINARY_DIR}/bin || true
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bin/scwrap.1
		VERBATIM
	)
	set_directory_properties(${CMAKE_CURRENT_BINARY_DIR} ADDITIONAL_MAKE_CLEAN_FILES scwrap.1)

	INSTALL(
		FILES ${CMAKE_CURRENT_BINARY_DIR}/bin/scas.1
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man1
	)

	INSTALL(
		FILES ${CMAKE_CURRENT_BINARY_DIR}/bin/scdump.1
		DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man1
	)

	INSTALL(
		FILES ${CMAKE_CURRENT_BINARY_DIR}/bin/scwrap.1
		DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man1
	)

endif()

INSTALL(
    TARGETS scas scdump scwrap
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION include/scas
)
