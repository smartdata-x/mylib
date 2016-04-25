/*
 * base_dic_redis_auto.h
 *
 *  Created on: 2014年12月22日
 *      Author: kerry
 */

#ifndef BASE_DIC_REDIS_AUTO_H_
#define BASE_DIC_REDIS_AUTO_H_

#include "storage/storage.h"
#include "storage/redis_storage_impl.h"
#include "basic/basictypes.h"
#include "basic/basic_info.h"
#include "thread/base_thread_handler.h"
#include "thread/base_thread_lock.h"
#include "logic/logic_comm.h"

namespace base_dic{

class RedisPool{
public:
	RedisPool();
	virtual ~RedisPool();
public:
	static void Init(std::list<base::ConnAddr>& addrlist,const int32 conn_num = 5);
	static void Dest();

	static base_storage::DictionaryStorageEngine* RedisConnectionPop(void);
	static void RedisConnectionPush(base_storage::DictionaryStorageEngine* engine);
private:


public:
	static std::list<base::ConnAddr>  addrlist_;
	static std::list<base_storage::DictionaryStorageEngine*>   dic_conn_pool_;
	static threadrw_t*                                        dic_pool_lock_;

};

class RedisOperation{
public:
	RedisOperation(){}
	virtual ~RedisOperation(){}
public:
	//base_storage::CommandReply* _CreateReply(redisReply* reply);

	/*template <typename Container>
	void GetBatchInfos(base_storage::DictionaryStorageEngine*engine,
	                              const std::string& command,
	                              Container& batch);*/
};



class AutoDicCommEngine{
public:
	AutoDicCommEngine();
	virtual ~AutoDicCommEngine();

	base_storage::DictionaryStorageEngine*  GetDicEngine(){
		if(!engine_){
			engine_->Release();//释放
			engine_ = NULL;
		}
		return engine_;
	}
private:
	base_storage::DictionaryStorageEngine*  engine_;
};


}



#endif /* BASE_DIC_REDIS_AUTO_H_ */
