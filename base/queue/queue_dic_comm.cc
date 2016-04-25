/*
 * queue_dic_comm.cc
 *
 *  Created on: 2015年1月3日
 *      Author: kerry
 */
#include "queue_dic_comm.h"
#include "database/dic/base_dic_redis_auto.h"
namespace base_queue{


QueueDicComm::QueueDicComm(){

}

QueueDicComm::~QueueDicComm(){
	Dest();
}

void QueueDicComm::Init(std::list<base::ConnAddr>& addrlist){
	base_dic::RedisPool::Init(addrlist);
}

void QueueDicComm::Dest(){
	base_dic::RedisPool::Dest();
}

bool QueueDicComm::AddBlockQueueMessage(const std::string& key,const std::string& str){
	//list
	bool r = false;
	base_dic::AutoDicCommEngine auto_engine;
	base_storage::DictionaryStorageEngine* redis_engine_  = auto_engine.GetDicEngine();
	r =  redis_engine_->AddListElement(key.c_str(),key.length(),str.c_str(),str.length(),1);
	return r;
}

bool QueueDicComm::GetBlockQueueMessage(const std::string& key,std::list<std::string>& list,
		const int32 count){
	bool r = false;
	int32 temp_count = count;
	base_dic::AutoDicCommEngine auto_engine;
	base_storage::DictionaryStorageEngine* redis_engine_  = auto_engine.GetDicEngine();
	//读取消息
	r = true;
	while(temp_count>0&&r){
		char* val = NULL;
		size_t val_len = 0;
		r = redis_engine_->PopListElement(key.c_str(),key.length(),&val,&val_len,1);
		if(!r)
			return true;
		std::string str(val,val_len);
		LOG_DEBUG2("%s",str.c_str());
		list.push_back(str);
		temp_count--;
	}
	return true;
}

bool QueueDicComm::GetBlockQueueMessage(const std::string& key,std::string& str){
	bool r = false;
	char* val = NULL;
	size_t val_len = 0;
	base_dic::AutoDicCommEngine auto_engine;
	base_storage::DictionaryStorageEngine* redis_engine_  = auto_engine.GetDicEngine();
	r = redis_engine_->PopListElement(key.c_str(),key.length(),&val,&val_len,1);
	if(!r)
		return false;
	str.assign(val,val_len);
	LOG_DEBUG2("%s",str.c_str());
	return true;
}

}



