//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015骞�9鏈�17鏃� Author: kerry

#include "mysql_controller.h"
#include "../db/base_db_mysql_auto.h"

namespace base_logic {
  //  static
  void * MysqlController::MySqlThreadFunc(void *arg) {
    MysqlController *MySqlEngine = reinterpret_cast<MysqlController *>(arg);
    while(1) {
      MySqlEngine->DispatchTask();
      LOG_DEBUG2("Thread : %s", "dispatch a task");
    }
    
    return NULL;
  }

  //  static
  void MysqlController::StartSqlThread(DataControllerEngine* Engine) {
    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);  //  创建一个分离的线程

    pthread_create(&tid, &attr, MySqlThreadFunc, (void *)Engine);
  }

  void MysqlController::InitParam(std::list<base::ConnAddr>* addrlist) {
}

void MysqlController::Release() {
}

bool MysqlController::AddTask(MySqlTask *task) {
  pthread_mutex_lock(&queue_mutex_);

  sqltasks_.push(task);

  if (sqltasks_.size() == 1) {
    pthread_cond_signal(&queue_cond_);
  }

  if (task->thread_flag_ == SYN)
    pthread_cond_wait(&(task->task_cond_), &queue_mutex_);

  pthread_mutex_unlock(&queue_mutex_);

  return true;
}

MySqlTask *MysqlController::DeleteTask() {
  pthread_mutex_lock(&queue_mutex_);

  MySqlTask *task = NULL;
  if (sqltasks_.empty()) {
    pthread_cond_wait(&queue_cond_, &queue_mutex_);
  }

  task = sqltasks_.front();
  sqltasks_.pop();

  pthread_mutex_unlock(&queue_mutex_);

  return task;
}

bool MysqlController::HandleTask(MySqlTask *task) {
  std::string sql = task->sql_;
  base_logic::DictionaryValue* dict = task->dict_;
  STORAGE_GET storage_get = task->callbackfunc_;

  bool r = dict->GetString(L"sql", &sql);
  if (!r)
	  return r;
  base_db::AutoMysqlCommEngine auto_engine;
  base_storage::DBStorageEngine* engine  = auto_engine.GetDBEngine();
  if (engine == NULL) {
	  LOG_ERROR("GetConnection Error");
	  return false;
  }
  LOG_MSG2("[%s]", sql.c_str());
  r = engine->SQLExec(sql.c_str());
  if (!r) {
    LOG_ERROR("exec sql error");
    return false;
  }

  if (storage_get != NULL)
    storage_get(reinterpret_cast<void*>(engine), dict);

  return true;
}

bool MysqlController::DispatchTask() {
  MySqlTask *task = DeleteTask();

  HandleTask(task);

  if (task->thread_flag_ == SYN)
    pthread_cond_signal(&task->task_cond_);

  delete task;
  return true;
}

bool MysqlController::WriteData(const int32 type, base_logic::Value* value) {

	MySqlTask *task = new MySqlTask();

	base_logic::DictionaryValue* dict =(base_logic::DictionaryValue*)(value);
	bool r = dict->GetString(L"sql", &task->sql_);
	if (!r)
		return r;

	r = dict->GetShortInteger(L"thread_flag", &task->thread_flag_);
	if (!r)
		task->thread_flag_ = SYN;

	task->dict_ = dict;

	AddTask(task);

  return true;
}


bool MysqlController::ReadData(const int32 type, base_logic::Value* value,
        void (*storage_get)(void*, base_logic::Value*)) {

	MySqlTask *task = new MySqlTask();

	base_logic::DictionaryValue* dict =(base_logic::DictionaryValue*)(value);
	bool r = dict->GetString(L"sql", &task->sql_);
  if (!r)
    return r;

  r = dict->GetShortInteger(L"thread_flag", &task->thread_flag_);
  if (!r)
    task->thread_flag_ = SYN;

  task->dict_ = dict;
  task->callbackfunc_ = storage_get;

  AddTask(task);
  return r;
}

}  // namespace base_logic



