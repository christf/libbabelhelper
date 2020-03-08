cmake_minimum_required(VERSION 2.8.8)

# list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

project(LIBBABELHELPER C)
set(libbabelhelper_VERSION "1")
set(PROJECT_VERSION "1")


set(CMAKE_MODULE_PATH ${LIBUECC_SOURCE_DIR})

set(DOXYFILE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

include(UseDoxygen OPTIONAL)

set(LIBDIR "lib${LIB_SUFFIX}")

add_subdirectory(src)

configure_file(${LIBBABELHELPER_SOURCE_DIR}/libbabelhelper.pc.in ${LIBBABELHELPER_BINARY_DIR}/libbabelhelper.pc @ONLY)
install(FILES ${LIBBABELHELPER_BINARY_DIR}/libbabelhelper.pc DESTINATION "${LIBDIR}/pkgconfig")

FILE(GLOB include_files "${CMAKE_CURRENT_SOURCE_DIR}/include/libbabelhelper/*.h")
install(FILES ${include_files} DESTINATION "include/libbabelhelper-${PROJECT_VERSION}/libbabelhelper")

add_custom_target(format
        find src include -type f -name '*.[ch]' | xargs clang-format -i --style=file
        WORKING_DIRECTORY ${LIBBABELHELPER_SOURCE_DIR}
)
