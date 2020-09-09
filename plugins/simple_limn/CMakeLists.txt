cmake_minimum_required(VERSION 3.1)

# Load in FIPS
get_filename_component(FIPS_ROOT_DIR "../fips" ABSOLUTE)
include("${FIPS_ROOT_DIR}/cmake/fips.cmake")

project(LIMN1K)
fips_setup()

fips_begin_sharedlib(limn1k)
fips_files(main.c limn.c rom.c platboard.c)
fips_end_sharedlib()

if(MSVC)
  target_compile_options(limn1k PRIVATE /W4 /WX)
else()
  target_compile_options(limn1k PRIVATE -Wall -Wextra -Werror -pedantic)
endif()

set_target_properties(limn1k PROPERTIES PREFIX "")
