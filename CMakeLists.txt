# Copyright (c) 2006, 2023, Oracle and/or its affiliates.
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is also distributed with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms,
# as designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have included with MySQL.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

IF(WIN32)
  # CMake 2.8.12 is needed for Visual Studio 2013
  # CMake 3.1.0 is needed for $<TARGET_PDB_FILE:${target}>
  CMAKE_MINIMUM_REQUIRED(VERSION 3.1.0)
ELSE()
  # There's a bug in unzipping prior to 2.8.2
  # Debian 7 has the oldest cmake we need to support
  CMAKE_MINIMUM_REQUIRED(VERSION 2.8.9)
ENDIF()

# We use CMAKE_SHARED_LIBRARY_<Lang>_FLAGS. See cmake --help-policy CMP0018
IF(CMAKE_VERSION VERSION_GREATER "2.8.8")
  CMAKE_POLICY(SET CMP0018 OLD)
ENDIF()

# We use PROPERTIES LINK_INTERFACE_LIBRARIES. See cmake --help-policy CMP0022
IF(CMAKE_VERSION VERSION_EQUAL "2.8.12" OR
   CMAKE_VERSION VERSION_GREATER "2.8.12")
 CMAKE_POLICY(SET CMP0022 OLD)
ENDIF()

# We use get_target_property() for non-existent targets (CMP0045)
# and INSTALL_NAME_DIR (CMP0042)
IF(CMAKE_VERSION VERSION_EQUAL "3.0.0" OR
   CMAKE_VERSION VERSION_GREATER "3.0.0")
 CMAKE_POLICY(SET CMP0045 OLD)
 CMAKE_POLICY(SET CMP0042 OLD)
ENDIF()

# In CMake 3.12 and above, the
#
# * ``check_include_file`` macro in the ``CheckIncludeFile`` module, the
# * ``check_include_file_cxx`` macro in the
#   ``CheckIncludeFileCXX`` module, and the
# * ``check_include_files`` macro in the ``CheckIncludeFiles`` module
#
# now prefer to link the check executable to the libraries listed in the
# ``CMAKE_REQUIRED_LIBRARIES`` variable.
IF(POLICY CMP0075)
  CMAKE_POLICY(SET CMP0075 OLD)
ENDIF()

MESSAGE(STATUS "Running cmake version ${CMAKE_VERSION}")

# Will set GIT_EXECUTABLE and GIT_FOUND
FIND_PACKAGE(Git)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

# First, decide about build type (debug or release)
# If cmake is invoked with -DCMAKE_BUILD_TYPE, 
# respect user wishes and do not (re)define CMAKE_BUILD_TYPE. If WITH_DEBUG
# is given, set CMAKE_BUILD_TYPE = Debug. Otherwise, use Relwithdebinfo.

IF(DEFINED CMAKE_BUILD_TYPE)
  SET(HAVE_CMAKE_BUILD_TYPE TRUE)
ENDIF()

OPTION(WITH_DEBUG "Use dbug/safemutex" OFF)

# Distinguish between community and non-community builds, with the
# default being a community build. This does not impact the feature
# set that will be compiled in; it's merely provided as a hint to
# custom packaging steps.
OPTION(COMMUNITY_BUILD "Set to true if this is a community build" ON) 

# Use a default manufacturer if no manufacturer was identified.
SET(MANUFACTURER_DOCSTRING
  "Set the entity that appears as the manufacturer of packages that support a manufacturer field.")
IF(NOT DEFINED MANUFACTURER) 
  SET(MANUFACTURER "Built from Source" CACHE  STRING ${MANUFACTURER_DOCSTRING})
  MARK_AS_ADVANCED(MANUFACTURER)
ENDIF()


# MAX_INDEXES - Set the maximum number of indexes per table, default 64U
IF (NOT MAX_INDEXES)
  SET(MAX_INDEXES 64U)
ENDIF(NOT MAX_INDEXES)

IF (${MAX_INDEXES} GREATER 255)
  MESSAGE(FATAL_ERROR "MAX_INDEXES values greater than 255 is not supported!")
ENDIF()
MESSAGE(STATUS "Configuring with MAX_INDEXES = ${MAX_INDEXES}")

# We choose to provide WITH_DEBUG as alias to standard CMAKE_BUILD_TYPE=Debug
# which turns out to be not trivial, as this involves synchronization 
# between CMAKE_BUILD_TYPE and WITH_DEBUG. Besides, we have to deal with cases
# where WITH_DEBUG is  reset from ON to OFF  and here we need to reset 
# CMAKE_BUILD_TYPE to either none or default RelWithDebInfo

SET(BUILDTYPE_DOCSTRING
 "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or
 CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel")

IF(WITH_DEBUG)
  SET(CMAKE_BUILD_TYPE "Debug" CACHE STRING ${BUILDTYPE_DOCSTRING} FORCE)
  IF(UNIX AND NOT APPLE)
    # Compiling with PIC speeds up embedded build, on PIC sensitive systems 
    # Predefine it to ON, in case user chooses to build embedded. 
    SET(WITH_PIC ON CACHE BOOL "Compile with PIC")
  ENDIF()
  SET(OLD_WITH_DEBUG 1 CACHE INTERNAL "" FORCE)
ELSEIF(NOT HAVE_CMAKE_BUILD_TYPE OR OLD_WITH_DEBUG)
  IF(CMAKE_BUILD_TYPE MATCHES "Debug" OR NOT HAVE_CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING 
       ${BUILDTYPE_DOCSTRING} FORCE)
  ENDIF()
  SET(OLD_WITH_DEBUG 0 CACHE INTERNAL "" FORCE)
ENDIF()

# Optionally set project name, e.g.
# foo.xcodeproj (mac) or foo.sln (windows)
SET(MYSQL_PROJECT_NAME_DOCSTRING "MySQL project name")
IF(DEFINED MYSQL_PROJECT_NAME)
  SET(MYSQL_PROJECT_NAME ${MYSQL_PROJECT_NAME} CACHE STRING
      ${MYSQL_PROJECT_NAME_DOCSTRING} FORCE)
ELSE()
  SET(MYSQL_PROJECT_NAME "MySQL" CACHE STRING
      ${MYSQL_PROJECT_NAME_DOCSTRING} FORCE)
  MARK_AS_ADVANCED(MYSQL_PROJECT_NAME)
ENDIF()

PROJECT(${MYSQL_PROJECT_NAME})

MACRO(REPORT_CXX_FLAGS)
  MESSAGE(STATUS "CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")
  FOREACH(BUILD_TYPE "" _DEBUG _RELWITHDEBINFO _RELEASE _MINSIZEREL)
    SET(flag "CMAKE_CXX_FLAGS${BUILD_TYPE}")
    MESSAGE(STATUS "${flag}: ${${flag}}")
  ENDFOREACH()
ENDMACRO()

# STRING(APPEND ...) from cmake VERSION 3.4
MACRO(STRING_APPEND STRING_VAR INPUT)
  SET(${STRING_VAR} "${${STRING_VAR}}${INPUT}")
ENDMACRO()

MACRO(STRING_PREPEND STRING_VAR INPUT)
  SET(${STRING_VAR} "${INPUT}${${STRING_VAR}}")
ENDMACRO()

SET(BUILD_IS_SINGLE_CONFIG TRUE)
MESSAGE(STATUS "CMAKE_GENERATOR: ${CMAKE_GENERATOR}")
IF(CMAKE_GENERATOR MATCHES "Visual Studio" OR CMAKE_GENERATOR STREQUAL "Xcode")
  SET(BUILD_IS_SINGLE_CONFIG FALSE)
ENDIF()

IF(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  SET(MY_COMPILER_IS_CLANG 1)
ELSEIF(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  SET(MY_COMPILER_IS_GNU 1)
ELSEIF(CMAKE_CXX_COMPILER_ID MATCHES "SunPro")
  SET(MY_COMPILER_IS_SUNPRO 1)
ENDIF()

IF(MY_COMPILER_IS_CLANG OR MY_COMPILER_IS_GNU)
  SET(MY_COMPILER_IS_GNU_OR_CLANG 1)
ENDIF()

# Maintainer mode is default on only for debug builds using GCC/G++
IF(CMAKE_BUILD_TYPE MATCHES "Debug" OR WITH_DEBUG)
  IF(CMAKE_COMPILER_IS_GNUCC AND CMAKE_COMPILER_IS_GNUCXX)
    SET(MYSQL_MAINTAINER_MODE ON CACHE BOOL
        "MySQL maintainer-specific development environment")
  ENDIF()
ENDIF()

OPTION(WITH_DEFAULT_COMPILER_OPTIONS
  "Use flags from cmake/build_configurations/compiler_options.cmake"
  ON)
OPTION(WITH_DEFAULT_FEATURE_SET
  "Use feature set in cmake/build_configurations/feature_set.cmake"
  ON)
IF(BUILD_CONFIG)
  INCLUDE(
  ${CMAKE_SOURCE_DIR}/cmake/build_configurations/${BUILD_CONFIG}.cmake)
ENDIF()

#cmake on 64bit windows/mac/solaris doesn't set CMAKE_SYSTEM_PROCESSOR correctly
SET(MYSQL_MACHINE_TYPE ${CMAKE_SYSTEM_PROCESSOR})


# Include the platform-specific file. To allow exceptions, this code
# looks for files in order of how specific they are. If there is, for
# example, a generic Linux.cmake and a version-specific
# Linux-2.6.28-11-generic, it will pick Linux-2.6.28-11-generic and
# include it. It is then up to the file writer to include the generic
# version if necessary.
FOREACH(_base
    ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_VERSION}-${CMAKE_SYSTEM_PROCESSOR}
    ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_VERSION}
    ${CMAKE_SYSTEM_NAME})
  SET(_file ${CMAKE_SOURCE_DIR}/cmake/os/${_base}.cmake)
  IF(EXISTS ${_file})
    INCLUDE(${_file})
    BREAK()
  ENDIF()
ENDFOREACH()


IF(UNIX)
  OPTION(WITH_INNODB_MEMCACHED "" OFF)
  OPTION(ENABLE_MEMCACHED_SASL "Enable SASL on InnoDB Memcached" OFF)
  OPTION(ENABLE_MEMCACHED_SASL_PWDB "Enable SASL on InnoDB Memcached" OFF)
ELSE()
  OPTION(WITH_INNODB_MEMCACHED "" OFF)
ENDIF()
 
# Following autotools tradition, add preprocessor definitions
# specified in environment variable CPPFLAGS
IF(DEFINED ENV{CPPFLAGS})
  ADD_DEFINITIONS($ENV{CPPFLAGS})
ENDIF()

INCLUDE(CheckTypeSize)
CHECK_TYPE_SIZE("void *" SIZEOF_VOIDP)
MESSAGE(STATUS "SIZEOF_VOIDP ${SIZEOF_VOIDP}")
# On some platforms, cmake may think that CMAKE_SIZEOF_VOID_P == 4
# even if we have configured for 64bit build....
SET(CMAKE_SIZEOF_VOID_P ${SIZEOF_VOIDP})

IF(WITH_DEFAULT_COMPILER_OPTIONS)
  INCLUDE(${CMAKE_SOURCE_DIR}/cmake/build_configurations/compiler_options.cmake)
ENDIF()
IF(WITH_DEFAULT_FEATURE_SET)
  INCLUDE(${CMAKE_SOURCE_DIR}/cmake/build_configurations/feature_set.cmake)
ENDIF()

INCLUDE(CMakePushCheckState)

# Add macros
INCLUDE(character_sets)
INCLUDE(compile_flags)
INCLUDE(cpu_info)
INCLUDE(zlib)
INCLUDE(lz4)
INCLUDE(libevent)
INCLUDE(ssl)
INCLUDE(sasl)
INCLUDE(ldap)
INCLUDE(rpc)
INCLUDE(readline)
INCLUDE(protobuf)
INCLUDE(mysql_version)
INCLUDE(libutils)
INCLUDE(dtrace)
INCLUDE(plugin)
INCLUDE(install_macros)
INCLUDE(install_layout)
INCLUDE(mysql_add_executable)
INCLUDE(curl)

# Handle options
IF(EXISTS ${CMAKE_SOURCE_DIR}/rapid)
  OPTION(WITH_RAPID
    "Build additonal code(plugins) that is located in rapid directory" ON)
ELSE()
  OPTION(WITH_RAPID
    "Build additonal code(plugins) that is located in rapid directory" OFF)
ENDIF()
OPTION(DISABLE_SHARED 
 "Don't build shared libraries, compile code as position-dependent" OFF)
IF(DISABLE_SHARED)
  SET(WITHOUT_DYNAMIC_PLUGINS 1)
ENDIF()
OPTION(ENABLED_PROFILING "Enable profiling" OFF)
OPTION(WITHOUT_SERVER OFF)
IF(UNIX)
  OPTION(WITH_VALGRIND "Valgrind instrumentation" OFF)
ENDIF()
IF(WIN32)
  OPTION(WITH_MSCRT_DEBUG "MS Visual Studio Debug CRT instrumentation" OFF)
ENDIF()
IF(NOT WITHOUT_SERVER)
  OPTION (WITH_UNIT_TESTS "Compile MySQL with unit tests" OFF)
ENDIF()
OPTION(WITH_AUTHENTICATION_LDAP
  "Report error if the LDAP authentication plugin cannot be built." OFF)

OPTION(FORCE_UNSUPPORTED_COMPILER "Disable compiler version checks" OFF)
MARK_AS_ADVANCED(WITHOUT_SERVER DISABLE_SHARED FORCE_UNSUPPORTED_COMPILER)

OPTION(WITH_XTRABACKUP "Build XtraBackup" ON)
# Disallow WITH_XTRABACKUP=OFF builds
IF(NOT WITH_XTRABACKUP)
  MESSAGE(FATAL_ERROR "This branch does not support -DWITH_XTRABACKUP=OFF configuration.")
ELSE()
  ADD_DEFINITIONS(-DXTRABACKUP)
ENDIF()
# Disallow WITHOUT_SERVER=ON builds
IF(WITHOUT_SERVER)
  MESSAGE(FATAL_ERROR "This branch does not support -DWITHOUT_SERVER=ON configuration.")
ENDIF()
# Disallow WITH_EMBEDDED_SERVER=OFF builds
IF(NOT WITH_EMBEDDED_SERVER)
  MESSAGE(FATAL_ERROR "This branch does not support -DWITH_EMBEDDED_SERVER=OFF configuration.")
ENDIF()

include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)
# We need some extra FAIL_REGEX patterns
# Note that CHECK_C_SOURCE_COMPILES is a misnomer, it will also link.
MACRO (MY_CHECK_C_COMPILER_FLAG FLAG RESULT)
  SET(SAVE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
  SET(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${FLAG}")
  CHECK_C_SOURCE_COMPILES("int main(void) { return 0; }" ${RESULT}
    FAIL_REGEX "argument unused during compilation"
    FAIL_REGEX "unsupported .*option"
    FAIL_REGEX "unknown .*option"
    FAIL_REGEX "unrecognized .*option"
    FAIL_REGEX "ignoring unknown option"
    FAIL_REGEX "[Ww]arning: [Oo]ption"
    FAIL_REGEX "error: visibility"
    FAIL_REGEX "warning: visibility"
    )
  SET(CMAKE_REQUIRED_FLAGS "${SAVE_CMAKE_REQUIRED_FLAGS}")
ENDMACRO()

MACRO (MY_CHECK_CXX_COMPILER_FLAG FLAG RESULT)
  SET(SAVE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
  SET(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${FLAG}")
  CHECK_CXX_SOURCE_COMPILES("int main(void) { return 0; }" ${RESULT}
    FAIL_REGEX "argument unused during compilation"
    FAIL_REGEX "unsupported .*option"
    FAIL_REGEX "unknown .*option"
    FAIL_REGEX "unrecognized .*option"
    FAIL_REGEX "ignoring unknown option"
    FAIL_REGEX "[Ww]arning: [Oo]ption"
    FAIL_REGEX "error: visibility"
    FAIL_REGEX "warning: visibility"
    )
  SET(CMAKE_REQUIRED_FLAGS "${SAVE_CMAKE_REQUIRED_FLAGS}")
ENDMACRO()

# Check whether a specific warning option is supported.
# Intended use is primarily to silence warnings from third-party code.
# If *our* code has warnings with gcc[56789] or clang[345678] the
# code should be fixed, rather than silencing the compiler warnings.
#
# Returns the corresponding "-Wno-<option>" if supported, 0 otherwise.
# WARNING_OPTION should be as found in clang/gcc documentation.
# Note: Sun Studio accepts/ignores anything starting with "-W".
#       Visual Studio has a few /W options, but are totally different.
# Hence we check for gcc/clang only.
# No return value for functions in cmake, so we use an output argument.
FUNCTION(MY_CHECK_CXX_COMPILER_WARNING WARNING_OPTION RETURN_VALUE)
  # Strip off possible leading -Wno- or -W
  STRING(REGEX REPLACE "^-Wno-" "" WARNING_OPTION ${WARNING_OPTION})
  STRING(REGEX REPLACE "^-W" "" WARNING_OPTION ${WARNING_OPTION})
  # Use HAVE_CXX_W_warning_option as cache variable.
  STRING(REPLACE "-" "_" VAR_NAME ${WARNING_OPTION})
  STRING(REPLACE "=" "_" VAR_NAME ${VAR_NAME})
  STRING(REPLACE "c++" "cpp" VAR_NAME ${VAR_NAME})
  SET(VAR_NAME "HAVE_CXX_W_${VAR_NAME}")
  IF(MY_COMPILER_IS_GNU_OR_CLANG)
    MY_CHECK_CXX_COMPILER_FLAG("-W${WARNING_OPTION}" ${VAR_NAME})
  ELSE()
    SET(${VAR_NAME} 0 CACHE INTERNAL "-W${WARNING_OPTION} not supported")
  ENDIF()
  IF(${VAR_NAME})
    SET(${RETURN_VALUE} "-Wno-${WARNING_OPTION}" PARENT_SCOPE)
  ELSE()
    SET(${RETURN_VALUE} 0 PARENT_SCOPE)
  ENDIF()
ENDFUNCTION()

MACRO(MY_SANITIZER_CHECK SAN_OPT RESULT)
  MY_CHECK_C_COMPILER_FLAG("${SAN_OPT}" C_RESULT)
  MY_CHECK_CXX_COMPILER_FLAG("${SAN_OPT}" CXX_RESULT)
  IF(C_RESULT AND CXX_RESULT)
    STRING_APPEND(CMAKE_C_FLAGS   " ${SAN_OPT}")
    STRING_APPEND(CMAKE_CXX_FLAGS " ${SAN_OPT}")
    # We switch on basic optimization also for debug builds.
    # We disable inlining for stack trace readability.
    # With optimization we may get some warnings, so we switch off -Werror
    STRING_APPEND(CMAKE_C_FLAGS   " -O1 -fno-inline -fPIC")
    STRING_APPEND(CMAKE_CXX_FLAGS " -O1 -fno-inline -fPIC")
    IF(NOT SANITIZE_MAINTAINER_MODE)
      SET(MYSQL_MAINTAINER_MODE OFF CACHE BOOL
        "MySQL maintainer-specific development environment" FORCE)
    ENDIF()
    SET(${RESULT} 1)
  ELSE()
    SET(${RESULT} 0)
  ENDIF()
ENDMACRO()

OPTION(WITH_ASAN "Enable address sanitizer" OFF)
IF(WITH_ASAN)
  MY_SANITIZER_CHECK("-fsanitize=address" WITH_ASAN_OK)
  IF(WITH_ASAN_OK)
    OPTION(WITH_ASAN_SCOPE "Enable -fsanitize-address-use-after-scope" ON)
    SET(HAVE_ASAN 1)
    # This works with clang, but not gcc it seems.
    MY_CHECK_CXX_COMPILER_FLAG(
      "-fsanitize=address -fsanitize-address-use-after-scope"
      HAVE_SANITIZE_SCOPE)
    IF(WITH_ASAN_SCOPE AND HAVE_SANITIZE_SCOPE)
      STRING_APPEND(CMAKE_C_FLAGS   " -fsanitize-address-use-after-scope")
      STRING_APPEND(CMAKE_CXX_FLAGS " -fsanitize-address-use-after-scope")
    ENDIF()
  ELSE()
    MESSAGE(FATAL_ERROR "Do not know how to enable address sanitizer")
  ENDIF()
ENDIF()

OPTION(WITH_MSAN "Enable memory sanitizer" OFF)
IF(WITH_MSAN)
  MY_SANITIZER_CHECK("-fsanitize=memory -fsanitize-memory-track-origins"
                     WITH_MSAN_OK)
  IF(NOT WITH_MSAN_OK)
    MESSAGE(FATAL_ERROR "Do not know how to enable memory sanitizer")
  ENDIF()
ENDIF()

OPTION(WITH_UBSAN "Enable undefined behavior sanitizer" OFF)
IF(WITH_UBSAN)
  MY_SANITIZER_CHECK("-fsanitize=undefined" WITH_UBSAN_OK)
  IF(NOT WITH_UBSAN_OK)
    MESSAGE(FATAL_ERROR
            "Do not know how to enable undefined behavior sanitizer")
  ELSE()
    MESSAGE(WARNING
            "Note that Undefined Behavior support is currently experimental.")
  ENDIF()
ENDIF()

IF(WITH_ASAN AND WITH_MSAN)
  MESSAGE(FATAL_ERROR
          "Cannot use AddressSanitizer and MemorySanitizer together")
ENDIF()


# Older versions of ccache must be disabled: export CCACHE_DISABLE=1
# See http://www.cmake.org/Wiki/CTest/Coverage
OPTION(ENABLE_GCOV "Enable gcov (debug, Linux builds only)" OFF)
IF (ENABLE_GCOV AND NOT WIN32 AND NOT APPLE)
  STRING_APPEND(CMAKE_C_FLAGS   " -fprofile-arcs -ftest-coverage -DHAVE_GCOV")
  STRING_APPEND(CMAKE_CXX_FLAGS " -fprofile-arcs -ftest-coverage -DHAVE_GCOV")
  STRING_APPEND(CMAKE_EXE_LINKER_FLAGS " -fprofile-arcs -ftest-coverage -lgcov")
ENDIF()

OPTION(ENABLE_GPROF "Enable gprof (optimized, Linux builds only)" OFF)
IF (ENABLE_GPROF AND NOT WIN32 AND NOT APPLE)
  STRING_APPEND(CMAKE_C_FLAGS " -pg")
  STRING_APPEND(CMAKE_CXX_FLAGS " -pg")
  STRING_APPEND(CMAKE_EXE_LINKER_FLAGS " -pg")
ENDIF()

IF(CMAKE_SYSTEM_NAME MATCHES "Linux")
  OPTION(REPRODUCIBLE_BUILD "Take extra pains to make build result independent of build location and time" OFF)
ENDIF()
IF(REPRODUCIBLE_BUILD)
  SET(DEBUG_PREFIX_FLAGS
    "-fdebug-prefix-map=${CMAKE_SOURCE_DIR}/=./ -fdebug-prefix-map=${CMAKE_CURRENT_BINARY_DIR}=./obj")

  # See if -fdebug-prefix= commands are included in the debug output,
  # making the build unreproducible with switches recorded.
  # See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=69821.
  EXECUTE_PROCESS(COMMAND ${CMAKE_C_COMPILER} -g3 -x c -S -fdebug-prefix-map=foo=bar -o - -
    INPUT_FILE /dev/null OUTPUT_VARIABLE DEBUG_PREFIX_MAP_RESULT)
  IF(DEBUG_PREFIX_MAP_RESULT MATCHES "foo=bar")
    SET(DEBUG_PREFIX_FLAGS "${DEBUG_PREFIX_FLAGS} -gno-record-gcc-switches")
  ENDIF()

  STRING_APPEND(CMAKE_C_FLAGS   " ${DEBUG_PREFIX_FLAGS}")
  STRING_APPEND(CMAKE_CXX_FLAGS " ${DEBUG_PREFIX_FLAGS}")

  SET(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--build-id=none")
  SET(CMAKE_CXX_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--build-id=none")

  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE
    "${CMAKE_SOURCE_DIR}/scripts/invoke-with-relative-paths.pl")
ENDIF()

OPTION(ENABLED_LOCAL_INFILE
 "If we should enable LOAD DATA LOCAL by default" ${IF_WIN})
MARK_AS_ADVANCED(ENABLED_LOCAL_INFILE)

OPTION(OPTIMIZER_TRACE "Support tracing of Optimizer" ON)

#
# Options related to client-side protocol tracing
#

OPTION(WITH_CLIENT_PROTOCOL_TRACING
  "Support for client-side protocol tracing plugins" ON)
OPTION(WITH_TEST_TRACE_PLUGIN
  "Have a built-in test protocol trace plugin in libmysql (requires WITH_CLIENT_PROTOCOL_TRACING option)" OFF)

# Sanity checks for protocol tracing options

IF(WITH_TEST_TRACE_PLUGIN AND NOT WITH_CLIENT_PROTOCOL_TRACING)
MESSAGE("WARNING: Test trace plugin was selected but client protocol tracing infrastructure is not enabled - ignoring")
SET(WITH_TEST_TRACE_PLUGIN 0)
ENDIF()

IF(WITH_TEST_TRACE_PLUGIN AND NOT CMAKE_BUILD_TYPE MATCHES "Debug")
  MESSAGE(SEND_ERROR 
          "Test trace plugin was selected but it can be included only in debug binaries.
          Set WITH_TEST_TRACE_PLUGIN to OFF or WITH_DEBUG to ON.")
ENDIF()

# Add safemutex for debug configurations
IF(NOT WITH_INNODB_MEMCACHED)
  FOREACH(LANG C CXX)
    STRING_PREPEND(CMAKE_${LANG}_FLAGS_DEBUG "-DSAFE_MUTEX ")
  ENDFOREACH()
ENDIF()

# Enable debug sync for debug builds.
FOREACH(LANG C CXX)
  STRING_PREPEND(CMAKE_${LANG}_FLAGS_DEBUG "-DENABLED_DEBUG_SYNC ")
ENDFOREACH()


# Set commonly used variables
IF(WIN32)
  SET(DEFAULT_MYSQL_HOME "C:/Program Files/MySQL/MySQL Server ${MYSQL_BASE_VERSION}" )
  SET(SHAREDIR share)
ELSE()
  SET(DEFAULT_MYSQL_HOME ${CMAKE_INSTALL_PREFIX})
  SET(SHAREDIR ${DEFAULT_MYSQL_HOME}/${INSTALL_MYSQLSHAREDIR})
ENDIF()

SET(DEFAULT_BASEDIR "${DEFAULT_MYSQL_HOME}")
IF(INSTALL_MYSQLDATADIR MATCHES "^/.*")
  SET(MYSQL_DATADIR ${INSTALL_MYSQLDATADIR} CACHE PATH "default MySQL data directory")
ELSE()
  SET(MYSQL_DATADIR "${DEFAULT_MYSQL_HOME}/${INSTALL_MYSQLDATADIR}" CACHE PATH "default MySQL data directory")
ENDIF()
IF(INSTALL_MYSQLKEYRINGDIR MATCHES "^/.*")
  SET(MYSQL_KEYRINGDIR ${INSTALL_MYSQLKEYRINGDIR} CACHE PATH "default MySQL keyring directory")
ELSE()
  SET(MYSQL_KEYRINGDIR "${DEFAULT_MYSQL_HOME}/${INSTALL_MYSQLKEYRINGDIR}" CACHE PATH "default MySQL keyring directory")
ENDIF()
SET(DEFAULT_CHARSET_HOME "${DEFAULT_MYSQL_HOME}")
SET(PLUGINDIR "${DEFAULT_MYSQL_HOME}/${INSTALL_PLUGINDIR}")
IF(SYSCONFDIR)
  SET(DEFAULT_SYSCONFDIR "${SYSCONFDIR}")
ENDIF()

IF(WIN32) # P_tmpdir is not defined on Windows as of VS2015.
  SET(TMPDIR "" # So we use empty path as default. In practice TMP/TEMP is used
    CACHE PATH
    "PATH to MySQL TMP dir")
ELSE()
  SET(TMPDIR "P_tmpdir"
    CACHE PATH
    "PATH to MySQL TMP dir. Defaults to the P_tmpdir macro in <stdio.h>")
ENDIF()
IF(TMPDIR STREQUAL "P_tmpdir")
  # Do not quote it, to refer to the P_tmpdir macro.
  SET(DEFAULT_TMPDIR "P_tmpdir")
ELSE()
  # Quote it, to make it a const char string.
  SET(DEFAULT_TMPDIR "\"${TMPDIR}\"")
ENDIF()

INCLUDE(cmake/boost.cmake)

IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
  OPTION(WITH_SYSTEMD "Enable installation of systemd support files" OFF)
  IF (WITH_SYSTEMD)
    INCLUDE(cmake/systemd.cmake)
  ENDIF()
ELSE()
  IF (WITH_SYSTEMD)
    MESSAGE(FATAL_ERROR "Installation of systemd support files not supported")
  ENDIF()
ENDIF()

# Run platform tests
INCLUDE(configure.cmake)

IF(APPLE)
  # Homebrew default location is
  IF(APPLE_ARM)
    SET(HOMEBREW_HOME "/opt/homebrew/opt")
  ELSE()
    SET(HOMEBREW_HOME "/usr/local/opt")
  ENDIF()
ENDIF()

# Common defines and includes
ADD_DEFINITIONS(-DHAVE_CONFIG_H)
INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR}/include
  ${CMAKE_SOURCE_DIR}/extra/rapidjson/include
  ${CMAKE_CURRENT_BINARY_DIR}/libbinlogevents/include
  ${CMAKE_SOURCE_DIR}/libbinlogevents/export)
# Add bundled or system zlib.
MYSQL_CHECK_ZLIB()
# Add openssl.
MYSQL_CHECK_SSL()
# Add system/bundled editline.
MYSQL_CHECK_EDITLINE()
# Add libevent
MYSQL_CHECK_LIBEVENT()
# Add lz4 library
MYSQL_CHECK_LZ4()
# Add SASL library
MYSQL_CHECK_SASL()
# Add LDAP library
MYSQL_CHECK_LDAP()
# Add protoc and libprotobuf
IF(NOT WITHOUT_SERVER)
  MYSQL_CHECK_PROTOBUF()
ENDIF()
# Try and set CURL_LIBRARY
MYSQL_CHECK_CURL()

# Check for SYS_thread_selfid system call
CHECK_C_SOURCE_COMPILES("
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
int main(int ac, char **av)
{
  unsigned long long tid = syscall(SYS_thread_selfid);
  return (tid != 0 ? 0 : 1);
}"
HAVE_SYS_THREAD_SELFID)

# Check for gettid() system call
CHECK_C_SOURCE_COMPILES("
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
int main(int ac, char **av)
{
  unsigned long long tid = syscall(SYS_gettid);
  return (tid != 0 ? 0 : 1);
}"
HAVE_SYS_GETTID)

IF(CMAKE_SYSTEM_NAME MATCHES "Linux")
  # Check for pthread_setname_np
  CHECK_C_SOURCE_COMPILES("
  #include <pthread.h>
  int main(int ac, char **av)
  {
    const char *thread_name= 0;
    int ret = pthread_setname_np(pthread_self(), thread_name);
    return ret;
  }"
  HAVE_PTHREAD_SETNAME_NP)
ENDIF()

# Check for pthread_getthreadid_np()
CHECK_C_SOURCE_COMPILES("
#include <pthread_np.h>
int main(int ac, char **av)
{
  unsigned long long tid = pthread_getthreadid_np();
  return (tid != 0 ? 0 : 1);
}"
HAVE_PTHREAD_GETTHREADID_NP)

# Check for pthread_self() returning an integer type
CHECK_C_SOURCE_COMPILES("
#include <sys/types.h>
#include <pthread.h>
int main(int ac, char **av)
{
  unsigned long long tid = pthread_self();
  return (tid != 0 ? 0 : 1);
}"
HAVE_INTEGER_PTHREAD_SELF
FAIL_REGEX "warning: incompatible pointer to integer conversion"
)

#
# Setup maintainer mode options by the end. Platform checks are
# not run with the warning options as to not perturb fragile checks
# (i.e. do not make warnings into errors).
#
# Why don't these flags affect the entire build?
# Because things may already have been included with ADD_SUBDIRECTORY
#
OPTION(MYSQL_MAINTAINER_MODE
       "MySQL maintainer-specific development environment" OFF)

INCLUDE(maintainer)

IF(WITH_UNIT_TESTS AND NOT WITH_XTRABACKUP)
 ENABLE_TESTING()
ENDIF()

ADD_SUBDIRECTORY(include)
ADD_SUBDIRECTORY(dbug)
ADD_SUBDIRECTORY(strings)
ADD_SUBDIRECTORY(vio)
ADD_SUBDIRECTORY(regex)
ADD_SUBDIRECTORY(mysys)
ADD_SUBDIRECTORY(mysys_ssl)
ADD_SUBDIRECTORY(libmysql)
ADD_SUBDIRECTORY(libbinlogevents)
ADD_SUBDIRECTORY(libbinlogstandalone)

# gcc9 will not allow CTORs to do memset(this, 0, sizeof(*this));
IF(CMAKE_COMPILER_IS_GNUCXX AND NOT GXX_VERSION VERSION_LESS 9)
  STRING_APPEND(CMAKE_CXX_FLAGS " -Wno-class-memaccess")
ENDIF()

IF(NOT WITHOUT_SERVER)
  SET (MYSQLD_STATIC_PLUGIN_LIBS "" CACHE INTERNAL "")
  SET (MYSQLD_STATIC_EMBEDDED_PLUGIN_LIBS "" CACHE INTERNAL "")
  # Add storage engines and plugins.
  CONFIGURE_PLUGINS()
ELSE()
  # We may still want Cluster client libraries, use -DWITH_NDBCLUSTER=1
  ADD_SUBDIRECTORY(storage/ndb)
ENDIF()

IF(WITH_UNIT_TESTS AND NOT WITH_XTRABACKUP)
  MESSAGE(STATUS "With unit tests")
  ERROR()
  # Visual Studio 11 needs this extra flag in order to compile gmock.
  IF(WIN32)
    ADD_DEFINITIONS( /D _VARIADIC_MAX=10 )
  ENDIF()
  # libc++ doesn't have tuple in tr1
  IF(HAVE_LLVM_LIBCPP)
    ADD_DEFINITIONS(-DGTEST_USE_OWN_TR1_TUPLE=1)
  ENDIF()

  ADD_SUBDIRECTORY(unittest)
  ADD_SUBDIRECTORY(unittest/examples)
  ADD_SUBDIRECTORY(unittest/mytap)
  ADD_SUBDIRECTORY(unittest/mytap/t)
  IF(WITH_RAPID AND EXISTS ${CMAKE_SOURCE_DIR}/rapid/unittest/gunit)
    ADD_SUBDIRECTORY(rapid/unittest/gunit)
  ENDIF()
ENDIF()

ADD_SUBDIRECTORY(sql/share)
ADD_SUBDIRECTORY(libservices)
ADD_SUBDIRECTORY(extra)
IF(NOT WITHOUT_SERVER)
  ADD_SUBDIRECTORY(sql)
  OPTION (WITH_EMBEDDED_SERVER "Compile MySQL with embedded server" OFF)
  IF(WITH_EMBEDDED_SERVER) 
   ADD_SUBDIRECTORY(libmysqld)
  ENDIF(WITH_EMBEDDED_SERVER)
ENDIF()

# scripts/mysql_config depends on client and server targets loaded above.
# It is referenced by some of the directories below, so we insert it here.
ADD_SUBDIRECTORY(scripts)

IF(NOT WITHOUT_SERVER)
  ADD_SUBDIRECTORY(mysql-test)
  ADD_SUBDIRECTORY(mysql-test/lib/My/SafeProcess)
  IF(EXISTS ${CMAKE_SOURCE_DIR}/internal/CMakeLists.txt)
    ADD_SUBDIRECTORY(internal)
  ENDIF()
  ADD_SUBDIRECTORY(packaging/rpm-common)
  ADD_SUBDIRECTORY(packaging/rpm-oel)
  ADD_SUBDIRECTORY(packaging/rpm-fedora)
  ADD_SUBDIRECTORY(packaging/rpm-sles)
  ADD_SUBDIRECTORY(packaging/rpm-docker)
ENDIF()

INCLUDE(cmake/abi_check.cmake)
INCLUDE(cmake/tags.cmake)

CONFIGURE_FILE(config.h.cmake   ${CMAKE_BINARY_DIR}/include/my_config.h)
CONFIGURE_FILE(config.h.cmake   ${CMAKE_BINARY_DIR}/include/config.h)
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/include/mysql_version.h.in
               ${CMAKE_BINARY_DIR}/include/mysql_version.h )
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/sql/sql_builtin.cc.in
    ${CMAKE_BINARY_DIR}/sql/sql_builtin.cc)
CONFIGURE_FILE(
    ${CMAKE_SOURCE_DIR}/cmake/info_macros.cmake.in
    ${CMAKE_BINARY_DIR}/info_macros.cmake @ONLY)

SET(KNOWN_CONVENIENCE_LIBRARIES "" CACHE INTERNAL "")

# Handle the "INFO_*" files.
INCLUDE(${CMAKE_BINARY_DIR}/info_macros.cmake)
# Source: This can be done during the cmake phase, all information is
# available, but should be repeated on each "make" just in case someone
# does "cmake ; make ; git pull ; make".
CREATE_INFO_SRC(${CMAKE_BINARY_DIR}/Docs)
ADD_CUSTOM_TARGET(INFO_SRC ALL
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/cmake/info_src.cmake
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
# Build flags: This must be postponed to the make phase.
ADD_CUSTOM_TARGET(INFO_BIN ALL
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/cmake/info_bin.cmake
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Packaging
IF(WIN32)
  SET(CPACK_GENERATOR "ZIP")
ELSE()
  SET(CPACK_GENERATOR "TGZ")
ENDIF() 

#
# RPM installs documentation directly from the source tree
#
IF(NOT INSTALL_LAYOUT MATCHES "RPM")
ENDIF()

INCLUDE(CPack)

# C/CXX compiler flags consist of:
# CPPFLAGS        Taken from environment, see above.
# ADD_DEFINITIONS In each individual CMakeLists.txt
# CMAKE_CXX_FLAGS From command line.
#                 We extend these in maintainer.cmake
#                 We extend these in compiler_options.cmake
# ENV{CXXFLAGS}   From environment, but environment is ignored if
#                 CMAKE_CXX_FLAGS is also given on command line
# CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}, typical initial cmake values:
#      DEBUG          -g
#      RELWITHDEBINFO -O2 -g -DNDEBUG
#      RELEASE        -O3 -DNDEBUG
#      MINSIZEREL     -Os -DNDEBUG
#
# The compiler command line is
# g++ DEFINITIONS CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_<configuration>
#
# So, if you want to use '-O3' for a RELWITHDEBINFO build, you should do:
# cmake -DCMAKE_CXX_FLAGS_RELWITHDEBINFO="-O3 -g -DNDEBUG"
#
# Note that CMakeCache.txt contains cmake builtins for these variables,
# *not* the values that will actually be used, which we report here:


IF(BUILD_IS_SINGLE_CONFIG)
  MESSAGE(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
ENDIF()
GET_PROPERTY(cwd_definitions DIRECTORY PROPERTY COMPILE_DEFINITIONS)
MESSAGE(STATUS "COMPILE_DEFINITIONS: ${cwd_definitions}")
REPORT_CXX_FLAGS()
MESSAGE(STATUS "CMAKE_C_LINK_FLAGS: ${CMAKE_C_LINK_FLAGS}")
MESSAGE(STATUS "CMAKE_CXX_LINK_FLAGS: ${CMAKE_CXX_LINK_FLAGS}")
MESSAGE(STATUS "CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS}")
MESSAGE(STATUS "CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS}")
MESSAGE(STATUS "CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS}")
