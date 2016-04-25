//  Copyright (c) 2015-2015 The KID Authors. All rights reserved.
//  Created on: 2015骞�9鏈�17鏃� Author: kerry

#include <pthread.h>

#include "database/storage/storage_controller_engine.h"
#include "database/storage/mysql_controller.h"
#include "database/db/base_db_mysql_auto.h"
#include "logic/logic_comm.h"

namespace base_logic {

DataControllerEngine *DataControllerEngine::MySqlEngine_ = NULL;

DataControllerEngine* DataControllerEngine::Create(int32 type) {
    DataControllerEngine* engine = NULL;

    switch (type) {
      case MYSQL_TYPE: {
        if (MySqlEngine_ == NULL) {
          MySqlEngine_ = new MysqlController();
          MysqlController::StartSqlThread(MySqlEngine_);
    	  }

        engine = MySqlEngine_;
        break;
      }
      default:
        break;
    }
    return engine;
}

void DataControllerEngine::Init(config::FileConfig* config) {
    base_db::MysqlDBPool::Init(config->mysql_db_list_);
}

void DataControllerEngine::Dest() {
    base_db::MysqlDBPool::Dest();
}

}  // namespace base_logic



