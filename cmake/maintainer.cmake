# Copyright (c) 2010, 2023, Oracle and/or its affiliates.
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
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA

MACRO(MY_ADD_C_WARNING_FLAG WARNING_FLAG)
  MY_CHECK_C_COMPILER_FLAG("-${WARNING_FLAG}" HAVE_${WARNING_FLAG})
  IF(HAVE_${WARNING_FLAG})
    SET(MY_C_WARNING_FLAGS "${MY_C_WARNING_FLAGS} -${WARNING_FLAG}")
  ENDIF()
ENDMACRO()

MACRO(MY_ADD_CXX_WARNING_FLAG WARNING_FLAG)
  STRING(REPLACE "c++" "cpp" WARNING_VAR ${WARNING_FLAG})
  MY_CHECK_CXX_COMPILER_FLAG("-${WARNING_FLAG}" HAVE_${WARNING_VAR})
  IF(HAVE_${WARNING_VAR})
    SET(MY_CXX_WARNING_FLAGS "${MY_CXX_WARNING_FLAGS} -${WARNING_FLAG}")
  ENDIF()
ENDMACRO()

# Common warning flags for GCC, G++, Clang and Clang++
SET(MY_WARNING_FLAGS "-Wall -Wextra -Wformat-security -Wvla")

# The default =3 given by -Wextra is a bit too strict for our code.
IF(CMAKE_COMPILER_IS_GNUCXX)
  MY_CHECK_CXX_COMPILER_FLAG("-Wimplicit-fallthrough=2"
    HAVE_IMPLICIT_FALLTHROUGH)
  IF(HAVE_IMPLICIT_FALLTHROUGH)
    SET(MY_WARNING_FLAGS "${MY_WARNING_FLAGS} -Wimplicit-fallthrough=2")
  ENDIF()
ENDIF()

# Common warning flags for GCC and Clang
SET(MY_C_WARNING_FLAGS
    "${MY_WARNING_FLAGS} -Wwrite-strings -Wdeclaration-after-statement")

# Common warning flags for G++ and Clang++
SET(MY_CXX_WARNING_FLAGS
    "${MY_WARNING_FLAGS} -Woverloaded-virtual -Wno-unused-parameter")

# The default =3 given by -Wextra is a bit too strict for our code.
IF(CMAKE_COMPILER_IS_GNUCXX)
  MY_ADD_CXX_WARNING_FLAG("Wimplicit-fallthrough=2")
ENDIF()

# Extra warning flags for Clang++
IF(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  SET(MY_CXX_WARNING_FLAGS
      "${MY_CXX_WARNING_FLAGS} -Wno-null-conversion -Wno-unused-private-field")
ENDIF()

# Turn on Werror (warning => error) when using maintainer mode.
IF(MYSQL_MAINTAINER_MODE)
  STRING_APPEND(MY_C_WARNING_FLAGS   " -Werror")
  STRING_APPEND(MY_CXX_WARNING_FLAGS " -Werror")
  MY_ADD_CXX_WARNING_FLAG("Wno-error=class-memaccess")
ENDIF()

# Set warning flags for GCC/Clang
IF(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
  STRING_APPEND(CMAKE_C_FLAGS " ${MY_C_WARNING_FLAGS}")
ENDIF()
# Set warning flags for G++/Clang++
IF(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  STRING_APPEND(CMAKE_CXX_FLAGS " ${MY_CXX_WARNING_FLAGS}")
ENDIF()
