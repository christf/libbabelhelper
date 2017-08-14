include_directories(${LIBBABELHELPER_SOURCE_DIR}/include)

set(LIBBABELHELPER_SRC babelhelper.c)
set(LIBBABELHELPER_ABI 0)

add_library(libbabelhelper_shared SHARED ${LIBBABELHELPER_SRC})
set_target_properties(libbabelhelper_shared PROPERTIES
  OUTPUT_NAME babelhelper
  SOVERSION ${LIBBABELHELPER_ABI}
  VERSION ${LIBBABELHELPER_ABI}.${PROJECT_VERSION}
  COMPILE_FLAGS -Wall
)

add_library(libbabelhelper_static STATIC ${LIBBABELHELPER_SRC})
set_target_properties(libbabelhelper_static PROPERTIES
  OUTPUT_NAME babelhelper
  COMPILE_FLAGS -Wall
)

install(TARGETS libbabelhelper_shared libbabelhelper_static
  ARCHIVE DESTINATION ${LIBDIR}
  LIBRARY DESTINATION ${LIBDIR}
  RUNTIME DESTINATION bin
)

