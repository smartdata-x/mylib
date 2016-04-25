#ifndef _MIG_FM_LBS_CONNECTOR_H__
#define _MIG_FM_LBS_CONNECTOR_H__

#include "storage/storage.h"
#include "basic/basictypes.h"
#include "basic/basic_info.h"
#include "thread/base_thread_handler.h"
#include "thread/base_thread_lock.h"
#include "logic/logic_comm.h"

namespace base_db{

class MysqlDBPool{
public:
	MysqlDBPool();
	virtual ~MysqlDBPool();
public:
	static void Init(std::list<base::ConnAddr>& addrlist,const int32 db_conn_num = 5);
	static void Dest();
public:
#if defined (_DB_POOL_)
	static base_storage::DBStorageEngine* DBConnectionPop(void);
	static void DBConnectionPush(base_storage::DBStorageEngine* engine);
#endif
public:
	static std::list<base::ConnAddr>  addrlist_;
#if defined (_DB_POOL_)
	static std::list<base_storage::DBStorageEngine*>  db_conn_pool_;
	static threadrw_t*                                db_pool_lock_;
#endif
};

class AutoMysqlCommEngine{
public:
	AutoMysqlCommEngine();
	virtual ~AutoMysqlCommEngine();
	base_storage::DBStorageEngine*  GetDBEngine(){
		if(engine_){
      engine_->Release();  //  释放多余的记录集
			if(!engine_->CheckConnect()){//失去连接重新连接
				//重新创建连接  
				LOG_DEBUG("lost connection");
				if(!engine_->Connections(MysqlDBPool::addrlist_))
					return NULL;
			}
			return engine_;
		}
		return engine_;
	}
private:
	base_storage::DBStorageEngine*  engine_;
};

}

#endif
