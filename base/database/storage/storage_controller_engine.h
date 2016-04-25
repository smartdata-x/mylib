//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015骞�9鏈�17鏃� Author: kerry

#ifndef KID_STORAGE_STORAGE_CONTROLLER_ENGINE_H_
#define KID_STORAGE_STORAGE_CONTROLLER_ENGINE_H_
#include <pthread.h>

#include <list>
#include "basic/basictypes.h"
#include "logic/base_values.h"
#include "config/config.h"
#include "storage/storage.h"

enum STORAGETYPE{
    REIDS_TYPE = 0,
    MYSQL_TYPE = 1,
    MEM_TYPE = 2
};

enum MEMCACHE_TYPE{
    MEM_KEY_VALUE = 0,
    BATCH_KEY_VALUE = 1,
};

enum REDIS_TYPE{
    HASH_VALUE = 0,
    KEY_VALUE = 1,
    SET_VALUE = 2,
};


namespace base_logic {

class MysqlController;

class DataControllerEngine {
 public:
    static DataControllerEngine* Create(int32 type);
    static void Init(config::FileConfig* config);
    static void Dest();
    virtual ~DataControllerEngine() {}
 public:
    virtual void Release() = 0;
    virtual void InitParam(std::list<base::ConnAddr>* addrlist) = 0;
 public:
    virtual bool ReadData(const int32 type, base_logic::Value* value,
    void (*storage_get)(void*, base_logic::Value*)) = 0;

    virtual bool WriteData(const int32 type, base_logic::Value* value) = 0;

 private:
    //static void StartSqlThread(DataControllerEngine *MySqlEngine);
    //static void *MySqlThreadFunc(void *arg);

 private:
    static DataControllerEngine *MySqlEngine_;
};
}  // namespace base_logic


#endif /* PLUGINS_CRAWLERSVC_STORAGE_STORAGE_BASE_ENGINE_H_ */
