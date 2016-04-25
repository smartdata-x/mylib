#include "push_db_comm.h"
#include "basic/basic_info.h"
#include "database/db/base_db_mysql_auto.h"
#include "storage/storage.h"
#include <mysql.h>
#include <sstream>

namespace base_push{

void PushDBComm::Init(std::list<base::ConnAddr>& addrlist){
#if defined (_DB_POOL_)
	base_db::MysqlDBPool::Init(addrlist);
#endif
}

void PushDBComm::Dest(){
#if defined (_DB_POOL_)
	base_db::MysqlDBPool::Dest();
#endif
}

bool PushDBComm::GetUserBdInfo(const int64 platform,const int64 uid,base_push::BaiduPushMessage& message){
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

    //call proc_GetUserBaiduPushInfo(1000,10000);
	os<<"call proc_GetUserBaiduPushInfo("<<platform<<","<<uid<<");";
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
			int64 channel = 0;
			std::string userid;
			std::string pkg_name;
			std::string tag;
			int machine = 0;

			if(rows[0]!=NULL)
				channel = atoll(rows[0]);
			if(rows[1]!=NULL)
				userid = rows[1];
			if(rows[2]!=NULL)
				pkg_name = rows[2];
			if(rows[3]!=NULL)
				tag = rows[3];
			if(rows[4]!=NULL)
				machine = atoll(rows[4]);
			message.set_channel(channel);
			message.set_baidu_userid(userid);
			message.set_pkg_name(pkg_name);
			message.set_tag(tag);
			message.set_machine(machine);
		}
		return true;
	}
	return false;
}

bool PushDBComm::GetAllPushMessage(std::list<base_push::BaiduPushMessage*>& list){
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

    //call bc.proc_GetAllPushMessage();
	os<<"call proc_GetAllPushMessage();";
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
			std::string title;
			std::string description;
			int style = 0;
			std::string tag;
			std::string pkg_name;
			std::string url;
			int32 level = 0;
			int64 msg_id = 0;
			if(rows[7]!=NULL)
				level = atol(rows[7]);
			if(rows[0]!=NULL)
				msg_id = atoll(rows[0]);
			base_push::BaiduPushMessage* push_message = new base_push::BaiduPushMessage(0,0,level,msg_id);
			if(rows[1]!=NULL){
				title = rows[1];
				push_message->set_title(title);
			}
			if(rows[2]!=NULL){
				description = rows[2];
				push_message->set_description(description);
			}
			if(rows[3]!=NULL){
				style = atol(rows[3]);
				push_message->set_notification_basic_style(style);
			}
			if(rows[4]!=NULL){
				tag = rows[4];
				push_message->set_tag(tag);
			}
			if(rows[5]!=NULL){
				url = rows[5];
				push_message->set_url(url);
			}
			if(rows[6]!=NULL){
				pkg_name = rows[6];
				push_message->set_pkg_name(pkg_name);
			}
			list.push_back(push_message);
		}
		return true;
	}
	return false;
}

bool PushDBComm::BindBDPushUserinfo(const int64 platform,const int64 uid,const int64 channel,
			const int64 baidu_userid,const std::string& pkg_name,const std::string& tag,
			const int64 appid,const int64 requestid,const int32 machine){
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

    //call proc_BindPushBaiduInfo(platform,uid,channel,'baidu_userid','pkg_name','tag','13123123','12312',1);
	os<<"call proc_BindPushBaiduInfo("
			<<platform<<","
			<<uid<<",\'"
			<<channel<<"\',\'"<<baidu_userid
			<<"\',\'"<<pkg_name.c_str()
			<<"\',\'"<<tag.c_str()<<"\',\'"<<appid<<"\',\'"<<requestid<<
			"\',"<<machine<<");";
	std::string sql = os.str();
	LOG_DEBUG2("[%s]", sql.c_str());
	r = engine->SQLExec(sql.c_str());

	if (!r) {
		LOG_ERROR("exec sql error");
		return false;
	}
	return true;
}

bool PushDBComm::BindBDPushUserinfo(const int64 platform,const int64 uid,const int64 channel,
			const std::string& baidu_userid,const std::string& pkg_name,const std::string& tag,
			const std::string& appid,const std::string& requestid,const int32 machine){
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

    //call proc_BindPushBaiduInfo(platform,uid,channel,'baidu_userid','pkg_name','tag','13123123','12312',1);
	os<<"call proc_BindPushBaiduInfo("
			<<platform<<","
			<<uid<<",\'"
			<<channel<<"\',\'"<<baidu_userid.c_str()
			<<"\',\'"<<pkg_name.c_str()
			<<"\',\'"<<tag.c_str()<<"\',\'"<<appid<<"\',\'"<<requestid<<
			"\',"<<machine<<");";
	std::string sql = os.str();
	LOG_DEBUG2("[%s]", sql.c_str());
	r = engine->SQLExec(sql.c_str());

	if (!r) {
		LOG_ERROR("exec sql error");
		return false;
	}
	return true;
}

bool PushDBComm::UpdateAllPushMessage(const int64 msg_id){
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

    //call proc_UpdateAllMessageStat(1);
	os<<"call proc_UpdateAllMessageStat("<<msg_id<<");";
	std::string sql = os.str();
	LOG_DEBUG2("[%s]", sql.c_str());
	r = engine->SQLExec(sql.c_str());

	if (!r) {
		LOG_ERROR("exec sql error");
		return false;
	}
	return true;
}


}
