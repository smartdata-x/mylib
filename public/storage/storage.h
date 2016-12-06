#ifndef MIG_FM_PUBLIC_STORAGE_STORAGE_H__
#define MIG_FM_PUBLIC_STORAGE_STORAGE_H__
#include <string>
#include <list>
#include <vector>
#include <map>
#include "basic/basictypes.h"
#include "config/config_info.h"

namespace base_storage{

enum StorageImplType{
	IMPL_DIC = 0,
	IMPL_DB = 1
};

enum DictionaryImplType{
   IMPL_MEM = 0,
   IMPL_RADIES = 1
};

enum DBImplType{
	IMPL_MYSQL = 0,
	IMPL_MSSQL = 1
};


enum ParamType{
	PARAM_IN = 0,
	PARAM_OUT =1
};

enum VarType{
	TYPE_INT8  = 0,
	TYPE_INT16 = 1,
	TYPE_INT32 = 2,
	TYPE_INT64 = 3,
	TYPE_CHAR = 4,
};

typedef struct db_conn_t{
    void *proc;
}db_conn_t;


typedef struct db_res_t{
    void *proc;
}db_res_t;

typedef struct db_row_t{
    void *proc;
}db_row_t;

struct CommandReply {
	enum ReplyType {
		REPLY_UNKNOWN,
		REPLY_STRING,
		REPLY_ARRAY,
		REPLY_INTEGER,
		REPLY_NIL,
		REPLY_STATUS,
		REPLY_ERROR,
	};

	int		type; // see enum ReplyType

	bool IsError() const { return REPLY_ERROR==type; }
	bool IsNil() const { return REPLY_NIL==type; }

	CommandReply(int _type = REPLY_UNKNOWN)
		: type(_type) {}
	virtual ~CommandReply() {}
	virtual void Release() { delete this; }
};

template <class InnerType, int rpl>
struct TypedCommandReply : public CommandReply {
	typedef InnerType value_type;

	value_type value;

	TypedCommandReply(const value_type &val=value_type())
		: CommandReply(rpl), value(val) {}
	virtual void Release() { delete this; }
};

typedef TypedCommandReply<int64, CommandReply::REPLY_INTEGER> IntegerReply;
typedef TypedCommandReply<std::string, CommandReply::REPLY_STRING> StringReply;
typedef TypedCommandReply<std::string, CommandReply::REPLY_ERROR> ErrorReply;
typedef TypedCommandReply<std::string, CommandReply::REPLY_STATUS> StatusReply;

struct ArrayReply : public CommandReply {
	typedef std::vector<CommandReply *> value_type;
	typedef value_type::iterator iterator;
	typedef value_type::const_iterator const_iterator;

	value_type value;

	ArrayReply() : CommandReply(REPLY_ARRAY) {}

	virtual void Release() { Clear(); delete this; }
	void Clear() {
		for (iterator it=value.begin(); it!=value.end(); ++it) {
			(*it)->Release();
		}
		value.clear();
	}
};

class StorageEngine{
public:
	static StorageEngine* Create(int32 type);
public:
	virtual bool Connections(std::list<base::ConnAddr>& addrlist) = 0;
	virtual bool Release() = 0;
};

class DBStorageEngine:public StorageEngine{
public:
	static DBStorageEngine* Create(int32 type);
	virtual ~DBStorageEngine(){}

public:
	virtual bool Release() = 0;
	
	virtual bool SQLExec(const char* sql) = 0; //执行语句
	
  virtual bool SQLExecs(std::list<std::string>& sqls) = 0;

	virtual bool StoredProcedure() = 0; //执行存储过程

	virtual bool AddSPName(const char* sp_name) = 0;//添加存储过程名

	virtual bool AddSPParam(const int32 var,const int32 type,const char* name,
	        void* param,const int32 outstrlen = 0) = 0; //添加参数

	virtual bool CheckAffect(const int32 index,const char* name) = 0;

	virtual bool Affected(unsigned long& rows) = 0;
	
	virtual uint32 RecordCount() = 0;
	
  virtual bool FreeRes() = 0;
    
	virtual db_row_t* FetchRows(void) = 0;//get Recordset
	
	virtual bool CheckConnect(void) = 0;
	
	virtual char* GetEntry(db_row_t *row,int cloidx) = 0;

	virtual char* GetResult(int colidx) = 0;

};

class DictionaryStorageEngine:public StorageEngine{

public:
    static DictionaryStorageEngine* Create(int32 type);
    virtual ~DictionaryStorageEngine(){}

public:
	
    virtual bool  Release() = 0;

    virtual bool CheckConnect(void) = 0;
 
    virtual bool  SetValue(const char* key,const size_t key_len,
    						const char* val,const size_t val_len) = 0;

    virtual bool  AddValue(const char* key,const size_t key_len,
    						const char* val,const size_t val_len) = 0;

    virtual bool  ReplaceValue(const char* key,const size_t key_len,
    							const char* val,const size_t val_len) = 0;

    virtual bool  GetValue(const char* key,const size_t key_len,
    						char** val,size_t* val_len) = 0;

    virtual bool  DelValue(const char* key,const size_t key_len) = 0;

    virtual bool  MGetValue(const char* const * key_array,const size_t *key_len_array,
    						size_t element_count) = 0;

    virtual bool FetchValue(char* key,size_t *key_len,char** value,size_t *val_len) = 0;

    virtual bool IncDecValue(const char* key, size_t key_len, int64 incby, int64 &result) = 0;

	virtual bool IncrValue(const char* key,const size_t key_len,
		                   const char* val,const size_t val_len) = 0;

	virtual bool DecrValue(const char* key,const size_t key_len,
		                   const char* val,const size_t val_len) = 0;
        //list 1Ϊleft ��ͷ 0 Ϊright ��β
    virtual bool AddListElement(const char* key,const size_t key_len,
    						    const char* val,const size_t val_len,
								const int flag) = 0;
    
    virtual bool GetListElement (const char* key,const size_t key_len,
    								const int index,char** val,size_t *val_len) = 0;
    
    virtual bool DelListElement(const char* key,const size_t key_len,const int index) = 0;
    
    virtual bool SetListElement(const int index,const char* key,const size_t key_len,
    								const char* val,const size_t val_len) =0 ;
    
    virtual bool PopListElement(const char* key,const size_t key_len,char** val,
    		size_t *val_len,const int flag) = 0;

	virtual bool AddHashRadomElement(const char* hash_name,const char* val,
									const size_t val_len) = 0;

	virtual bool GetHashRadomElement(const char* hash_name,char** val,size_t *val_len,
		                              const int radom_num) = 0;

	virtual bool DelHashRadomElement(const char* hash_name) = 0;

	virtual bool AddHashElement(const char* hash_name,const char* key,const size_t key_len,
								const char* val,const size_t val_len) = 0;

	virtual bool SetHashElement(const char* hash_name,const char* key,const size_t key_len,
								const char* val,const size_t val_len) = 0;

	virtual bool GetHashElement(const char* hash_name,const char* key,const size_t key_len,
								char** val,size_t *val_len) = 0;

	virtual bool DelHashElement(const char* hash_name,const char* key,const size_t key_len) =0;

	virtual int  GetHashSize(const char* hash_name) = 0;

	virtual bool GetListAll(const char* key,const size_t key_len,std::list<std::string>& list) = 0;

	virtual int GetListSize(const char* list_name) = 0;

	virtual bool GetHashValues(const char* hash_name,
		                       const size_t hash_name_len,
                           std::list<std::string>& list) = 0;
  virtual bool GetHashKeys(const char* hash_name,
                            const size_t hash_name_len,
                            std::list<std::string>& list) = 0;
	virtual bool GetAllHash(const char* hash_name,
		                    const size_t hash_name_len,
							std::map<std::string,std::string>& map) = 0;

  virtual bool GetSortedSet(const char* hash_name,
                         const size_t hash_name_len,
                         const char *head,
                         const char *tail,
                         int order,
                         std::map<std::string,std::string>& map) = 0;

	 //1Ϊleft ��ͷ 0 Ϊright ��β
	virtual bool GetListRange(const char* key,const size_t key_len,
				int from, int to, std::list<std::string>& list,
				const int flag) = 0;

	virtual CommandReply *DoCommand(const char *format/*, ...*/) = 0;
  virtual bool DoCommands(std::list<std::string> command_list, std::list<CommandReply*> &reply_list);
  // 通用接口命令
  virtual CommandReply *DoCommandV(const char *format, ...){return NULL;};
  virtual int AppendCommandV(const char *format, ...) {return 0;}
  virtual bool GetPipleReply(size_t cmd_size, std::list<CommandReply*> &reply_list_out){return true;}

	virtual void *GetContext() {}
};

}
#endif
