#include "base_db_mysql_auto.h"
#include "storage/storage.h"
#include "basic/basictypes.h"
#include "basic/basic_info.h"
#include "thread/base_thread_handler.h"
#include "thread/base_thread_lock.h"

namespace base_db{

#if defined (_DB_POOL_)
threadrw_t* MysqlDBPool::db_pool_lock_;
std::list<base_storage::DBStorageEngine*>  MysqlDBPool::db_conn_pool_;
#endif

std::list<base::ConnAddr> MysqlDBPool::addrlist_;

AutoMysqlCommEngine::AutoMysqlCommEngine(){
#if defined (_DB_POOL_)
	engine_ = base_db::MysqlDBPool::DBConnectionPop();
#endif
}

AutoMysqlCommEngine::~AutoMysqlCommEngine(){
#if defined (_DB_POOL_)
	base_db::MysqlDBPool::DBConnectionPush(engine_);
#endif
}

void MysqlDBPool::Init(std::list<base::ConnAddr>& addrlist,
				  const int32 db_conn_num/* = 5*/){
	addrlist_ = addrlist;

#if defined (_DB_POOL_)
	bool r =false;
	InitThreadrw(&db_pool_lock_);
	for (int i = 0; i<db_conn_num;i++){
		base_storage::DBStorageEngine* engine  =
				base_storage::DBStorageEngine::Create(base_storage::IMPL_MYSQL);
		if (engine==NULL){
			assert(0);
			LOG_ERROR("create db conntion error");
			continue;
		}

		r = engine->Connections(addrlist_);
		if (!r){
			assert(0);
			LOG_ERROR("db conntion error");
			continue;
		}

		db_conn_pool_.push_back(engine);

	}

#endif
}

#if defined (_DB_POOL_)

void MysqlDBPool::DBConnectionPush(base_storage::DBStorageEngine* engine){
	base_logic::WLockGd lk(db_pool_lock_);
	db_conn_pool_.push_back(engine);
}

base_storage::DBStorageEngine* MysqlDBPool::DBConnectionPop(){
	if(db_conn_pool_.size()<=0)
		return NULL;
	base_logic::WLockGd lk(db_pool_lock_);
    base_storage::DBStorageEngine* engine = db_conn_pool_.front();
    db_conn_pool_.pop_front();
    return engine;
}

#endif


void MysqlDBPool::Dest(){
#if defined (_DB_POOL_)
	base_logic::WLockGd lk(db_pool_lock_);
	while(db_conn_pool_.size()>0){
		base_storage::DBStorageEngine* engine = db_conn_pool_.front();
		db_conn_pool_.pop_front();
		if(engine){
			engine->Release();
			delete engine;
			engine =NULL;
		}
	}
	DeinitThreadrw(db_pool_lock_);

#endif
}

}
