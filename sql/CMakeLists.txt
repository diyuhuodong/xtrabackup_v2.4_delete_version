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


INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}/include 
  ${CMAKE_SOURCE_DIR}/sql/conn_handler
  ${CMAKE_SOURCE_DIR}/libbinlogevents/include
  ${CMAKE_SOURCE_DIR}/sql 
  ${CMAKE_SOURCE_DIR}/sql/auth
  ${CMAKE_SOURCE_DIR}/regex 
  ${SSL_INCLUDE_DIRS}
  ${CMAKE_BINARY_DIR}/sql
)

SET(CONF_SOURCES
  ${CMAKE_CURRENT_BINARY_DIR}/sql_builtin.cc 
)
SET(GEN_SOURCES
  ${CMAKE_CURRENT_BINARY_DIR}/sql_yacc.h 
  ${CMAKE_CURRENT_BINARY_DIR}/sql_yacc.cc
  ${CMAKE_CURRENT_BINARY_DIR}/sql_hints.yy.h 
  ${CMAKE_CURRENT_BINARY_DIR}/sql_hints.yy.cc
  ${CMAKE_CURRENT_BINARY_DIR}/lex_hash.h 
)
SET(GEN_DIGEST_SOURCES
  ${CMAKE_CURRENT_BINARY_DIR}/lex_token.h 
)

SET_SOURCE_FILES_PROPERTIES(${GEN_SOURCES}
                            ${GEN_DIGEST_SOURCES}
                            ${CONF_SOURCES}
                            PROPERTIES GENERATED 1)

# Gen_lex_token
# Make sure sql_yacc.h is generated before compiling gen_lex_token
ADD_EXECUTABLE(gen_lex_token gen_lex_token.cc)
ADD_DEPENDENCIES(gen_lex_token GenServerSource)

ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/lex_token.h
  COMMAND gen_lex_token > lex_token.h
  DEPENDS gen_lex_token
)

ADD_DEFINITIONS(-DMYSQL_SERVER -DHAVE_REPLICATION)
IF(SSL_DEFINES)
 ADD_DEFINITIONS(${SSL_DEFINES})
ENDIF()

SET(SQL_SHARED_SOURCES
  abstract_query_plan.cc
  auth/sql_authentication.cc
  auth/sql_auth_cache.cc
  auth/sql_authorization.cc
  auth/sql_user_table.cc
  auth/sql_user.cc
  auth/password.c
  auth/password_policy_service.cc
  auth/sql_security_ctx.cc
  auth/service_security_context.cc
  keyring_service.cc
  ssl_wrapper_service.cc
  bootstrap.cc
  conn_handler/connection_handler_manager.cc 
  datadict.cc
  debug_sync.cc
  derror.cc
  discover.cc
  field.cc
  field_conv.cc 
  filesort.cc
  filesort_utils.cc
  aggregate_check.cc
  geometry_rtree.cc
  gstream.cc
  handler.cc
  hostname.cc
  init.cc
  item.cc
  item_buff.cc
  item_cmpfunc.cc 
  item_create.cc
  item_func.cc
  item_geofunc.cc
  item_geofunc_buffer.cc
  item_geofunc_internal.cc
  item_geofunc_relchecks.cc
  item_geofunc_relchecks_bgwrap.cc
  item_geofunc_setops.cc
  item_json_func.cc
  item_row.cc 
  item_strfunc.cc
  item_subselect.cc
  item_sum.cc
  item_timefunc.cc 
  item_xmlfunc.cc 
  item_inetfunc.cc
  json_dom.cc
  json_binary.cc
  json_path.cc
  key.cc
  keycaches.cc
  lock.cc
  locking_service.cc
  locks/shared_spin_lock.cc
  log.cc
  mdl.cc
  migrate_keyring.cc
  my_decimal.cc
  net_serv.cc
  opt_costconstantcache.cc
  opt_costconstants.cc
  opt_costmodel.cc
  opt_explain.cc
  opt_explain_traditional.cc
  opt_explain_json.cc
  opt_hints.cc
  opt_range.cc
  opt_statistics.cc
  opt_sum.cc 
  opt_trace.cc
  opt_trace2server.cc
  parse_file.cc
  parse_tree_helpers.cc
  parse_tree_hints.cc
  parse_tree_items.cc
  parse_tree_node_base.cc
  parse_tree_nodes.cc
  select_lex_visitor.cc
  parser_service.cc
  partition_info.cc
  partitioning/partition_handler.cc
  procedure.cc 
  protocol_classic.cc
  records.cc
  rpl_group_replication.cc
  rpl_handler.cc
  rpl_transaction_ctx.cc
  rpl_transaction_write_set_ctx.cc
  rpl_trx_tracking.cc
  rpl_write_set_handler.cc
  rules_table_service.cc
  session_tracker.cc
  set_var.cc 
  sp.cc
  sp_cache.cc
  sp_head.cc
  sp_instr.cc
  sp_pcontext.cc 
  sp_rcontext.cc
  spatial.cc
  string_service.cc
  sql_admin.cc
  sql_alloc_error_handler.cc
  sql_alter.cc
  sql_alter_instance.cc
  sql_analyse.cc
  sql_base.cc 
  sql_bootstrap.cc
  sql_initialize.cc
  sql_cache.cc
  sql_class.cc
  sql_connect.cc
  sql_crypt.cc
  sql_cursor.cc
  sql_data_change.cc
  sql_db.cc
  sql_delete.cc
  sql_derived.cc
  sql_digest.cc
  sql_do.cc 
  sql_error.cc
  sql_exception_handler.cc
  sql_executor.cc
  sql_get_diagnostics.cc
  sql_handler.cc
  sql_help.cc
  sql_insert.cc
  sql_join_buffer.cc
  sql_lex.cc 
  sql_lex_hash.cc
  sql_lex_hints.cc
  sql_list.cc
  sql_load.cc
  sql_locale.cc
  sql_manager.cc
  sql_optimizer.cc
  sql_parse.cc
  sql_partition.cc
  sql_partition_admin.cc
  sql_planner.cc
  sql_plugin.cc
  sql_prepare.cc
  sql_profile.cc
  sql_query_rewrite.cc
  sql_reload.cc
  sql_rename.cc
  sql_resolver.cc
  sql_rewrite.cc
  sql_select.cc
  sql_servers.cc
  sql_cmd_show.cc
  sql_show.cc
  sql_show_status.cc
  sql_show_processlist.cc
  sql_signal.cc
  sql_state.c
  sql_table.cc
  sql_tablespace.cc
  sql_test.cc
  sql_thd_internal_api.cc
  sql_time.cc
  sql_timer.cc
  sql_tmp_table.cc
  sql_trigger.cc
  sql_truncate.cc
  sql_udf.cc
  sql_union.cc
  sql_update.cc
  sql_view.cc
  strfunc.cc
  sys_vars.cc
  sys_vars_resource_mgr.cc
  table.cc
  table_cache.cc
  table_trigger_dispatcher.cc
  tc_log.cc
  thr_malloc.cc 
  transaction.cc
  transaction_info.cc
  trigger.cc
  trigger_creation_ctx.cc
  trigger_loader.cc
  trigger_chain.cc
  tztime.cc
  uniques.cc
  unireg.cc
  xa.cc
)

SET(SQL_EXPORTED_SOURCES ${SQL_SHARED_SOURCES} PARENT_SCOPE)

SET(SQL_SOURCE
  ${GEN_SOURCES}
  ${GEN_DIGEST_SOURCES}
  ${CONF_SOURCES}
  ${SQL_SHARED_SOURCES}
  ${BUNDLED_LZ4_PATH}/xxhash.c
  ../libmysql/errmsg.c
  ../sql-common/client.c
  ../sql-common/client_plugin.c
  ../sql-common/get_password.c
  ../sql-common/my_path_permissions.cc
  ../sql-common/my_time.c
  ../sql-common/my_user.c 
  ../sql-common/pack.c
  ../sql-common/client_authentication.cc
  ../sql-common/sql_string.cc 
  command_service.cc
  conn_handler/channel_info.cc
  conn_handler/connection_handler_per_thread.cc
  conn_handler/connection_handler_one_thread.cc
  conn_handler/socket_connection.cc
  conn_handler/init_net_server_extension.cc
  des_key_file.cc
  event_data_objects.cc
  event_db_repository.cc 
  event_parse_data.cc
  event_queue.cc
  event_scheduler.cc
  events.cc
  mf_iocache.cc
  mysqld.cc
  mysqld_thd_manager.cc
  protocol_callback.cc
  signal_handler.cc
  sql_audit.cc
  sql_client.cc
  srv_session.cc
  srv_session_info_service.cc
  srv_session_service.cc
  auth/sha2_password_common.cc
  )

DOWNGRADE_OPENSSL3_DEPRECATION_WARNINGS(
  ${CMAKE_SOURCE_DIR}/sql-common/client_authentication.cc
  ${CMAKE_SOURCE_DIR}/sql/auth/auth_common.h
  ${CMAKE_SOURCE_DIR}/sql/auth/sql_authentication.cc
  ${CMAKE_SOURCE_DIR}/sql/auth/sql_authentication.h
  ${CMAKE_SOURCE_DIR}/sql/des_key_file.cc
  ${CMAKE_SOURCE_DIR}/sql/item_strfunc.cc)

IF(NOT WIN32)
  LIST(APPEND SQL_SOURCE mysqld_daemon.cc)
ENDIF()

IF(WIN32)
  LIST(APPEND SQL_SOURCE
              named_pipe.cc
              conn_handler/named_pipe_connection.cc
              conn_handler/shared_memory_connection.cc
      )
ENDIF()

INCLUDE(${MYSQL_CMAKE_SCRIPT_DIR}/compile_flags.cmake)
INCLUDE_DIRECTORIES(SYSTEM
  ${BOOST_PATCHES_DIR}
  ${BOOST_INCLUDE_DIR}
  )

ADD_COMPILE_FLAGS(
  ${BUNDLED_LZ4_PATH}/xxhash.c
  COMPILE_FLAGS -DXXH_NAMESPACE=MY_ -I${BUNDLED_LZ4_PATH}
)

ADD_COMPILE_FLAGS(
  rpl_write_set_handler.cc
  COMPILE_FLAGS -I${CMAKE_SOURCE_DIR}/extra/lz4 -I${BUNDLED_LZ4_PATH}
  )

# Fixes "C1128: number of sections exceeded object file format limit" in MSVC
IF(WIN32 AND CMAKE_SIZEOF_VOID_P MATCHES 8)
  ADD_COMPILE_FLAGS(item_geofunc_setops.cc COMPILE_FLAGS "/bigobj")
ENDIF()

# Boost source has unused local typedefs.
MY_CHECK_CXX_COMPILER_FLAG("-Wno-unused-local-typedefs" HAVE_NO_UNUSED_TYPEDEFS)
IF(HAVE_NO_UNUSED_TYPEDEFS)
  ADD_COMPILE_FLAGS(
    debug_sync.cc
    handler.cc
    geometry_rtree.cc
    item_geofunc.cc
    item_geofunc_buffer.cc
    item_geofunc_internal.cc
    item_geofunc_relchecks.cc
    item_geofunc_relchecks_bgwrap.cc
    item_geofunc_setops.cc
    item_json_func.cc
    spatial.cc
    COMPILE_FLAGS "-Wno-unused-local-typedefs"
  )
ENDIF()

# Segfault in boost::geometry code at high optimization levels
IF(CMAKE_CXX_COMPILER_ID MATCHES "SunPro")
  STRING(TOUPPER "${CMAKE_BUILD_TYPE}" CMAKEBT)
  IF(CMAKEBT MATCHES "REL")
    ADD_COMPILE_FLAGS(item_geofunc_buffer.cc COMPILE_FLAGS "-xO2")
  ENDIF()
ENDIF()

# MD5_Init() MD5_Update() MD5_Final() are all deprecated
IF(SOLARIS AND CMAKE_COMPILER_IS_GNUCXX)
  ADD_COMPILE_FLAGS(
    des_key_file.cc
    item_strfunc.cc
    COMPILE_FLAGS "-Wno-deprecated-declarations")
ENDIF()

# Boost source has misleading indentation
MY_CHECK_CXX_COMPILER_FLAG("-Wmisleading-indentation"
                           HAVE_MISLEADING_INDENTATION)
IF(HAVE_MISLEADING_INDENTATION)
  ADD_COMPILE_FLAGS(
    geometry_rtree.cc
    item_geofunc.cc
    item_geofunc_buffer.cc
    item_geofunc_internal.cc
    item_geofunc_relchecks.cc
    item_geofunc_relchecks_bgwrap.cc
    item_geofunc_setops.cc    
    COMPILE_FLAGS "-Wno-misleading-indentation")
ENDIF()

# gcc 7.2.0 complains:
# boost/geometry/algorithms/detail/expand/indexed.hpp:81
# *((void*)& box +24) may be used uninitialized
MY_CHECK_CXX_COMPILER_FLAG("-Wmaybe-uninitialized" HAVE_MAYBE_UNINITIALIZED)
IF(HAVE_MAYBE_UNINITIALIZED)
  ADD_COMPILE_FLAGS(
    geometry_rtree.cc
    COMPILE_FLAGS "-Wno-maybe-uninitialized")
ENDIF()

# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=98753
IF(MY_COMPILER_IS_GNU AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 11)
  ADD_COMPILE_FLAGS(
    ${CMAKE_CURRENT_BINARY_DIR}/sql_yacc.cc
    ${CMAKE_CURRENT_BINARY_DIR}/sql_hints.yy.cc
    COMPILE_FLAGS "-Wno-free-nonheap-object -Wno-unused-label")
ENDIF()

MY_CHECK_CXX_COMPILER_FLAG("-fno-builtin-memcmp" HAVE_NO_BUILTIN_MEMCMP)
# See comments in filesort_compare-t.cc about __builtin_memcmp
IF(HAVE_NO_BUILTIN_MEMCMP)
  ADD_COMPILE_FLAGS(
    filesort_utils.cc
    COMPILE_FLAGS "-fno-builtin-memcmp"
  )
ENDIF()

IF (WIN32 AND OPENSSL_APPLINK_C)
  ADD_COMPILE_FLAGS(
    ../sql-common/client_authentication.cc
    COMPILE_FLAGS "-DHAVE_OPENSSL_APPLINK_C"
  )
ENDIF()

ADD_CONVENIENCE_LIBRARY(sql ${SQL_SOURCE})
ADD_DEPENDENCIES(sql GenServerSource)
ADD_DEPENDENCIES(sql GenDigestServerSource)
ADD_DEPENDENCIES(sql GenBootstrapPriv)
ADD_DEPENDENCIES(sql GenSysSchema)
DTRACE_INSTRUMENT(sql)
TARGET_LINK_LIBRARIES(sql ${MYSQLD_STATIC_PLUGIN_LIBS} 
  mysys mysys_ssl dbug strings vio regex binlogevents_static
  ${LIBWRAP} ${LIBCRYPT} ${LIBDL}
  ${SSL_LIBRARIES})

#
# On Windows platform we compile in the clinet-side Windows Native Authentication
# plugin which is used by the client connection code included in the server.
#
IF(WIN32)
  ADD_DEFINITIONS(-DAUTHENTICATION_WIN)
  TARGET_LINK_LIBRARIES(sql auth_win_client)
ENDIF()

SET (BINLOG_SOURCE rpl_gtid_misc.cc
                   rpl_gtid_sid_map.cc rpl_gtid_set.cc rpl_gtid_specification.cc
                   rpl_gtid_state.cc rpl_gtid_owned.cc rpl_gtid_execution.cc
                   rpl_gtid_mutex_cond_array.cc rpl_gtid_persist.cc
                   log_event.cc log_event_old.cc binlog.cc sql_binlog.cc
                   rpl_filter.cc rpl_record.cc rpl_record_old.cc rpl_trx_tracking.cc
                   rpl_utility.cc rpl_injector.cc rpl_table_access.cc)
ADD_CONVENIENCE_LIBRARY(binlog ${BINLOG_SOURCE})
TARGET_LINK_LIBRARIES(binlog binlogevents_static)

SET (RPL_SOURCE rpl_handler.cc rpl_tblmap.cc rpl_context.cc)
ADD_DEPENDENCIES(binlog GenError)
ADD_CONVENIENCE_LIBRARY(rpl ${RPL_SOURCE})
SET (MASTER_SOURCE rpl_master.cc rpl_binlog_sender.cc)
ADD_DEPENDENCIES(rpl GenError)
ADD_CONVENIENCE_LIBRARY(master ${MASTER_SOURCE})
ADD_DEPENDENCIES(master GenError)
SET (SLAVE_SOURCE rpl_slave.cc rpl_reporting.cc rpl_mi.cc rpl_rli.cc
		  rpl_info_handler.cc rpl_info_file.cc
                  rpl_info_table.cc rpl_info_values.cc rpl_info.cc
                  rpl_info_factory.cc rpl_info_table_access.cc dynamic_ids.cc
                  rpl_rli_pdb.cc rpl_info_dummy.cc rpl_mts_submode.cc
                  rpl_slave_commit_order_manager.cc rpl_msr.cc
                  rpl_trx_boundary_parser.cc rpl_channel_service_interface.cc)
ADD_CONVENIENCE_LIBRARY(slave ${SLAVE_SOURCE})
ADD_DEPENDENCIES(slave GenError)

######################### GUnit Lib #################################

SET(SQL_GUNIT_LIB_SOURCE 
  filesort_utils.cc 
  mdl.cc 
  sql_list.cc 
  ../sql-common/sql_string.cc
  thr_malloc.cc
  locks/shared_spin_lock.cc
  )

ADD_CONVENIENCE_LIBRARY(sqlgunitlib ${SQL_GUNIT_LIB_SOURCE})
ADD_DEPENDENCIES(sqlgunitlib GenError)

######################### GUnit Lib #################################


IF(WIN32)
  SET(MYSQLD_SOURCE main.cc nt_servc.cc nt_servc.h message.rc)
ELSE()
  SET(MYSQLD_SOURCE main.cc ${DTRACE_PROBES_ALL})
ENDIF()

OPTION(DEBUG_EXTNAME "Build server as mysqld-debug (debug builds only)" OFF)
MARK_AS_ADVANCED(DEBUG_EXTNAME)

IF(DEBUG_EXTNAME)
ENDIF()

IF(APPLE) 
  # Add CoreServices framework since some dloadable plugins may need it 
  FIND_LIBRARY(CORESERVICES NAMES CoreServices) 
  IF(CORESERVICES) 
  ENDIF() 
ENDIF() 

IF(NOT WITHOUT_DYNAMIC_PLUGINS)
  IF(NOT mysqld_link_flags)
    SET(mysqld_link_flags)
  ENDIF()
  IF (MINGW)
  ENDIF()
  IF(MSVC)
    # Set module definition file.
    # Also use non-incremental linker, incremental appears to crash from
    # time to time,if used with /DEF option
    SET_TARGET_PROPERTIES(mysqld PROPERTIES LINK_FLAGS
      "${mysqld_link_flags} /DEF:${CMAKE_CURRENT_BINARY_DIR}/mysqld.def /INCREMENTAL:NO")

    SET(_PLATFORM x86)
    IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
      SET(_PLATFORM  x64)
    ENDIF()
    ADD_CUSTOM_COMMAND(TARGET mysqld PRE_LINK
      COMMAND echo ${_PLATFORM} && cscript ARGS //nologo ${PROJECT_SOURCE_DIR}/win/create_def_file.js
      ${_PLATFORM}
      "$<TARGET_FILE:binlog>"
      "$<TARGET_FILE:binlogevents_static>"
      "$<TARGET_FILE:dbug>"
      "$<TARGET_FILE:mysys>"
      "$<TARGET_FILE:mysys_ssl>"
      "$<TARGET_FILE:regex>"
      "$<TARGET_FILE:slave>"
      "$<TARGET_FILE:sql>"
      "$<TARGET_FILE:strings>"
      "$<TARGET_FILE:vio>"
      > mysqld.def
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    ADD_DEPENDENCIES(sql GenError)
  ENDIF()
ENDIF()

# Plugins may need more symbols than the server does.
IF(UNIX AND STATIC_SSL_LIBRARY)
  IF(APPLE)
    TARGET_LINK_LIBRARIES(mysqld
      -Wl,-force_load ${CRYPTO_LIBRARY}
      -Wl,-force_load ${OPENSSL_LIBRARY}
      )
  ELSE()
    # OpenSSL 3.0.8 has an issue with duplicate symbols between static crypto
    # and ssl libraries. See https://github.com/openssl/openssl/issues/20238
    IF("${OPENSSL_VERSION}" VERSION_EQUAL "3.0.8")
      TARGET_LINK_LIBRARIES(mysqld
        -Wl,--whole-archive,--allow-multiple-definition ${SSL_LIBRARIES}
        -Wl,--no-whole-archive
        )
    ELSE()
      TARGET_LINK_LIBRARIES(mysqld
        -Wl,--whole-archive ${SSL_LIBRARIES} -Wl,--no-whole-archive
        )
    ENDIF()
  ENDIF()
ENDIF()

# Provide plugins with minimal set of libraries
SET(INTERFACE_LIBS ${LIBRT})
IF(INTERFACE_LIBS)
ENDIF()

# On Solaris, some extra effort is required in order to get dtrace probes
# from static libraries
 

SET(WITH_MYSQLD_LDFLAGS "" CACHE STRING "Additional linker flags for mysqld")
MARK_AS_ADVANCED(WITH_MYSQLD_LDFLAGS)
IF(WITH_MYSQLD_LDFLAGS)
  IF(NOT MYSQLD_LINK_FLAGS)
    SET(MYSQLD_LINK_FLAGS) 
  ENDIF() 
  SET_TARGET_PROPERTIES(mysqld PROPERTIES LINK_FLAGS 
     "${MYSQLD_LINK_FLAGS} ${WITH_MYSQLD_LDFLAGS}")
ENDIF()

# See DEBUG_EXTNAME above: mysqld may already have name mysqld-debug,
# and the rename is a no-op. If it is called 'mysqld' we rename it.
INSTALL_DEBUG_TARGET(mysqld
  DESTINATION ${INSTALL_SBINDIR}
  RENAME mysqld-debug${CMAKE_EXECUTABLE_SUFFIX}
)

# Handle out-of-source build from source package with possibly broken 
# bison. Copy bison output to from source to build directory, if not already 
# there
INCLUDE(${CMAKE_SOURCE_DIR}/cmake/bison.cmake)
COPY_BISON_OUTPUT(
  ${CMAKE_CURRENT_SOURCE_DIR}/sql_yacc.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/sql_yacc.h
  ${CMAKE_CURRENT_BINARY_DIR}/sql_yacc.cc
  ${CMAKE_CURRENT_BINARY_DIR}/sql_yacc.h
)

COPY_BISON_OUTPUT(
  ${CMAKE_CURRENT_SOURCE_DIR}/sql_hints.yy.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/sql_hints.yy.h
  ${CMAKE_CURRENT_BINARY_DIR}/sql_hints.yy.cc
  ${CMAKE_CURRENT_BINARY_DIR}/sql_hints.yy.h
)

RUN_BISON(
  ${CMAKE_CURRENT_SOURCE_DIR}/sql_yacc.yy 
  ${CMAKE_CURRENT_BINARY_DIR}/sql_yacc.cc
  ${CMAKE_CURRENT_BINARY_DIR}/sql_yacc.h
  MYSQL
)

RUN_BISON(
  ${CMAKE_CURRENT_SOURCE_DIR}/sql_hints.yy 
  ${CMAKE_CURRENT_BINARY_DIR}/sql_hints.yy.cc
  ${CMAKE_CURRENT_BINARY_DIR}/sql_hints.yy.h
  HINT_PARSER_
)

SET_SOURCE_FILES_PROPERTIES(
  ${CMAKE_CURRENT_BINARY_DIR}/sql_yacc.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/sql_digest.cc
  PROPERTIES OBJECT_DEPENDS ${GEN_DIGEST_SOURCES}
)

# Gen_lex_hash
ADD_EXECUTABLE(gen_lex_hash gen_lex_hash.cc)

ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/lex_hash.h
  COMMAND gen_lex_hash > lex_hash.h
  DEPENDS gen_lex_hash
)

ADD_CUSTOM_TARGET( 
        GenServerSource
        DEPENDS ${GEN_SOURCES}
)

ADD_CUSTOM_TARGET( 
        GenDigestServerSource
        DEPENDS ${GEN_DIGEST_SOURCES}
)


IF(WIN32 OR HAVE_DLOPEN AND NOT DISABLE_SHARED)
ENDIF()

FOREACH(tool gtar tar git)
  STRING(TOUPPER ${tool}  TOOL)
  FIND_PROGRAM(${TOOL}_EXECUTABLE ${tool} DOC "path to the executable")
  MARK_AS_ADVANCED(${TOOL}_EXECUTABLE)
ENDFOREACH()

CONFIGURE_FILE(
  ${CMAKE_SOURCE_DIR}/cmake/make_dist.cmake.in
  ${CMAKE_BINARY_DIR}/make_dist.cmake @ONLY)

ADD_CUSTOM_TARGET(dist 
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/make_dist.cmake
  DEPENDS ${CMAKE_BINARY_DIR}/sql/sql_yacc.cc ${CMAKE_BINARY_DIR}/sql/sql_yacc.h ${CMAKE_BINARY_DIR}/sql/sql_hints.yy.cc ${CMAKE_BINARY_DIR}/sql/sql_hints.yy.h
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

IF(WITH_MAN_PAGES)
  ADD_DEPENDENCIES(dist man)
ENDIF()

ADD_CUSTOM_TARGET(distclean
  COMMAND ${CMAKE_COMMAND} -E echo  WARNING: distclean target is not functional
  COMMAND ${CMAKE_COMMAND} -E echo  Use 'git clean -fdx' instead
  VERBATIM
  )

ADD_CUSTOM_TARGET(show-dist-name
  COMMAND ${CMAKE_COMMAND} -E echo "${CPACK_PACKAGE_FILE_NAME}"
)
