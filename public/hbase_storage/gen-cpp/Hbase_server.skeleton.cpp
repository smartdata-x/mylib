// This autogenerated skeleton file illustrates how to build a server.
// You should copy it to another filename to avoid overwriting it.

#include "Hbase.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

using namespace  ::apache::hadoop::hbase::thrift;

class HbaseHandler : virtual public HbaseIf {
 public:
  HbaseHandler() {
    // Your initialization goes here
  }

  /**
   * Brings a table on-line (enables it)
   * 
   * @param tableName name of the table
   */
  void enableTable(const Bytes& tableName) {
    // Your implementation goes here
    printf("enableTable\n");
  }

  /**
   * Disables a table (takes it off-line) If it is being served, the master
   * will tell the servers to stop serving it.
   * 
   * @param tableName name of the table
   */
  void disableTable(const Bytes& tableName) {
    // Your implementation goes here
    printf("disableTable\n");
  }

  /**
   * @return true if table is on-line
   * 
   * @param tableName name of the table to check
   */
  bool isTableEnabled(const Bytes& tableName) {
    // Your implementation goes here
    printf("isTableEnabled\n");
  }

  void compact(const Bytes& tableNameOrRegionName) {
    // Your implementation goes here
    printf("compact\n");
  }

  void majorCompact(const Bytes& tableNameOrRegionName) {
    // Your implementation goes here
    printf("majorCompact\n");
  }

  /**
   * List all the userspace tables.
   * 
   * @return returns a list of names
   */
  void getTableNames(std::vector<Text> & _return) {
    // Your implementation goes here
    printf("getTableNames\n");
  }

  /**
   * List all the column families assoicated with a table.
   * 
   * @return list of column family descriptors
   * 
   * @param tableName table name
   */
  void getColumnDescriptors(std::map<Text, ColumnDescriptor> & _return, const Text& tableName) {
    // Your implementation goes here
    printf("getColumnDescriptors\n");
  }

  /**
   * List the regions associated with a table.
   * 
   * @return list of region descriptors
   * 
   * @param tableName table name
   */
  void getTableRegions(std::vector<TRegionInfo> & _return, const Text& tableName) {
    // Your implementation goes here
    printf("getTableRegions\n");
  }

  /**
   * Create a table with the specified column families.  The name
   * field for each ColumnDescriptor must be set and must end in a
   * colon (:). All other fields are optional and will get default
   * values if not explicitly specified.
   * 
   * @throws IllegalArgument if an input parameter is invalid
   * 
   * @throws AlreadyExists if the table name already exists
   * 
   * @param tableName name of table to create
   * 
   * @param columnFamilies list of column family descriptors
   */
  void createTable(const Text& tableName, const std::vector<ColumnDescriptor> & columnFamilies) {
    // Your implementation goes here
    printf("createTable\n");
  }

  /**
   * Deletes a table
   * 
   * @throws IOError if table doesn't exist on server or there was some other
   * problem
   * 
   * @param tableName name of table to delete
   */
  void deleteTable(const Text& tableName) {
    // Your implementation goes here
    printf("deleteTable\n");
  }

  /**
   * Get a single TCell for the specified table, row, and column at the
   * latest timestamp. Returns an empty list if no such value exists.
   * 
   * @return value for specified row/column
   * 
   * @param tableName name of table
   * 
   * @param row row key
   * 
   * @param column column name
   * 
   * @param attributes Get attributes
   */
  void get(std::vector<TCell> & _return, const Text& tableName, const Text& row, const Text& column, const std::map<Text, Text> & attributes) {
    // Your implementation goes here
    printf("get\n");
  }

  /**
   * Get the specified number of versions for the specified table,
   * row, and column.
   * 
   * @return list of cells for specified row/column
   * 
   * @param tableName name of table
   * 
   * @param row row key
   * 
   * @param column column name
   * 
   * @param numVersions number of versions to retrieve
   * 
   * @param attributes Get attributes
   */
  void getVer(std::vector<TCell> & _return, const Text& tableName, const Text& row, const Text& column, const int32_t numVersions, const std::map<Text, Text> & attributes) {
    // Your implementation goes here
    printf("getVer\n");
  }

  /**
   * Get the specified number of versions for the specified table,
   * row, and column.  Only versions less than or equal to the specified
   * timestamp will be returned.
   * 
   * @return list of cells for specified row/column
   * 
   * @param tableName name of table
   * 
   * @param row row key
   * 
   * @param column column name
   * 
   * @param timestamp timestamp
   * 
   * @param numVersions number of versions to retrieve
   * 
   * @param attributes Get attributes
   */
  void getVerTs(std::vector<TCell> & _return, const Text& tableName, const Text& row, const Text& column, const int64_t timestamp, const int32_t numVersions, const std::map<Text, Text> & attributes) {
    // Your implementation goes here
    printf("getVerTs\n");
  }

  /**
   * Get all the data for the specified table and row at the latest
   * timestamp. Returns an empty list if the row does not exist.
   * 
   * @return TRowResult containing the row and map of columns to TCells
   * 
   * @param tableName name of table
   * 
   * @param row row key
   * 
   * @param attributes Get attributes
   */
  void getRow(std::vector<TRowResult> & _return, const Text& tableName, const Text& row, const std::map<Text, Text> & attributes) {
    // Your implementation goes here
    printf("getRow\n");
  }

  /**
   * Get the specified columns for the specified table and row at the latest
   * timestamp. Returns an empty list if the row does not exist.
   * 
   * @return TRowResult containing the row and map of columns to TCells
   * 
   * @param tableName name of table
   * 
   * @param row row key
   * 
   * @param columns List of columns to return, null for all columns
   * 
   * @param attributes Get attributes
   */
  void getRowWithColumns(std::vector<TRowResult> & _return, const Text& tableName, const Text& row, const std::vector<Text> & columns, const std::map<Text, Text> & attributes) {
    // Your implementation goes here
    printf("getRowWithColumns\n");
  }

  /**
   * Get all the data for the specified table and row at the specified
   * timestamp. Returns an empty list if the row does not exist.
   * 
   * @return TRowResult containing the row and map of columns to TCells
   * 
   * @param tableName name of the table
   * 
   * @param row row key
   * 
   * @param timestamp timestamp
   * 
   * @param attributes Get attributes
   */
  void getRowTs(std::vector<TRowResult> & _return, const Text& tableName, const Text& row, const int64_t timestamp, const std::map<Text, Text> & attributes) {
    // Your implementation goes here
    printf("getRowTs\n");
  }

  /**
   * Get the specified columns for the specified table and row at the specified
   * timestamp. Returns an empty list if the row does not exist.
   * 
   * @return TRowResult containing the row and map of columns to TCells
   * 
   * @param tableName name of table
   * 
   * @param row row key
   * 
   * @param columns List of columns to return, null for all columns
   * 
   * @param timestamp
   * @param attributes Get attributes
   */
  void getRowWithColumnsTs(std::vector<TRowResult> & _return, const Text& tableName, const Text& row, const std::vector<Text> & columns, const int64_t timestamp, const std::map<Text, Text> & attributes) {
    // Your implementation goes here
    printf("getRowWithColumnsTs\n");
  }

  /**
   * Get all the data for the specified table and rows at the latest
   * timestamp. Returns an empty list if no rows exist.
   * 
   * @return TRowResult containing the rows and map of columns to TCells
   * 
   * @param tableName name of table
   * 
   * @param rows row keys
   * 
   * @param attributes Get attributes
   */
  void getRows(std::vector<TRowResult> & _return, const Text& tableName, const std::vector<Text> & rows, const std::map<Text, Text> & attributes) {
    // Your implementation goes here
    printf("getRows\n");
  }

  /**
   * Get the specified columns for the specified table and rows at the latest
   * timestamp. Returns an empty list if no rows exist.
   * 
   * @return TRowResult containing the rows and map of columns to TCells
   * 
   * @param tableName name of table
   * 
   * @param rows row keys
   * 
   * @param columns List of columns to return, null for all columns
   * 
   * @param attributes Get attributes
   */
  void getRowsWithColumns(std::vector<TRowResult> & _return, const Text& tableName, const std::vector<Text> & rows, const std::vector<Text> & columns, const std::map<Text, Text> & attributes) {
    // Your implementation goes here
    printf("getRowsWithColumns\n");
  }

  /**
   * Get all the data for the specified table and rows at the specified
   * timestamp. Returns an empty list if no rows exist.
   * 
   * @return TRowResult containing the rows and map of columns to TCells
   * 
   * @param tableName name of the table
   * 
   * @param rows row keys
   * 
   * @param timestamp timestamp
   * 
   * @param attributes Get attributes
   */
  void getRowsTs(std::vector<TRowResult> & _return, const Text& tableName, const std::vector<Text> & rows, const int64_t timestamp, const std::map<Text, Text> & attributes) {
    // Your implementation goes here
    printf("getRowsTs\n");
  }

  /**
   * Get the specified columns for the specified table and rows at the specified
   * timestamp. Returns an empty list if no rows exist.
   * 
   * @return TRowResult containing the rows and map of columns to TCells
   * 
   * @param tableName name of table
   * 
   * @param rows row keys
   * 
   * @param columns List of columns to return, null for all columns
   * 
   * @param timestamp
   * @param attributes Get attributes
   */
  void getRowsWithColumnsTs(std::vector<TRowResult> & _return, const Text& tableName, const std::vector<Text> & rows, const std::vector<Text> & columns, const int64_t timestamp, const std::map<Text, Text> & attributes) {
    // Your implementation goes here
    printf("getRowsWithColumnsTs\n");
  }

  /**
   * Apply a series of mutations (updates/deletes) to a row in a
   * single transaction.  If an exception is thrown, then the
   * transaction is aborted.  Default current timestamp is used, and
   * all entries will have an identical timestamp.
   * 
   * @param tableName name of table
   * 
   * @param row row key
   * 
   * @param mutations list of mutation commands
   * 
   * @param attributes Mutation attributes
   */
  void mutateRow(const Text& tableName, const Text& row, const std::vector<Mutation> & mutations, const std::map<Text, Text> & attributes) {
    // Your implementation goes here
    printf("mutateRow\n");
  }

  /**
   * Apply a series of mutations (updates/deletes) to a row in a
   * single transaction.  If an exception is thrown, then the
   * transaction is aborted.  The specified timestamp is used, and
   * all entries will have an identical timestamp.
   * 
   * @param tableName name of table
   * 
   * @param row row key
   * 
   * @param mutations list of mutation commands
   * 
   * @param timestamp timestamp
   * 
   * @param attributes Mutation attributes
   */
  void mutateRowTs(const Text& tableName, const Text& row, const std::vector<Mutation> & mutations, const int64_t timestamp, const std::map<Text, Text> & attributes) {
    // Your implementation goes here
    printf("mutateRowTs\n");
  }

  /**
   * Apply a series of batches (each a series of mutations on a single row)
   * in a single transaction.  If an exception is thrown, then the
   * transaction is aborted.  Default current timestamp is used, and
   * all entries will have an identical timestamp.
   * 
   * @param tableName name of table
   * 
   * @param rowBatches list of row batches
   * 
   * @param attributes Mutation attributes
   */
  void mutateRows(const Text& tableName, const std::vector<BatchMutation> & rowBatches, const std::map<Text, Text> & attributes) {
    // Your implementation goes here
    printf("mutateRows\n");
  }

  /**
   * Apply a series of batches (each a series of mutations on a single row)
   * in a single transaction.  If an exception is thrown, then the
   * transaction is aborted.  The specified timestamp is used, and
   * all entries will have an identical timestamp.
   * 
   * @param tableName name of table
   * 
   * @param rowBatches list of row batches
   * 
   * @param timestamp timestamp
   * 
   * @param attributes Mutation attributes
   */
  void mutateRowsTs(const Text& tableName, const std::vector<BatchMutation> & rowBatches, const int64_t timestamp, const std::map<Text, Text> & attributes) {
    // Your implementation goes here
    printf("mutateRowsTs\n");
  }

  /**
   * Atomically increment the column value specified.  Returns the next value post increment.
   * 
   * @param tableName name of table
   * 
   * @param row row to increment
   * 
   * @param column name of column
   * 
   * @param value amount to increment by
   */
  int64_t atomicIncrement(const Text& tableName, const Text& row, const Text& column, const int64_t value) {
    // Your implementation goes here
    printf("atomicIncrement\n");
  }

  /**
   * Delete all cells that match the passed row and column.
   * 
   * @param tableName name of table
   * 
   * @param row Row to update
   * 
   * @param column name of column whose value is to be deleted
   * 
   * @param attributes Delete attributes
   */
  void deleteAll(const Text& tableName, const Text& row, const Text& column, const std::map<Text, Text> & attributes) {
    // Your implementation goes here
    printf("deleteAll\n");
  }

  /**
   * Delete all cells that match the passed row and column and whose
   * timestamp is equal-to or older than the passed timestamp.
   * 
   * @param tableName name of table
   * 
   * @param row Row to update
   * 
   * @param column name of column whose value is to be deleted
   * 
   * @param timestamp timestamp
   * 
   * @param attributes Delete attributes
   */
  void deleteAllTs(const Text& tableName, const Text& row, const Text& column, const int64_t timestamp, const std::map<Text, Text> & attributes) {
    // Your implementation goes here
    printf("deleteAllTs\n");
  }

  /**
   * Completely delete the row's cells.
   * 
   * @param tableName name of table
   * 
   * @param row key of the row to be completely deleted.
   * 
   * @param attributes Delete attributes
   */
  void deleteAllRow(const Text& tableName, const Text& row, const std::map<Text, Text> & attributes) {
    // Your implementation goes here
    printf("deleteAllRow\n");
  }

  /**
   * Increment a cell by the ammount.
   * Increments can be applied async if hbase.regionserver.thrift.coalesceIncrement is set to true.
   * False is the default.  Turn to true if you need the extra performance and can accept some
   * data loss if a thrift server dies with increments still in the queue.
   * 
   * @param increment The single increment to apply
   */
  void increment(const TIncrement& increment) {
    // Your implementation goes here
    printf("increment\n");
  }

  void incrementRows(const std::vector<TIncrement> & increments) {
    // Your implementation goes here
    printf("incrementRows\n");
  }

  /**
   * Completely delete the row's cells marked with a timestamp
   * equal-to or older than the passed timestamp.
   * 
   * @param tableName name of table
   * 
   * @param row key of the row to be completely deleted.
   * 
   * @param timestamp timestamp
   * 
   * @param attributes Delete attributes
   */
  void deleteAllRowTs(const Text& tableName, const Text& row, const int64_t timestamp, const std::map<Text, Text> & attributes) {
    // Your implementation goes here
    printf("deleteAllRowTs\n");
  }

  /**
   * Get a scanner on the current table, using the Scan instance
   * for the scan parameters.
   * 
   * @param tableName name of table
   * 
   * @param scan Scan instance
   * 
   * @param attributes Scan attributes
   */
  ScannerID scannerOpenWithScan(const Text& tableName, const TScan& scan, const std::map<Text, Text> & attributes) {
    // Your implementation goes here
    printf("scannerOpenWithScan\n");
  }

  /**
   * Get a scanner on the current table starting at the specified row and
   * ending at the last row in the table.  Return the specified columns.
   * 
   * @return scanner id to be used with other scanner procedures
   * 
   * @param tableName name of table
   * 
   * @param startRow Starting row in table to scan.
   * Send "" (empty string) to start at the first row.
   * 
   * @param columns columns to scan. If column name is a column family, all
   * columns of the specified column family are returned. It's also possible
   * to pass a regex in the column qualifier.
   * 
   * @param attributes Scan attributes
   */
  ScannerID scannerOpen(const Text& tableName, const Text& startRow, const std::vector<Text> & columns, const std::map<Text, Text> & attributes) {
    // Your implementation goes here
    printf("scannerOpen\n");
  }

  /**
   * Get a scanner on the current table starting and stopping at the
   * specified rows.  ending at the last row in the table.  Return the
   * specified columns.
   * 
   * @return scanner id to be used with other scanner procedures
   * 
   * @param tableName name of table
   * 
   * @param startRow Starting row in table to scan.
   * Send "" (empty string) to start at the first row.
   * 
   * @param stopRow row to stop scanning on. This row is *not* included in the
   * scanner's results
   * 
   * @param columns columns to scan. If column name is a column family, all
   * columns of the specified column family are returned. It's also possible
   * to pass a regex in the column qualifier.
   * 
   * @param attributes Scan attributes
   */
  ScannerID scannerOpenWithStop(const Text& tableName, const Text& startRow, const Text& stopRow, const std::vector<Text> & columns, const std::map<Text, Text> & attributes) {
    // Your implementation goes here
    printf("scannerOpenWithStop\n");
  }

  /**
   * Open a scanner for a given prefix.  That is all rows will have the specified
   * prefix. No other rows will be returned.
   * 
   * @return scanner id to use with other scanner calls
   * 
   * @param tableName name of table
   * 
   * @param startAndPrefix the prefix (and thus start row) of the keys you want
   * 
   * @param columns the columns you want returned
   * 
   * @param attributes Scan attributes
   */
  ScannerID scannerOpenWithPrefix(const Text& tableName, const Text& startAndPrefix, const std::vector<Text> & columns, const std::map<Text, Text> & attributes) {
    // Your implementation goes here
    printf("scannerOpenWithPrefix\n");
  }

  /**
   * Get a scanner on the current table starting at the specified row and
   * ending at the last row in the table.  Return the specified columns.
   * Only values with the specified timestamp are returned.
   * 
   * @return scanner id to be used with other scanner procedures
   * 
   * @param tableName name of table
   * 
   * @param startRow Starting row in table to scan.
   * Send "" (empty string) to start at the first row.
   * 
   * @param columns columns to scan. If column name is a column family, all
   * columns of the specified column family are returned. It's also possible
   * to pass a regex in the column qualifier.
   * 
   * @param timestamp timestamp
   * 
   * @param attributes Scan attributes
   */
  ScannerID scannerOpenTs(const Text& tableName, const Text& startRow, const std::vector<Text> & columns, const int64_t timestamp, const std::map<Text, Text> & attributes) {
    // Your implementation goes here
    printf("scannerOpenTs\n");
  }

  /**
   * Get a scanner on the current table starting and stopping at the
   * specified rows.  ending at the last row in the table.  Return the
   * specified columns.  Only values with the specified timestamp are
   * returned.
   * 
   * @return scanner id to be used with other scanner procedures
   * 
   * @param tableName name of table
   * 
   * @param startRow Starting row in table to scan.
   * Send "" (empty string) to start at the first row.
   * 
   * @param stopRow row to stop scanning on. This row is *not* included in the
   * scanner's results
   * 
   * @param columns columns to scan. If column name is a column family, all
   * columns of the specified column family are returned. It's also possible
   * to pass a regex in the column qualifier.
   * 
   * @param timestamp timestamp
   * 
   * @param attributes Scan attributes
   */
  ScannerID scannerOpenWithStopTs(const Text& tableName, const Text& startRow, const Text& stopRow, const std::vector<Text> & columns, const int64_t timestamp, const std::map<Text, Text> & attributes) {
    // Your implementation goes here
    printf("scannerOpenWithStopTs\n");
  }

  /**
   * Returns the scanner's current row value and advances to the next
   * row in the table.  When there are no more rows in the table, or a key
   * greater-than-or-equal-to the scanner's specified stopRow is reached,
   * an empty list is returned.
   * 
   * @return a TRowResult containing the current row and a map of the columns to TCells.
   * 
   * @throws IllegalArgument if ScannerID is invalid
   * 
   * @throws NotFound when the scanner reaches the end
   * 
   * @param id id of a scanner returned by scannerOpen
   */
  void scannerGet(std::vector<TRowResult> & _return, const ScannerID id) {
    // Your implementation goes here
    printf("scannerGet\n");
  }

  /**
   * Returns, starting at the scanner's current row value nbRows worth of
   * rows and advances to the next row in the table.  When there are no more
   * rows in the table, or a key greater-than-or-equal-to the scanner's
   * specified stopRow is reached,  an empty list is returned.
   * 
   * @return a TRowResult containing the current row and a map of the columns to TCells.
   * 
   * @throws IllegalArgument if ScannerID is invalid
   * 
   * @throws NotFound when the scanner reaches the end
   * 
   * @param id id of a scanner returned by scannerOpen
   * 
   * @param nbRows number of results to return
   */
  void scannerGetList(std::vector<TRowResult> & _return, const ScannerID id, const int32_t nbRows) {
    // Your implementation goes here
    printf("scannerGetList\n");
  }

  /**
   * Closes the server-state associated with an open scanner.
   * 
   * @throws IllegalArgument if ScannerID is invalid
   * 
   * @param id id of a scanner returned by scannerOpen
   */
  void scannerClose(const ScannerID id) {
    // Your implementation goes here
    printf("scannerClose\n");
  }

  /**
   * Get the row just before the specified one.
   * 
   * @return value for specified row/column
   * 
   * @param tableName name of table
   * 
   * @param row row key
   * 
   * @param family column name
   */
  void getRowOrBefore(std::vector<TCell> & _return, const Text& tableName, const Text& row, const Text& family) {
    // Your implementation goes here
    printf("getRowOrBefore\n");
  }

  /**
   * Get the regininfo for the specified row. It scans
   * the metatable to find region's start and end keys.
   * 
   * @return value for specified row/column
   * 
   * @param row row key
   */
  void getRegionInfo(TRegionInfo& _return, const Text& row) {
    // Your implementation goes here
    printf("getRegionInfo\n");
  }

};

int main(int argc, char **argv) {
  int port = 9090;
  shared_ptr<HbaseHandler> handler(new HbaseHandler());
  shared_ptr<TProcessor> processor(new HbaseProcessor(handler));
  shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
  shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
  shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

  TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
  server.serve();
  return 0;
}

