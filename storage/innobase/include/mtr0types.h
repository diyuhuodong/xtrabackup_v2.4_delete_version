/*****************************************************************************

Copyright (c) 1995, 2023, Oracle and/or its affiliates.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License, version 2.0,
as published by the Free Software Foundation.

This program is also distributed with certain software (including
but not limited to OpenSSL) that is licensed under separate terms,
as designated in a particular file or component or in included license
documentation.  The authors of MySQL hereby grant you an additional
permission to link the program and your derivative works with the
separately licensed software that they have included with MySQL.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License, version 2.0, for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Suite 500, Boston, MA 02110-1335 USA

*****************************************************************************/

/**************************************************//**
@file include/mtr0types.h
Mini-transaction buffer global types

Created 11/26/1995 Heikki Tuuri
*******************************************************/

#ifndef mtr0types_h
#define mtr0types_h

#ifndef UNIV_INNOCHECKSUM
#include "sync0rw.h"
#endif /* UNIV_INNOCHECKSUM */

struct mtr_t;

/** Logging modes for a mini-transaction */
enum mtr_log_t {
	/** Default mode: log all operations modifying disk-based data */
	MTR_LOG_ALL = 21,

	/** Log no operations and dirty pages are not added to the flush list */
	MTR_LOG_NONE = 22,

	/** Don't generate REDO log but add dirty pages to flush list */
	MTR_LOG_NO_REDO = 23,

	/** Inserts are logged in a shorter form */
	MTR_LOG_SHORT_INSERTS = 24
};

/** @name Log item types
The log items are declared 'byte' so that the compiler can warn if val
and type parameters are switched in a call to mlog_write_ulint. NOTE!
For 1 - 8 bytes, the flag value must give the length also! @{ */
enum mlog_id_t {
	/** if the mtr contains only one log record for one page,
	i.e., write_initial_log_record has been called only once,
	this flag is ORed to the type of that first log record */
	MLOG_SINGLE_REC_FLAG = 128,

	/** one byte is written */
	MLOG_1BYTE = 1,

	/** 2 bytes ... */
	MLOG_2BYTES = 2,

	/** 4 bytes ... */
	MLOG_4BYTES = 4,

	/** 8 bytes ... */
	MLOG_8BYTES = 8,

	/** Record insert */
	MLOG_REC_INSERT = 9,

	/** Mark clustered index record deleted */
	MLOG_REC_CLUST_DELETE_MARK = 10,

	/** Mark secondary index record deleted */
	MLOG_REC_SEC_DELETE_MARK = 11,

	/** update of a record, preserves record field sizes */
	MLOG_REC_UPDATE_IN_PLACE = 13,

	/*!< Delete a record from a page */
	MLOG_REC_DELETE = 14,

	/** Delete record list end on index page */
	MLOG_LIST_END_DELETE = 15,

	/** Delete record list start on index page */
	MLOG_LIST_START_DELETE = 16,

	/** Copy record list end to a new created index page */
	MLOG_LIST_END_COPY_CREATED = 17,

	/** Reorganize an index page in ROW_FORMAT=REDUNDANT */
	MLOG_PAGE_REORGANIZE = 18,

	/** Create an index page */
	MLOG_PAGE_CREATE = 19,

	/** Insert entry in an undo log */
	MLOG_UNDO_INSERT = 20,

	/** erase an undo log page end */
	MLOG_UNDO_ERASE_END = 21,

	/** initialize a page in an undo log */
	MLOG_UNDO_INIT = 22,

	/** discard an update undo log header */
	MLOG_UNDO_HDR_DISCARD = 23,

	/** reuse an insert undo log header */
	MLOG_UNDO_HDR_REUSE = 24,

	/** create an undo log header */
	MLOG_UNDO_HDR_CREATE = 25,

	/** mark an index record as the predefined minimum record */
	MLOG_REC_MIN_MARK = 26,

	/** initialize an ibuf bitmap page */
	MLOG_IBUF_BITMAP_INIT = 27,

#ifdef UNIV_LOG_LSN_DEBUG
	/** Current LSN */
	MLOG_LSN = 28,
#endif /* UNIV_LOG_LSN_DEBUG */

	/** this means that a file page is taken into use and the prior
	contents of the page should be ignored: in recovery we must not
	trust the lsn values stored to the file page.
	Note: it's deprecated because it causes crash recovery problem
	in bulk create index, and actually we don't need to reset page
	lsn in recv_recover_page_func() now. */
	MLOG_INIT_FILE_PAGE = 29,

	/** write a string to a page */
	MLOG_WRITE_STRING = 30,

	/** If a single mtr writes several log records, this log
	record ends the sequence of these records */
	MLOG_MULTI_REC_END = 31,

	/** dummy log record used to pad a log block full */
	MLOG_DUMMY_RECORD = 32,

	/** log record about an .ibd file creation */
	MLOG_FILE_CREATE = 33,

	/** rename databasename/tablename (no .ibd file name suffix) */
	MLOG_FILE_RENAME = 34,

	/** delete a tablespace file that starts with (space_id,page_no) */
	MLOG_FILE_DELETE = 35,

	/** mark a compact index record as the predefined minimum record */
	MLOG_COMP_REC_MIN_MARK = 36,

	/** create a compact index page */
	MLOG_COMP_PAGE_CREATE = 37,

	/** compact record insert */
	MLOG_COMP_REC_INSERT = 38,

	/** mark compact clustered index record deleted */
	MLOG_COMP_REC_CLUST_DELETE_MARK = 39,

	/** mark compact secondary index record deleted; this log
	record type is redundant, as MLOG_REC_SEC_DELETE_MARK is
	independent of the record format. */
	MLOG_COMP_REC_SEC_DELETE_MARK = 40,

	/** update of a compact record, preserves record field sizes */
	MLOG_COMP_REC_UPDATE_IN_PLACE = 41,

	/** delete a compact record from a page */
	MLOG_COMP_REC_DELETE = 42,

	/** delete compact record list end on index page */
	MLOG_COMP_LIST_END_DELETE = 43,

	/*** delete compact record list start on index page */
	MLOG_COMP_LIST_START_DELETE = 44,

	/** copy compact record list end to a new created index page */
	MLOG_COMP_LIST_END_COPY_CREATED = 45,

	/** reorganize an index page */
	MLOG_COMP_PAGE_REORGANIZE = 46,

	/** log record about creating an .ibd file, with format */
	MLOG_FILE_CREATE2 = 47,

	/** write the node pointer of a record on a compressed
	non-leaf B-tree page */
	MLOG_ZIP_WRITE_NODE_PTR = 48,

	/** write the BLOB pointer of an externally stored column
	on a compressed page */
	MLOG_ZIP_WRITE_BLOB_PTR = 49,

	/** write to compressed page header */
	MLOG_ZIP_WRITE_HEADER = 50,

	/** compress an index page */
	MLOG_ZIP_PAGE_COMPRESS = 51,

	/** compress an index page without logging it's image */
	MLOG_ZIP_PAGE_COMPRESS_NO_DATA = 52,

	/** reorganize a compressed page */
	MLOG_ZIP_PAGE_REORGANIZE = 53,

	/** rename a tablespace file that starts with (space_id,page_no) */
	MLOG_FILE_RENAME2 = 54,

	/** note the first use of a tablespace file since checkpoint */
	MLOG_FILE_NAME = 55,

	/** note that all buffered log was written since a checkpoint */
	MLOG_CHECKPOINT = 56,

	/** Create a R-Tree index page */
	MLOG_PAGE_CREATE_RTREE = 57,

	/** create a R-tree compact page */
	MLOG_COMP_PAGE_CREATE_RTREE = 58,

	/** this means that a file page is taken into use.
	We use it to replace MLOG_INIT_FILE_PAGE. */
	MLOG_INIT_FILE_PAGE2 = 59,

	/** Table is being truncated. (Marked only for file-per-table) */
	MLOG_TRUNCATE = 60,

	/** notify that an index tree is being loaded without writing
	redo log about individual pages */
	MLOG_INDEX_LOAD = 61,

	/** biggest value (used in assertions) */
	MLOG_BIGGEST_TYPE = MLOG_INDEX_LOAD
};

/* @} */

/** @name Flags for MLOG_FILE operations
(stored in the page number parameter, called log_flags in the
functions).  The page number parameter was originally written as 0. @{ */
#define MLOG_FILE_FLAG_TEMP	1	/*!< identifies TEMPORARY TABLE in
					MLOG_FILE_CREATE, MLOG_FILE_CREATE2 */
/* @} */

/** Size of a MLOG_CHECKPOINT record in bytes.
The record consists of a MLOG_CHECKPOINT byte followed by
mach_write_to_8(checkpoint_lsn). */
#define SIZE_OF_MLOG_CHECKPOINT	9

/** Types for the mlock objects to store in the mtr memo; NOTE that the
first 3 values must be RW_S_LATCH, RW_X_LATCH, RW_NO_LATCH */
enum mtr_memo_type_t {
#ifndef UNIV_INNOCHECKSUM
	MTR_MEMO_PAGE_S_FIX = RW_S_LATCH,

	MTR_MEMO_PAGE_X_FIX = RW_X_LATCH,

	MTR_MEMO_PAGE_SX_FIX = RW_SX_LATCH,

	MTR_MEMO_BUF_FIX = RW_NO_LATCH,
#endif /* !UNIV_CHECKSUM */

#ifdef UNIV_DEBUG
	MTR_MEMO_MODIFY = 32,
#endif /* UNIV_DEBUG */

	MTR_MEMO_S_LOCK = 64,

	MTR_MEMO_X_LOCK = 128,

	MTR_MEMO_SX_LOCK = 256
};

#ifdef UNIV_DEBUG
# define MTR_MAGIC_N		54551
#endif /* UNIV_DEBUG */

enum mtr_state_t {
	MTR_STATE_INIT = 0,
	MTR_STATE_ACTIVE = 12231,
	MTR_STATE_COMMITTING = 56456,
	MTR_STATE_COMMITTED = 34676
};

#endif /* mtr0types_h */
