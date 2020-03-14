include_directories(${LIBBABELHELPER_SOURCE_DIR}/include)

set(LIBBABELHELPER_ABI 0)

add_library(babelhelper babelhelper.c)
set_target_properties(babelhelper PROPERTIES
  SOVERSION ${LIBBABELHELPER_ABI}
  VERSION ${LIBBABELHELPER_ABI}.${PROJECT_VERSION}
)

install(TARGETS babelhelper
  ARCHIVE DESTINATION ${LIBDIR}
  LIBRARY DESTINATION ${LIBDIR}
  RUNTIME DESTINATION bin
)

