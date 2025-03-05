#include "version_info.hh"
#include "version_info_git.hh"

int principia_version_code() {
	return PRINCIPIA_VERSION_CODE;
}

const char* principia_version_string() {
	return PRINCIPIA_VERSION_STRING;
}

const char* principia_version_hash() {
	return PRINCIPIA_VERSION_HASH;
}
