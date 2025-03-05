
execute_process(COMMAND git rev-parse --short HEAD
	WORKING_DIRECTORY "${GENERATE_VERSION_SOURCE_DIR}"
	OUTPUT_VARIABLE VERSION_GITHASH OUTPUT_STRIP_TRAILING_WHITESPACE
	ERROR_QUIET)
if(NOT VERSION_GITHASH)
	set(VERSION_GITHASH "(none)")
else()
	message(STATUS "*** Detected Git commit ${VERSION_GITHASH} ***")
endif()

configure_file(
	${GENERATE_VERSION_SOURCE_DIR}/src/version_info_git.hh.in
	${GENERATE_VERSION_BINARY_DIR}/version_info_git.hh)
