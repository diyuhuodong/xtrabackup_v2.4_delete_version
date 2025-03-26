###############################################################################
# Test history-on-server feature
###############################################################################


###############################################################################
# Gets a single column value from the last history record added
function get_one_value()
{
    local column=$1
    val=`${MYSQL} ${MYSQL_ARGS} -Ns -e "SELECT $column FROM PERCONA_SCHEMA.xtrabackup_history ORDER BY start_time DESC LIMIT 1"`
}


###############################################################################
# Checks a single column from the last history record added for some value and
# not NULL.
function check_for_not_NULL()
{
    local column=$1
    get_one_value "$column"
    if [ -z "$val" ] || [ "$val" = "NULL" ];
    then
        vlog "Error: $column in history record invalid, expected NOT NULL, got \"$val\""
        exit 1
    fi
}


###############################################################################
# Checks a single column from the last history record added to see if is a
# specific value.
function check_for_value()
{
    local column=$1
    shift
    get_one_value "$column"
    if [ -z "$val" ] || [ "$val" != "$@" ];
    then
        vlog "Error: $column in history record invalid, got \"$val\" expected \"$@\""
        exit 1
    fi
}

###############################################################################
# Checks the number of records to see if is a specific value.
function check_count()
{
  local expected=$1
  local val=`${MYSQL} ${MYSQL_ARGS} -Ns -e "SELECT COUNT(*) FROM PERCONA_SCHEMA.xtrabackup_history"`
  if [ -z "$val" ] || [ "$val" != "$expected" ];
  then
      vlog "Error: count in history is invalid, got \"$val\" expected \"$expected\""
      exit 1
  fi
}

###############################################################################
vlog "Prepping server"
start_server
load_dbase_schema incremental_sample
multi_row_insert incremental_sample.test \({1..100},100\)
backup_dir=$topdir/backups
mkdir $backup_dir


###############################################################################
# This tests the to make sure that no xtrabackup_history unless --history is
# specified
#vlog "Testing no --history"
#innobackupex --stream=tar $backup_dir > /dev/null

#run_cmd_expect_failure get_one_value "uuid"


###############################################################################
# This tests the basic creation of a history record and that fields are
# populated with some data. It also tests specifically that
# partial, incremental, compact, compressed, encrypted and format are exactly
# the correct values after the backup.
# Missing is a test that binlog_pos is NULL, that would require restarting the
# server without the log-bin option in the .cnf file but that has been tested
# manually and doesn't seem to be something that would be required to be
# validated.
vlog "Testing basic history record"
innobackupex --history=test1 --stream=tar $backup_dir > /dev/null

for column in uuid name tool_name tool_command tool_version ibbackup_version \
server_version start_time end_time lock_time binlog_pos innodb_from_lsn \
innodb_to_lsn
do
    check_for_not_NULL "$column"
done

for column in partial incremental compact compressed encrypted
do
    check_for_value "$column" "N"
done

get_one_value "lock_time"
lock_time_without_lock=$val

if [ $val -lt 0 ];
then	
    vlog "Error: lock_time in history record invalid, expected > 0, got \"$val\""
    exit 1
fi

check_for_value "format" "tar"

# saving for later
get_one_value "innodb_to_lsn"
first_to_lsn=$val


###############################################################################
# This tests the taking of an incremental backup based on the last record
# of a history series and validates that the lsns in the record are correct.
# It also tests that format, incremental and compact are exactly the correct
# values after the backup.
vlog "Testing incremental based on history name"

multi_row_insert incremental_sample.test \({101..200},100\)

innobackupex --history=test1 --incremental \
--incremental-history-name=test1 --no-lock $backup_dir > /dev/null

# saving for later
get_one_value "uuid"
second_uuid=$val
get_one_value "innodb_from_lsn"
second_from_lsn=$val
get_one_value "innodb_to_lsn"
second_to_lsn=$val

check_for_value "lock_time" "0"

check_for_value "format" "file"
check_for_value "incremental" "Y"
check_for_value "compact" "N"

if [ -z "$second_from_lsn" ] || [ "$second_from_lsn" != "$first_to_lsn" ]
then
    vlog "Second backup was not properly based on the to_lsn of the first"
    exit 1
fi

multi_row_insert incremental_sample.test \({201..300},100\)

# This will be a backup based on the last incremental just done, so, its
# innodb_from_lsn (third_from_lsn) should be the same as the value in 
# second_to_lsn. This tests that we find the right record in the test1 series
# out of the two records that should be present before the backup is done.
innobackupex --history=test1 --incremental \
--incremental-history-name=test1 $backup_dir > /dev/null

# saving for later
get_one_value "uuid"
third_uuid=$val
get_one_value "innodb_from_lsn"
third_from_lsn=$val
get_one_value "innodb_to_lsn"
third_to_lsn=$val

if [ -z "$third_from_lsn" ] || [ "$third_from_lsn" != "$second_to_lsn" ]
then
    vlog "Third backup was not properly based on the to_lsn of the second"
    exit 1
fi


###############################################################################
# This tests that we can base an incremental on a specific history record
# identified by its uuid that we captured earlier from a history record or it
# could be scraped from the output of innobackupex at some point in the past.
# It also tests specifically that incremental, compressed, encrypted and format
# are exactly the correct values after the backup.
# It tests that --history can be specified, resulting in a history record with
# no name
vlog "Testing incremental based on history uuid"
multi_row_insert incremental_sample.test \({301..400},100\)

innobackupex --history --incremental --incremental-history-uuid=$third_uuid \
--stream=xbstream --compress --encrypt=AES256 \
--encrypt-key=percona_xtrabackup_is_awesome___ --transition-key=percona \
$backup_dir > /dev/null

get_one_value "innodb_from_lsn"
fourth_from_lsn=$val

for column in incremental compressed encrypted
do
    check_for_value "$column" "Y"
done

check_for_value "format" "xbstream"
check_for_value "name" "NULL"

# validate command tool and encrypt key scrubbibng but need to pop off first
# three arguments in the result added by test framework function innobackupex
get_one_value "tool_command"
val=`set -- $val; shift 2; echo "$@"`
expected_val="--history --incremental "\
"--incremental-history-uuid=$third_uuid --stream=xbstream --compress "\
"--encrypt=AES256 --encrypt-key=... --transition-key=... $backup_dir"

if [ -z "$val" ] || [ "$val" != "$expected_val" ]
then
  vlog "Error: tool_command in history record invalid, got \"$val\" expected \"$expected_val\""
  exit 1
fi

if [ -z "$fourth_from_lsn" ] || [ "$fourth_from_lsn" != "$third_to_lsn" ]
then
    vlog "Fourth backup was not properly based on the to_lsn of the third"
    exit 1
fi


###############################################################################
# This tests that innobackupex fails when an invalid --incremental-history-name
# is given.
vlog "Testing bad --incremental-history-name"
run_cmd_expect_failure $IB_BIN $IB_ARGS --incremental \
--incremental-history-name=foo --stream=tar $backup_dir > /dev/null



###############################################################################
# This tests that innobackupex fails when an invalid --incremental-history-uuid
# is given.
vlog "Testing bad --incremental-history-uuid"
run_cmd_expect_failure $IB_BIN $IB_ARGS --incremental \
--incremental-history-uuid=foo --stream=tar $backup_dir > /dev/null

###############################################################################
# PXB-1462 - long gtid_executed breaks --history functionality
. inc/common.sh
if is_server_version_higher_than 5.6.0
then
  vlog "Testing PXB-1462"
  # make sure we don't have dirty pages before enabling GTID
  innodb_wait_for_flush_all
  stop_server
  start_server --server-id=1 --enforce-gtid-consistency --gtid-mode=ON --log-bin --log-slave-updates
  ${MYSQL} ${MYSQL_ARGS} -Ns -e "RESET MASTER"
  ${MYSQL} ${MYSQL_ARGS} -Ns -e "SET GLOBAL gtid_purged='aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa:1,aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaab:1,aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaac:1,aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaad:1,aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaae:1,aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaf:1,aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaa0:1,aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaa1:1,aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaa2:1,aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaa3:1,aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaa4:1,aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaa5:1,aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaa6:1,aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaa7:1,aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaa8:1,aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaa9:1,ffffffff-ffff-ffff-ffff-ffffffffffff:1'"
  vlog "Testing upgrade"
  ${MYSQL} ${MYSQL_ARGS} -Ns -e "DROP TABLE IF EXISTS PERCONA_SCHEMA.xtrabackup_history"
  ${MYSQL} ${MYSQL_ARGS} -Ns -e "CREATE TABLE IF NOT EXISTS PERCONA_SCHEMA.xtrabackup_history(
  uuid VARCHAR(40) NOT NULL PRIMARY KEY,
  name VARCHAR(255) DEFAULT NULL,
  tool_name VARCHAR(255) DEFAULT NULL,
  tool_command TEXT DEFAULT NULL,
  tool_version VARCHAR(255) DEFAULT NULL,
  ibbackup_version VARCHAR(255) DEFAULT NULL,
  server_version VARCHAR(255) DEFAULT NULL,
  start_time TIMESTAMP NULL DEFAULT NULL,
  end_time TIMESTAMP NULL DEFAULT NULL,
  lock_time BIGINT UNSIGNED DEFAULT NULL,
  binlog_pos varchar(128) DEFAULT NULL,
  innodb_from_lsn BIGINT UNSIGNED DEFAULT NULL,
  innodb_to_lsn BIGINT UNSIGNED DEFAULT NULL,
  partial ENUM('Y', 'N') DEFAULT NULL,
  incremental ENUM('Y', 'N') DEFAULT NULL,
  format ENUM('file', 'tar', 'xbstream') DEFAULT NULL,
  compact ENUM('Y', 'N') DEFAULT NULL,
  compressed ENUM('Y', 'N') DEFAULT NULL,
  encrypted ENUM('Y', 'N') DEFAULT NULL
  ) CHARACTER SET utf8 ENGINE=innodb"
  ${MYSQL} ${MYSQL_ARGS} -Ns -e "INSERT INTO \`PERCONA_SCHEMA\`.\`xtrabackup_history\` VALUES ('1bc0b0cb-9dec-11eb-bfc3-d45d64347a19',NULL,'xtrabackup','--defaults-file=/work/pxb/ins/2.4/xtrabackup-test/var/w1/var1/my.cnf --no-version-check --backup --history --target-dir=/work/pxb/ins/2.4/xtrabackup-test/var/w1/var1/backup0','2.4.21','2.4.21','5.7.31-34-debug-log','2021-04-15 10:11:34','2021-04-15 10:11:36',0,'filename \'mysql-bin.000001\', position \'1424\', GTID of the last change \'12c397b9-9dec-11eb-abcb-d45d64347a19:1-2\'',0,2789167,'N','N','file','N','N','N')"
  xtrabackup --backup --history --target-dir=$topdir/backup0
  check_count 2
  check_for_value "SUBSTRING(binlog_pos, -39)" "ffffffff-ffff-ffff-ffff-ffffffffffff:1'"
  get_one_value "char_length(binlog_pos)"
  if [ -z "$val" ] || [ "$val" -le "128" ]
  then
    vlog "Data truncated at binlog_pos"
    vlog "len returned: ${val}"
    exit 1
  fi
  # Delete newly created record to check record inserted previous update
  ${MYSQL} ${MYSQL_ARGS} -Ns -e "DELETE FROM \`PERCONA_SCHEMA\`.\`xtrabackup_history\` ORDER BY start_time DESC LIMIT 1"
  check_for_value "SUBSTRING(binlog_pos, -41)" "12c397b9-9dec-11eb-abcb-d45d64347a19:1-2'"

  vlog "Testing new table"
  ${MYSQL} ${MYSQL_ARGS} -Ns -e "DROP TABLE IF EXISTS PERCONA_SCHEMA.xtrabackup_history"
  xtrabackup --backup --history --target-dir=$topdir/backup1
  check_count 1
  get_one_value "char_length(binlog_pos)"
  if [ -z "$val" ] || [ "$val" -le "128" ]
  then
    vlog "Data truncated at binlog_pos"
    vlog "len returned: ${val}"
    exit 1
  fi
else
  vlog "Server does not support GTID"
fi

#clean-up
rm -rf $topdir/backup0 $topdir/backup1
