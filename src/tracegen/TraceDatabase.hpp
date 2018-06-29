#ifndef TRACEDATABASE_HPP
#define TRACEDATABASE_HPP

#include <sqlite3.h>
#include <string>
#include "Traces.hpp"

class TraceDatabase {
public:
  TraceDatabase(std::string path);
  ~TraceDatabase();

  // Returns id of trace used in database
  sqlite3_int64 addHandlerTrace(const Trace &trace);

  // Use id from addHandlerTrace for handler_id
  void addPreActionTrace(const sqlite3_int64 handler_id, const PreActionTrace &trace);
  void addPostActionTrace(const sqlite3_int64 handler_id, const PostActionTrace &trace);
  void addNested(const sqlite3_int64 parent_id, const sqlite3_int64 child_id);

private:
  sqlite3 *db = nullptr;

  // Check to see if the schema has been initialized
  bool isInitialized();

  // Create the schema
  void initialize();

  // pre is set to false for normal handler actions, true for pre-actions
  void addTraceActions(const sqlite3_int64 row_id, const Trace &trace);

  // Make sure err is SQLITE_OK
  void checkResult(int err, char* errMsg);

  TraceDatabase(const TraceDatabase &other);
  TraceDatabase& operator=(const TraceDatabase &other);
};

#endif
