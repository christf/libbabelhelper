cmake_minimum_required(VERSION 2.8.8)

project(LIBBABELHELPER C)
set(PROJECT_VERSION 1)

set(LIBDIR "lib${LIB_SUFFIX}")

option(BUILD_SHARED_LIBS "Build shared library")
add_compile_options("-Wall")

add_subdirectory(src)

configure_file(${LIBBABELHELPER_SOURCE_DIR}/libbabelhelper.pc.in ${LIBBABELHELPER_BINARY_DIR}/libbabelhelper.pc @ONLY)
install(FILES ${LIBBABELHELPER_BINARY_DIR}/libbabelhelper.pc DESTINATION "${LIBDIR}/pkgconfig")

FILE(GLOB include_files "${CMAKE_CURRENT_SOURCE_DIR}/include/libbabelhelper/*.h")
install(FILES ${include_files} DESTINATION "include/libbabelhelper-${PROJECT_VERSION}/libbabelhelper")

add_custom_target(format
        find src include -type f -name '*.[ch]' | xargs clang-format -i --style=file
        WORKING_DIRECTORY ${LIBBABELHELPER_SOURCE_DIR}
)
