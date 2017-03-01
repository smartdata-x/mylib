#include "kafka_producer.h"

int kafka_producer::state_ = 0;

kafka_producer::kafka_producer() {
  partition_ = 0;
  //conf_ = NULL;
  //topic_conf_ = NULL;
  rk_ = NULL;
  rkt_ = NULL;
  engine_ = NULL;
}

kafka_producer::~kafka_producer() {
}

int kafka_producer::Init(base::ConnAddr& addr) {
  return Init(addr.additional().empty()?0:atol(addr.additional().c_str()),
              addr.source().c_str(), addr.host().c_str(),NULL);
}


int kafka_producer::Init(const int partition, const char* topic,
                         const char* brokers, MsgDelivered func_msg_delivered) {
  char errstr[512];

  rd_kafka_conf_t* conf = rd_kafka_conf_new();
  if (NULL == conf)
    return PRODUCER_INIT_FAILED;

  if (RD_KAFKA_CONF_OK
      != rd_kafka_conf_set(conf, "message.max.bytes", "1024 * 1024", NULL, 0))
    return PRODUCER_INIT_FAILED;
  if (RD_KAFKA_CONF_OK
      != rd_kafka_conf_set(conf, "queue.buffering.max.messages", "500000",
                           NULL, 0))
    return PRODUCER_INIT_FAILED;
  if (RD_KAFKA_CONF_OK
      != rd_kafka_conf_set(conf, "queued.min.messages", "1000000", NULL, 0))
    return PRODUCER_INIT_FAILED;
  if (RD_KAFKA_CONF_OK
      != rd_kafka_conf_set(conf, "message.send.max.retries", "1", NULL, 0))
    return PRODUCER_INIT_FAILED;
  if (RD_KAFKA_CONF_OK
      != rd_kafka_conf_set(conf, "retry.backoff.ms", "500", NULL, 0))
    return PRODUCER_INIT_FAILED;
  if (RD_KAFKA_CONF_OK
      != rd_kafka_conf_set(conf, "compression.codec", "snappy", NULL, 0))
    return PRODUCER_INIT_FAILED;
  if (NULL != func_msg_delivered)
    rd_kafka_conf_set_dr_cb(conf, func_msg_delivered);

  rd_kafka_conf_set_log_cb(conf, kafka_producer::Logger);

  if (!(rk_ = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof(errstr))))
    return PRODUCER_INIT_FAILED;

  //rd_kafka_conf_destroy(conf);

  if (rd_kafka_brokers_add(rk_, brokers) == 0) {
    rd_kafka_destroy(rk_);
    return PRODUCER_INIT_FAILED;
  }
  rd_kafka_set_log_level(rk_, LOG_ERR);

  rd_kafka_topic_conf_t* topic_conf = rd_kafka_topic_conf_new();

  rkt_ = rd_kafka_topic_new(rk_, topic, topic_conf);
  //rd_kafka_topic_conf_destroy(topic_conf);
  if (NULL == rkt_) {
    rd_kafka_destroy(rk_);
    return PRODUCER_INIT_FAILED;
  }

  return PRODUCER_INIT_SUCCESS;
}

int kafka_producer::PushData(const char* buf, const int buf_len) {
  int produce_ret = PUSH_DATA_SUCCESS;
  if (NULL == buf)
    return INPUT_DATA_ERROR;
  if (0 == buf_len || buf_len > MAX_DATA_SIZE)
    return INPUT_DATA_ERROR;

  produce_ret = rd_kafka_produce(rkt_, partition_, RD_KAFKA_MSG_F_COPY,
                                 (void*) buf, (size_t) buf_len, NULL, 0, NULL);
  if (PUSH_DATA_FAILED == produce_ret)
    return PUSH_DATA_FAILED;
  rd_kafka_poll(rk_, 0);
  return PUSH_DATA_SUCCESS;
}

int kafka_producer::PushData(base_logic::Value* data) {
  std::string json_str;
  scoped_ptr<base_logic::ValueSerializer> engine(base_logic::ValueSerializer::Create(
      0, &json_str));
  engine->Serialize(*data);
  return PushData(json_str.c_str(), json_str.size());
}

void kafka_producer::Close() {
  rd_kafka_topic_destroy(rkt_);
  rkt_ = NULL;
  rd_kafka_destroy(rk_);
  rk_ = NULL;
}

void kafka_producer::Logger(const rd_kafka_t *rk, int level, const char *fac,
                            const char *buf) {
  fprintf(stderr, "kafka_producer::logger:level=%i,fac=%s,name=%s,buf=%s\n",
          level, fac, rk ? rd_kafka_name(rk) : NULL, buf);
  state_ = FETCH_DATA_FAILED;
}

int kafka_producer::CheckState() {
  return state_;
}
