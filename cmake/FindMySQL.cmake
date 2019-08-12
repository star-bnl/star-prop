# - Find mysqlclient
# Find the native MySQL includes and library
#
#  MYSQL_INCLUDE_DIRS - where to find mysql.h, etc.
#  MYSQL_LIBRARIES   - mysqlclient library.
#  MYSQL_FOUND       - True if mysqlclient is found.
#

find_path( MYSQL_INCLUDE_DIRS "mysql.h"
	PATH_SUFFIXES
		"mysql"
	PATHS
		"/usr/include"
		"/usr/local/include"
		"/usr/mysql/include"
		"/opt/local/include"
		"$ENV{PROGRAMFILES}/MySQL/*/include"
		"$ENV{SYSTEMDRIVE}/MySQL/*/include" )

find_library( MYSQL_LIBRARIES
	NAMES "mysqlclient" "mysqlclient_r"
	PATH_SUFFIXES
		"mysql"
	PATHS
		"/usr/local/lib"
		"/usr/mysql/lib"
		"/opt/local/lib"
		"$ENV{PROGRAMFILES}/MySQL/*/lib"
		"$ENV{SYSTEMDRIVE}/MySQL/*/lib" )
mark_as_advanced( MYSQL_LIBRARIES  MYSQL_INCLUDE_DIRS )

if( MYSQL_INCLUDE_DIRS AND EXISTS "${MYSQL_INCLUDE_DIRS}/mysql_version.h" )
	file( STRINGS "${MYSQL_INCLUDE_DIRS}/mysql_version.h" MYSQL_VERSION_H REGEX "^#define[ \t]+MYSQL_SERVER_VERSION[ \t]+\"[^\"]+\".*$" )
	string( REGEX REPLACE "^.*MYSQL_SERVER_VERSION[ \t]+\"([^\"]+)\".*$" "\\1" MYSQL_VERSION_STRING "${MYSQL_VERSION_H}" )
endif()

# handle the QUIETLY and REQUIRED arguments and set MYSQL_FOUND to TRUE if 
# all listed variables are TRUE
include( FindPackageHandleStandardArgs )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( MYSQL
	REQUIRED_VARS MYSQL_LIBRARIES MYSQL_INCLUDE_DIRS
	VERSION_VAR MYSQL_VERSION_STRING )
