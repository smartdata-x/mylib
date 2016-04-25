#include "lbs_db_comm.h"
#include "basic/basic_info.h"
#include "database/db/base_db_mysql_auto.h"
#include "storage/storage.h"
#include <mysql.h>
#include <sstream>

namespace base_lbs{

void LBSDBComm::Init(std::list<base::ConnAddr>& addrlist){
#if defined (_DB_POOL_)
	base_db::MysqlDBPool::Init(addrlist);
#endif
}

void LBSDBComm::Dest(){
#if defined (_DB_POOL_)
	base_db::MysqlDBPool::Dest();
#endif
}

bool LBSDBComm::GetBaiduAcessKey(const int64 platform_id,std::list<base_lbs::BaiDuAccessKey>& list){
	bool r = false;
#if defined (_DB_POOL_)
	base_db::AutoMysqlCommEngine auto_engine;
	base_storage::DBStorageEngine* engine  = auto_engine.GetDBEngine();
#endif
	std::stringstream os;
	MYSQL_ROW rows;

	if (engine==NULL){
		LOG_ERROR("GetConnection Error");
		return false;
	}

    //call proc_GetBaiduAcessKey();
	os<<"call proc_GetBaiduAccessKey()";
	std::string sql = os.str();
	LOG_DEBUG2("[%s]", sql.c_str());
	r = engine->SQLExec(sql.c_str());

	if (!r) {
		LOG_ERROR("exec sql error");
		return false;
	}


	int num = engine->RecordCount();
	if(num>0){
		while(rows = (*(MYSQL_ROW*)(engine->FetchRows())->proc)){
			std::string access_token = rows[0];
			LOG_DEBUG2("access_token %s",access_token.c_str());
			base_lbs::BaiDuAccessKey access_key_info(access_token);
			list.push_front(access_key_info);
		}
		return true;
	}
	return false;
}
}
