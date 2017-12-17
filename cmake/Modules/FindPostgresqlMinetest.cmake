## the PostgreSQL_ADDITIONAL_SEARCH_PATHS is not there in recent FindPostgreSQL.cmake (shipped with CMake)
##   it is called PostgreSQL_LIBRARY_ADDITIONAL_SEARCH_SUFFIXES now
##   therefore it may be safe to assume this mechanism has not worked for a while already

#find_program(POSTGRESQL_CONFIG_EXECUTABLE pg_config DOC "pg_config")
#find_library(POSTGRESQL_LIBRARY pq)
#if(POSTGRESQL_CONFIG_EXECUTABLE)
#	execute_process(COMMAND ${POSTGRESQL_CONFIG_EXECUTABLE} --includedir-server
#		OUTPUT_VARIABLE POSTGRESQL_SERVER_INCLUDE_DIRS
#		OUTPUT_STRIP_TRAILING_WHITESPACE)
#	execute_process(COMMAND ${POSTGRESQL_CONFIG_EXECUTABLE}
#		OUTPUT_VARIABLE POSTGRESQL_CLIENT_INCLUDE_DIRS
#		OUTPUT_STRIP_TRAILING_WHITESPACE)
#	# This variable is case sensitive for the cmake PostgreSQL module
#	set(PostgreSQL_ADDITIONAL_SEARCH_PATHS ${POSTGRESQL_SERVER_INCLUDE_DIRS} ${POSTGRESQL_CLIENT_INCLUDE_DIRS})
#endif()
#find_package(PostgreSQL)

message(FATAL_ERROR Not Applicable)
