add_executable(babel-dump EXCLUDE_FROM_ALL babel-dump.c)
target_link_libraries(babel-dump babelhelper)

add_executable(babel-monitor EXCLUDE_FROM_ALL babel-monitor.c)
target_link_libraries(babel-monitor babelhelper)
