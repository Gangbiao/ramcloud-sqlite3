#include "sqlite3.h"
#include "vfs-ramcloud.h"

#include <inttypes.h>
#include <assert.h>
#include <stdio.h>

#define ROWS 100000

int main(int argc, char **argv) {
  int retval;

  char *dbname = "./my-db";
  if (argc > 1)
    dbname = argv[1];

  //sqlite3_vfs *vfs = sqlite3_vfs_find("unix");
  //assert(vfs != NULL);
  //retval = sqlite3_vfs_register(vfs, 1);
  SQLITE_RCVFS_CONNECTION *conn =
    sqlite3_rcvfs_connect("zk:localhost:2181", "main", "sqlite3");
  assert(conn != NULL);
  retval = sqlite3_vfs_register(sqlite3_rcvfs("ramcloud", conn), 1);
  assert(retval == 0);

  sqlite3 *db;
  retval = sqlite3_open_v2(dbname, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
  assert(retval == 0);
  printf("file opened\n");

  retval = sqlite3_exec(db, "PRAGMA journal_mode=truncate;", NULL, NULL, NULL);
  printf("retval is %d\n", retval);
  assert(retval == 0);

  retval = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS test (value INTEGER);", NULL, NULL, NULL);
  printf("retval is %d\n", retval);
  assert(retval == 0);

  retval = sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);
  assert(retval == 0);
  sqlite3_stmt *sql_insert;
  retval = sqlite3_prepare(db, "INSERT INTO test (value) VALUES (:v)", -1, &sql_insert, NULL);
  assert(retval == 0);
  for (unsigned i = 0; i < ROWS; ++i) {
    retval = sqlite3_bind_int64(sql_insert, 1, i);
    assert(retval == 0);
    retval = sqlite3_step(sql_insert);
    assert(retval == SQLITE_DONE);
    retval = sqlite3_reset(sql_insert);
    assert(retval == 0);
  }
  retval = sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
  assert(retval == 0);

  retval = sqlite3_close_v2(db);
  assert(retval == 0);

  retval = sqlite3_open_v2(dbname, &db, SQLITE_OPEN_READONLY, NULL);
  assert(retval == 0);

  sqlite3_stmt *sql_sum;
  retval = sqlite3_prepare(db, "SELECT SUM(value) FROM test;", -1, &sql_sum, NULL);
  assert(retval == 0);
  retval = sqlite3_step(sql_sum);
  assert(retval = SQLITE_ROW);
  uint64_t sum = sqlite3_column_int64(sql_sum, 0);
  retval = sqlite3_reset(sql_sum);
  assert(retval == 0);

  retval = sqlite3_close_v2(db);
  assert(retval == 0);

  sqlite3_rcvfs_disconnect(conn);

  printf("sum is %lu\n", sum);
  return 0;
}

