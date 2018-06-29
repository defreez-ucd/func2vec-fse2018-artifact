#include "TraceDatabase.hpp"
#include <iostream>
#include <sstream>

using namespace std;

TraceDatabase::TraceDatabase(string path) {
  int err = sqlite3_open(path.c_str(), &db);
  if (err) {
    cerr << "FATAL ERROR: Unable to open/create database file" << endl;
    abort();
  }

  if (!isInitialized()) {
    initialize();
  }

  sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);
}

TraceDatabase::~TraceDatabase() {
  sqlite3_exec(db, "COMMIT TRANSACTION", NULL, NULL, NULL);
  sqlite3_close(db);
}

void TraceDatabase::addTraceActions(const sqlite3_int64 row_id, const Trace &trace) {
  int err = 0;
  char *errMsg = nullptr;

  for (const Item& i : trace.contexts) {
    ostringstream item_query;
    item_query << "INSERT INTO Context (handler, item, type, tactic) VALUES ("
               << row_id  << ","
               << "'" << i.name <<  "',"
               << "'" << i.getType() << "',"
               << "'" << i.tactic << "'"
               << ");";

    err = sqlite3_exec(db, item_query.str().c_str(), NULL, NULL, NULL);
    checkResult(err, errMsg);
  }

  for (const Item& i : trace.items) {
    ostringstream item_query;
    item_query << "INSERT INTO Response (handler, item, type, tactic) VALUES ("
               << row_id  << ","
               << "'" << i.name <<  "',"
               << "'" << i.getType() << "',"
               << "'" << i.tactic << "'"
               << ");";

    err = sqlite3_exec(db, item_query.str().c_str(), NULL, NULL, NULL);
    checkResult(err, errMsg);
  }
}

// TODO: Filter queries for injection
sqlite3_int64 TraceDatabase::addHandlerTrace(const Trace &trace) {
  ostringstream query;
  query << "INSERT INTO Handler (stack, predicate_loc, parent_function) VALUES ('"
        << trace.stack_id << "','"
        << trace.location << "','"
        << trace.parent_function << "');";
  char *errMsg = nullptr;
  int err = sqlite3_exec(db, query.str().c_str(), NULL, NULL, NULL);
  checkResult(err, errMsg);

  sqlite3_int64 trace_rowid = sqlite3_last_insert_rowid(db);
  addTraceActions(trace_rowid, trace);
  return trace_rowid;
}

void TraceDatabase::addPreActionTrace(const sqlite3_int64 handler_id, const PreActionTrace &trace) {
  addTraceActions(handler_id, trace);
}

void TraceDatabase::addPostActionTrace(const sqlite3_int64 handler_id, const PostActionTrace &trace) {
  addTraceActions(handler_id, trace);
}

void TraceDatabase::checkResult(int err, char *errMsg) {
  if (err != SQLITE_OK) {
    cerr << sqlite3_errmsg(db) << endl;
    sqlite3_close(db);
    abort();
  }
  sqlite3_free(errMsg);
}

void TraceDatabase::initialize() {
  cout << "Initializing database..." << endl;

  string query = "CREATE TABLE Handler(id INTEGER PRIMARY KEY, stack TEXT, predicate_loc TEXT, parent_function TEXT);";
  query += "CREATE TABLE Context(handler INTEGER, item TEXT, type TEXT, tactic TEXT);";
  query += "CREATE TABLE Response(handler INTEGER, item TEXT, type TEXT, tactic TEXT);";
  query += "CREATE TABLE Nesting(parent_handler INTEGER, child_handler INTEGER, PRIMARY KEY(parent_handler, child_handler));";

  char *errMsg = nullptr;
  int err = sqlite3_exec(db, query.c_str(), NULL, NULL, NULL);
  checkResult(err, errMsg);
}

bool TraceDatabase::isInitialized() {
  // We just check to see if the trace table exists.
  // This doesn't handle situations where the schema is corrupt / old
  string query = "SELECT name FROM sqlite_master WHERE type='table' and name='Trace';";
  bool initialized = false;
  int (*callback)(void*, int, char**, char**) =
    [](void* initialized, int, char**, char**) {
      *(bool*) initialized = true;
      return 0;
    };

  char *errMsg = nullptr;
  int err = sqlite3_exec(db, query.c_str(), callback, &initialized, &errMsg);
  checkResult(err, errMsg);

  if (err != SQLITE_OK) {
    cerr << "Error while checking for db initialization: " << sqlite3_errmsg(db) << endl;
    sqlite3_free(errMsg);
    sqlite3_close(db);
    abort();
  }

  return initialized;
}
