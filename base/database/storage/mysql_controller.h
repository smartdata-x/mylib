//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015骞�9鏈�17鏃� Author: kerry

#ifndef KID_MYSQL_CONTROLLER_
#define KID_MYSQL_CONTROLLER_

#include <pthread.h>

#include <string>
#include <list>
#include <queue>
#include "../storage/storage_controller_engine.h"
#include "logic/base_values.h"
#include "basic/scoped_ptr.h"

namespace base_logic {

typedef void (*STORAGE_GET)(void*, base_logic::Value*);

//  同步
#define SYN  0X01

//  异步
#define ASYN 0X02

class MySqlTask {
 public:
  MySqlTask() : callbackfunc_(NULL), thread_flag_(SYN), dict_(NULL) {
    pthread_cond_init(&task_cond_, NULL);
  }

  MySqlTask(std::string &sql, STORAGE_GET callbackfunc, short thread_flag)
    : sql_(sql), callbackfunc_(callbackfunc), thread_flag_(thread_flag), \
	  dict_(NULL) {
    pthread_cond_init(&task_cond_, NULL);
  }

  ~MySqlTask() {
    pthread_cond_destroy(&task_cond_);
  }

  std::string sql_;
  base_logic::DictionaryValue* dict_;
  STORAGE_GET callbackfunc_;
  int16 thread_flag_;
  pthread_cond_t  task_cond_;
};

class MysqlController:public DataControllerEngine {
 public:
    MysqlController() {
      pthread_cond_init(&queue_cond_, NULL);
      pthread_mutex_init(&queue_mutex_, NULL);
    }
    virtual ~MysqlController() {
      pthread_cond_destroy(&queue_cond_);
      pthread_mutex_destroy(&queue_mutex_);
    }
 public:
    void InitParam(std::list<base::ConnAddr>* addrlist);
    void Release();

 public:
    bool ReadData(const int32 type, base_logic::Value* value,
            void (*storage_get)(void*, base_logic::Value*));

    bool WriteData(const int32 type, base_logic::Value* value);

    bool DispatchTask();

 private:
    bool HandleTask(MySqlTask *task);

    bool AddTask(MySqlTask *task);
    MySqlTask * DeleteTask();
 
 public:
    static void StartSqlThread(DataControllerEngine *MySqlEngine);
    static void *MySqlThreadFunc(void *arg);

 private:
    std::queue<MySqlTask *> sqltasks_;
    pthread_cond_t queue_cond_;
    pthread_mutex_t queue_mutex_;
};
}  // namespace base_logic



#endif  // PUB_STORAGE_MYSQL_CONTROLLER_H_
