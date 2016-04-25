/*
 * base_dic_redis_auto.cc
 *
 *  Created on: 2014年12月22日
 *      Author: kerry
 */
#include "base_dic_redis_auto.h"
#include "storage/storage.h"
#include "basic/basictypes.h"
#include "basic/basic_info.h"
#include "thread/base_thread_handler.h"
#include "thread/base_thread_lock.h"
namespace base_dic{

threadrw_t* RedisPool::dic_pool_lock_;
std::list<base_storage::DictionaryStorageEngine*>  RedisPool::dic_conn_pool_;

std::list<base::ConnAddr> RedisPool::addrlist_;

AutoDicCommEngine::AutoDicCommEngine(){
	engine_ = base_dic::RedisPool::RedisConnectionPop();
}

AutoDicCommEngine::~AutoDicCommEngine(){
	base_dic::RedisPool::RedisConnectionPush(engine_);
}


void RedisPool::Init(std::list<base::ConnAddr>& addrlist,const int32 conn_num){
	bool r =false;
	addrlist_ = addrlist;
	InitThreadrw(&dic_pool_lock_);
	for (int i = 0; i<=conn_num;i++){
		base_storage::DictionaryStorageEngine* engine =
				base_storage::DictionaryStorageEngine::Create(base_storage::IMPL_RADIES);

		if (engine==NULL){
			assert(0);
			continue;
		}
		/*MIG_DEBUG(USER_LEVEL, "ip:%s,port:%d", addrlist_.front().host().c_str(),
				addrlist_.front().port());*/
		bool r =  engine->Connections(addrlist_);
		if (!r)
			continue;

		dic_conn_pool_.push_back(engine);
	}
}


void RedisPool::Dest(){
	base_logic::WLockGd lk(dic_pool_lock_);
	while(dic_conn_pool_.size()>0){
		base_storage::DictionaryStorageEngine* engine = dic_conn_pool_.front();
		dic_conn_pool_.pop_front();
		if(engine){
			engine->Release();
			delete engine;
			engine =NULL;
		}
	}
	DeinitThreadrw(dic_pool_lock_);
}

void RedisPool::RedisConnectionPush(base_storage::DictionaryStorageEngine* engine){
	base_logic::WLockGd lk(dic_pool_lock_);
	dic_conn_pool_.push_back(engine);
}

base_storage::DictionaryStorageEngine* RedisPool::RedisConnectionPop(){
	if(dic_conn_pool_.size()<=0)
		return NULL;
	base_logic::WLockGd lk(dic_pool_lock_);
    base_storage::DictionaryStorageEngine* engine = dic_conn_pool_.front();
    dic_conn_pool_.pop_front();
    return engine;
}

/*
template <typename Container, typename Element>
struct ElementStorageContainerT{
static void ElementStorageList(Container container,Element elment){
	container.push_back(elment);
}
static void ELmenetStorageVector(Container container,Element elment){
	container.push_back(elment);
}

};

template <typename Container,typename Value>
void RedisOperation::GetBatchInfos(base_storage::DictionaryStorageEngine*engine,
	                              const std::string& command,
	                              Container& batch){
    redisContext *context = (redisContext *)engine->GetContext();
	LOG_DEBUG2("%s",command.c_str());
	if (NULL == context)
		return false;
	{
		redisReply *rpl = (redisReply *) redisCommand(context,command.c_str());
		base_storage::CommandReply *reply = _CreateReply(rpl);
		freeReplyObject(rpl);
		if (NULL == reply)
			return false;
		if (base_storage::CommandReply::REPLY_ARRAY == reply->type) {
			base_storage::ArrayReply *arep =
				static_cast<base_storage::ArrayReply *>(reply);
			base_storage::ArrayReply::value_type &items = arep->value;
			for (base_storage::ArrayReply::iterator it = items.begin();
				it != items.end();++it) {
					base_storage::CommandReply *item = (*it);
					if (base_storage::CommandReply::REPLY_STRING == item->type) {
						base_storage::StringReply *srep = static_cast<base_storage::StringReply *>(item);
						//通过模板转化
						//ElementStorageContainerT<Container,std::string>::ElementStorageList(batch,srep->value);
					}
			}
		}
		reply->Release();
	}
	return true;
}*/




}



