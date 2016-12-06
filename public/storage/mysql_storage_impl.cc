#include "mysql_storage_impl.h"
#include "log/mig_log.h"
namespace base_storage {

MysqlStorageEngineImpl::MysqlStorageEngineImpl()
:connected_(false){

  conn_.reset(new db_conn_t);
  result_.reset(new db_res_t);
  rows_.reset(new db_row_t);
  conn_.get()->proc = NULL;
  result_.get()->proc = NULL;
  rows_.get()->proc = NULL;
}

MysqlStorageEngineImpl::~MysqlStorageEngineImpl() {
  Release();
}

bool MysqlStorageEngineImpl::Connections(std::list<base::ConnAddr>& addrlist) {
  connected_ = false;
  base::ConnAddr addr;
  MYSQL* mysql = NULL;
  mysql = mysql_init((MYSQL*) 0);
  mysql_options(mysql, MYSQL_SET_CHARSET_NAME, "utf8");
  std::list<base::ConnAddr>::iterator it = addrlist.begin();
  while (it != addrlist.end()) {
    addr = (*it);
    if (mysql_real_connect(mysql, addr.host().c_str(), addr.usr().c_str(),
                           addr.pwd().c_str(), addr.source().c_str(),
                           addr.port(), 0,
                           /*CLIENT_INTERACTIVE*/CLIENT_MULTI_STATEMENTS)
        == NULL) {
      MIG_ERROR(USER_LEVEL, "mysql:connection to database failed failed %s",
                mysql_error(mysql));
      return false;
    }

    //mysql_set_character_set(&mysql,"gbk");
    /*MIG_INFO(USER_LEVEL,"mysql ip[%s] port[%d] user[%s] pwd[%s] db[%s]",
     addr.host().c_str(),addr.port(),addr.usr().c_str(),
     addr.pwd().c_str(),addr.source().c_str());*/
    break;
  }
  mysql_set_server_option(mysql, MYSQL_OPTION_MULTI_STATEMENTS_ON);
  mysql_query(mysql, "SET NAMES 'binary'");
  connected_ = true;
  conn_.get()->proc = mysql;
  return true;
}

bool MysqlStorageEngineImpl::Release() {

  FreeRes();

  MYSQL* mysql = (MYSQL*)(conn_.get()->proc);
  mysql_close(mysql);
  conn_.get()->proc = NULL;
  result_.get()->proc = NULL;
  rows_.get()->proc = NULL;
  return true;
}

bool MysqlStorageEngineImpl::FreeRes() {
  MYSQL_RES * result = (MYSQL_RES *) result_.get()->proc;
  if (NULL == result)
    return true;
  mysql_free_result(result);
  MYSQL* mysql = (MYSQL*) conn_.get()->proc;
  while (!mysql_next_result(mysql)) {
    result = mysql_store_result(mysql);
    mysql_free_result(result);
  }
  result_.get()->proc = NULL;
  return true;
}

bool MysqlStorageEngineImpl::SQLExec(const char* sql) {
  bool r = CheckConnect();
  if (!r) {
    MIG_ERROR(USER_LEVEL, "lost connection");
    return false;
  }
  FreeRes();
  MYSQL* mysql = (MYSQL*) conn_.get()->proc;
  mysql_query(mysql, sql);
  result_.get()->proc = mysql_store_result(mysql);
  if (mysql->net.last_errno != 0) {
    MIG_ERROR(USER_LEVEL, "mysql error code [%d] [%s]", mysql_errno(mysql),
              mysql_error(mysql));
  }
  return true;
}

bool MysqlStorageEngineImpl::SQLExecs(std::list<std::string>& sqls) {
  bool r = CheckConnect();
  if(!r){
    MIG_ERROR(USER_LEVEL,"lost connection");
    return false;
  }
  FreeRes();
  MYSQL* mysql = (MYSQL*)conn_.get()->proc;
  mysql_autocommit(mysql, 0);
  std::list<std::string>::iterator it = sqls.begin();
  while (it != sqls.end()) {
    int r = mysql_query(mysql, (*it).c_str());
    if (r != 0) {
      MIG_ERROR(USER_LEVEL,"mysql error code [%d] [%s]",
      mysql_errno(mysql),mysql_error(mysql));
      ++it;
      continue;
    }
    ++it;
  }
  mysql_commit(mysql);
  mysql_autocommit(mysql, 1);
  return true;
}


uint32 MysqlStorageEngineImpl::RecordCount(){
    //unsigned long ulCount = (unsigned long)mysql_num_rows((MYSQL_RES *)(result_.get()->proc));
  unsigned long ulCount = (unsigned long)mysql_affected_rows((MYSQL*)conn_.get()->proc);
  MIG_DEBUG(USER_LEVEL,"ulconut ==%d===\n",ulCount);
  return ulCount;
}


bool MysqlStorageEngineImpl::Affected(unsigned long& rows) {
  rows = (unsigned long) mysql_affected_rows((MYSQL*) conn_.get()->proc);
  return true;
}

db_row_t* MysqlStorageEngineImpl::FetchRows() {
  MYSQL_RES* result = (MYSQL_RES*) (result_.get()->proc);
  row_ = mysql_fetch_row(result);
  rows_.get()->proc = &row_;
  return rows_.get();
}

bool MysqlStorageEngineImpl::CheckConnect(void) {
  MYSQL* mysql = (MYSQL*) conn_.get()->proc;
  return (mysql_ping(mysql) == 0);
}

}
