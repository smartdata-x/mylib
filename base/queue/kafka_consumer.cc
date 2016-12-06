#include "kafka_consumer.h"
#include "logic/logic_comm.h"

kafka_consumer::kafka_consumer() {
  rk_ = NULL;
  rkt_ = NULL;
  partition_ = 0;
  msg_consume_ = 0;
}

kafka_consumer::~kafka_consumer() {
}

int kafka_consumer::Init(base::ConnAddr& addr) {
  return Init(atol(addr.additional().c_str()), addr.source().c_str(),
       addr.host().c_str(),NULL);
}

int kafka_consumer::Init(const int partition, const char* topic,
                         const char* brokers, MsgConsume msg_consume) {
  char err_str[512];
  partition_ = partition;
  msg_consume_ = msg_consume;

  rd_kafka_conf_t *conf = rd_kafka_conf_new();
  if (NULL == conf)
    return CONSUMER_INIT_FAILED;

  if (RD_KAFKA_CONF_OK
      != rd_kafka_conf_set(conf, "group.id", "one", err_str, sizeof(err_str)))
    return CONSUMER_INIT_FAILED;

  if (RD_KAFKA_CONF_OK
      != rd_kafka_conf_set(conf, "queued.min.messages", "1000000", NULL, 0))
    return CONSUMER_INIT_FAILED;

  if (RD_KAFKA_CONF_OK
      != rd_kafka_conf_set(conf , "auto.commit.interval.ms", "200", NULL, 0))
    return CONSUMER_INIT_FAILED;

  rk_ = rd_kafka_new(RD_KAFKA_CONSUMER, conf, err_str, sizeof(err_str));
  if (NULL == rk_){
    LOG_MSG2("rk_ error %s",err_str);
    return CONSUMER_INIT_FAILED;
  }

 // rd_kafka_conf_destroy(conf);


  rd_kafka_set_log_level(rk_, LOG_ERR);
  if (0 == rd_kafka_brokers_add(rk_, brokers))
    return CONSUMER_INIT_FAILED;

  rd_kafka_topic_conf_t *topic_conf = rd_kafka_topic_conf_new();

  rkt_ = rd_kafka_topic_new(rk_, topic, topic_conf);
  if (NULL == rkt_) {
    rd_kafka_destroy(rk_);
    return CONSUMER_INIT_FAILED;
  }

  if (rd_kafka_consume_start(rkt_, partition, RD_KAFKA_OFFSET_END) == -1) {
    rd_kafka_destroy(rk_);
    rd_kafka_topic_destroy(rkt_);
    return CONSUMER_INIT_FAILED;
  }

  return CONSUMER_INIT_SUCCESS;
}

int kafka_consumer::PullData(std::string& data) {
  rd_kafka_message_t *rkmessage = NULL;
  int consume_result = PULL_DATA_SUCCESS;
  //LOG_MSG2("rkt_=%p, partition_%d\n", rkt_, partition_);
  rd_kafka_poll(rk_, 0);
  rkmessage = rd_kafka_consume(rkt_, partition_, 100);
  if (!rkmessage) {
    return PULL_DATA_DATA_NULL;
  }
  if (rkmessage->err) {
    if (rkmessage->err == RD_KAFKA_RESP_ERR__UNKNOWN_PARTITION
        || rkmessage->err == RD_KAFKA_RESP_ERR__UNKNOWN_TOPIC) {
      //LOG_MSG2("kafka message error %s",rd_kafka_err2str(rkmessage->err));
      return rkmessage->err;
    } //LOG_MSG2("kafka message error %s",rd_kafka_err2str(rkmessage->err));
    return rkmessage->err;
  } //LOG_MSG2("payload%s,len%d offset=%lld\n", (const char*)rkmessage->payload, (int)rkmessage->len, rkmessage->offset);
  data = std::string((const char*) rkmessage->payload, rkmessage->len);
  rd_kafka_message_destroy(rkmessage);

  return consume_result;
}

int kafka_consumer::PullDataBatch(std::list<std::string>* list) {
  rd_kafka_message_t *rkmessage = NULL;
  int consume_result = PULL_DATA_SUCCESS;
  int error_code;
  std::string error_str;
  std::string data;
  rd_kafka_poll(rk_, 0);
  ssize_t msg_num = rd_kafka_consume_batch(rkt_, partition_, 100, &rkmessage,
                                           MAX_MSG_NUM_PER_TIME);

  if (!rkmessage) {
    return PULL_DATA_DATA_NULL;
  }

  if (rkmessage->err) {
    if (rkmessage->err == RD_KAFKA_RESP_ERR__UNKNOWN_PARTITION
        || rkmessage->err == RD_KAFKA_RESP_ERR__UNKNOWN_TOPIC) {
      LOG_ERROR2("kafka message error %s",rd_kafka_err2str(rkmessage->err));
      return rkmessage->err;
    } LOG_ERROR2("kafka message error %s",rd_kafka_err2str(rkmessage->err));
    return rkmessage->err;
  }

  if (-1 == msg_num)
    return PULL_DATA_FAILED;
  else {
    for (ssize_t i = 0; i < msg_num; i++) {
      if (1 > rkmessage->len)
        continue;
      data = std::string((const char*) rkmessage->payload, rkmessage->len);
      list->push_back(data);
    }
  }
  return PULL_DATA_SUCCESS;
}

void kafka_consumer::Close() {
  rd_kafka_consume_stop(rkt_, partition_);
  while (rd_kafka_outq_len(rk_) > 0)
    rd_kafka_poll(rk_, 10);
  rd_kafka_topic_destroy(rkt_);
  rkt_ = NULL;
  rd_kafka_destroy(rk_);
  rk_ = NULL;
}

int kafka_consumer::PullDataAndDeserialize(base_logic::Value* &value,
                                           int& error_code,
                                           std::string& error_str) {
  std::string data;
  int re = PullData(data);
  if (PULL_DATA_SUCCESS == re) {
    base_logic::ValueSerializer* engine = base_logic::ValueSerializer::Create(
        0, &data);
    value = engine->Deserialize(&error_code, &error_str);
  }
  return re;
}

int kafka_consumer::PullDataCallback() {
  int re = rd_kafka_consume_callback(rkt_, partition_, 1000, msg_consume_,
                                     NULL);

  if (re == -1)
    return PULL_DATA_FAILED;
  rd_kafka_poll(rk_, 0);

  return PULL_DATA_SUCCESS;
}

