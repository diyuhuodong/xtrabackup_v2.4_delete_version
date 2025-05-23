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

SET(HEADERS_GEN_CONFIGURE 
${CMAKE_CURRENT_BINARY_DIR}/mysql_version.h
${CMAKE_CURRENT_BINARY_DIR}/my_config.h
${CMAKE_CURRENT_BINARY_DIR}/mysqld_ername.h
${CMAKE_CURRENT_BINARY_DIR}/mysqld_error.h
${CMAKE_CURRENT_BINARY_DIR}/sql_state.h
)
SET(HEADERS_ABI 
  mysql.h 
  mysql_com.h
  my_command.h
  mysql_time.h 
  my_list.h
  my_alloc.h
  typelib.h 
  mysql/plugin.h
  mysql/plugin_audit.h
  mysql/plugin_ftparser.h
  mysql/plugin_validate_password.h
  mysql/plugin_keyring.h
  mysql/plugin_group_replication.h
)

SET(HEADERS 
  ${HEADERS_ABI} 
  my_dbug.h 
  m_string.h
  my_sys.h 
  my_xml.h
  mysql_embed.h
  my_thread.h
  my_thread_local.h
  decimal.h
  errmsg.h
  my_global.h
  my_getopt.h
  sslopt-longopts.h
  my_dir.h 
  sslopt-vars.h
  sslopt-case.h
  sql_common.h
  keycache.h
  m_ctype.h
  my_compiler.h
  mysql_com_server.h
  my_byteorder.h
  byte_order_generic.h
  byte_order_generic_x86.h
  little_endian.h
  big_endian.h
  thr_cond.h
  thr_mutex.h
  thr_rwlock.h
  ${HEADERS_GEN_CONFIGURE}
)


