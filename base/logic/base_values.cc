#include "base_values.h"
#include "logic/value_serializer.h"
#include "logic/xml_serializer.h"
#include "logic/json_serializer.h"
#include "logic/jsonp_serializer.h"
#include "logic/http_serializer.h"
#include "basic/basic_util.h"
#include "check/newmacro.h"

namespace base_logic {

Value* CopyWithoutEmptyChildren(Value* node) {

  switch (node->GetType()) {
    case Value::TYPE_LIST: {
      ListValue* list = static_cast<ListValue*>(node);
      ListValue* copy = new ListValue;
      for (ListValue::const_iterator it = list->begin(); it != list->end();
          ++it) {
        Value* child_copy = CopyWithoutEmptyChildren(*it);
        if (child_copy)
          copy->Append(child_copy);
      }
      if (!copy->empty())
        return copy;

      delete copy;
      return NULL;
    }

    case Value::TYPE_DICTIONARY: {
      DictionaryValue* dict = static_cast<DictionaryValue*>(node);
      DictionaryValue* copy = new DictionaryValue;
      for (DictionaryValue::key_iterator it = dict->begin_keys();
          it != dict->end_keys(); ++it) {
        Value* child = NULL;
        bool rv = dict->GetWithoutPathExpansion(*it, &child);
        Value* child_copy = CopyWithoutEmptyChildren(child);
        if (child_copy)
          copy->SetWithoutPathExpansion(*it, child_copy);
      }
      if (!copy->empty())
        return copy;

      delete copy;
      return NULL;
    }

    default:
      return node->DeepCopy();
  }
}

///////////////////// BaseValue ////////////////////

Value::~Value() {

}

//static
Value* Value::CreateNullValue() {
  return new Value(TYPE_NULL);
}

//static
Value* Value::CreateBooleanValue(bool in_value) {
  return new FundamentalValue(in_value);
}

//static
Value* Value::CreateCharIntegerValue(int8 in_value) {
  return new FundamentalValue(in_value);
}

//static
Value* Value::CreateShortIntegerValue(int16 in_value) {
  return new FundamentalValue(in_value);
}

//static
Value* Value::CreateIntegerValue(int32 in_value) {
  return new FundamentalValue(in_value);
}

//static
Value* Value::CreateBigIntegerValue(int64 in_value) {
  return new FundamentalValue(in_value);
}
//static
Value* Value::CreateRealValue(double in_value) {
  return new FundamentalValue(in_value);
}

// static
Value* Value::CreateStringValue(const std::string& in_value) {
  return new StringValue(in_value);
}

Value* Value::CreateStringValue(const std::wstring& in_value) {
  return new StringValue(in_value);
}

// static
BinaryValue* Value::CreateBinaryValue(char* buffer, size_t size) {
  return BinaryValue::Create(buffer, size);
}

bool Value::GetAsBoolean(bool* out_value) const {
  return false;
}

bool Value::GetAsCharInteger(int8* out_value) const {
  return false;
}

bool Value::GetAsShortInteger(int16* out_value) const {
  return false;
}
bool Value::GetAsInteger(int32* out_value) const {
  return false;
}

bool Value::GetAsBigInteger(int64* out_value) const {
  return false;
}

bool Value::GetAsReal(double* out_value) const {
  return false;
}

bool Value::GetAsString(std::string* out_value) const {
  return false;
}

bool Value::GetAsString(std::wstring* out_value) const {
  return false;
}

Value* Value::DeepCopy() const {
  return CreateNullValue();
}

bool Value::Equals(const Value* other) const {
  return other->IsType(TYPE_NULL);
}
///////////////////// FundamentalValue ////////////////////
FundamentalValue::~FundamentalValue() {

}

bool FundamentalValue::GetAsBoolean(bool* out_value) const {
  if (out_value && IsType(TYPE_BOOLEAN))
    *out_value = boolean_value_;
  return (IsType(TYPE_BOOLEAN));
}

bool FundamentalValue::GetAsCharInteger(int8* out_value) const {
  if (out_value && IsType(TYPE_CHAR_INTEGER))
    *out_value = char_integer_value_;
  return (IsType(TYPE_CHAR_INTEGER));
}

bool FundamentalValue::GetAsShortInteger(int16* out_value) const {
  if (out_value && IsType(TYPE_SHORT_INTEGER))
    *out_value = short_integer_value_;
  return (IsType(TYPE_SHORT_INTEGER));
}

bool FundamentalValue::GetAsInteger(int32* out_value) const {
  if (out_value && IsType(TYPE_INTEGER))
    *out_value = integer_value_;
  return (IsType(TYPE_INTEGER));
}

bool FundamentalValue::GetAsReal(double* out_value) const {
  if (out_value && IsType(TYPE_REAL))
    *out_value = real_value_;
  return (IsType(TYPE_REAL));
}

bool FundamentalValue::GetAsBigInteger(int64* out_value) const {
  if (out_value && IsType(TYPE_BIG_INTEGER))
    *out_value = big_integer_value_;
  return (IsType(TYPE_BIG_INTEGER));
}

Value* FundamentalValue::DeepCopy() const {
  switch (GetType()) {
    case TYPE_BOOLEAN:
      return CreateBooleanValue(boolean_value_);

    case TYPE_CHAR_INTEGER:
      return CreateCharIntegerValue(char_integer_value_);

    case TYPE_SHORT_INTEGER:
      return CreateShortIntegerValue(short_integer_value_);

    case TYPE_INTEGER:
      return CreateIntegerValue(integer_value_);

    case TYPE_REAL:
      return CreateRealValue(real_value_);

    case TYPE_BIG_INTEGER:
      return CreateBigIntegerValue(big_integer_value_);

    default:
      return NULL;
  }
}

bool FundamentalValue::Equals(const Value* other) const {
  if (other->GetType() != GetType())
    return false;

  switch (GetType()) {
    case TYPE_BOOLEAN: {
      bool lhs, rhs;
      return GetAsBoolean(&lhs) && other->GetAsBoolean(&rhs) && lhs == rhs;
    }
    case TYPE_CHAR_INTEGER: {
      int8 lhs, rhs;
      return GetAsCharInteger(&lhs) && other->GetAsCharInteger(&rhs)
          && lhs == rhs;
    }
    case TYPE_SHORT_INTEGER: {
      int16 lhs, rhs;
      return GetAsShortInteger(&lhs) && other->GetAsShortInteger(&rhs)
          && lhs == rhs;
    }
    case TYPE_INTEGER: {
      int lhs, rhs;
      return GetAsInteger(&lhs) && other->GetAsInteger(&rhs) && lhs == rhs;
    }
    case TYPE_BIG_INTEGER: {
      int64 lhs, rhs;
      return GetAsBigInteger(&lhs) && other->GetAsBigInteger(&rhs) && lhs == rhs;
    }
    case TYPE_REAL: {
      double lhs, rhs;
      return GetAsReal(&lhs) && other->GetAsReal(&rhs) && lhs == rhs;
    }
    default:
      return false;
  }
}

/////////////StringValue/////////////////
StringValue::StringValue(const std::string& in_value)
    : Value(TYPE_STRING),
      value_(in_value) {
}

StringValue::StringValue(const std::wstring& in_value)
    : Value(TYPE_STRING),
      value_(base::BasicUtil::StringConversions::WideToUTF8(in_value)) {
}

StringValue::~StringValue() {
}

bool StringValue::GetAsString(std::string* out_value) const {
  if (out_value)
    *out_value = value_;
  return true;
}

bool StringValue::GetAsString(std::wstring* out_value) const {
  if (out_value)
    *out_value = base::BasicUtil::StringConversions::UTF8ToWide(value_);
  return true;
}

Value* StringValue::DeepCopy() const {
  return CreateStringValue(value_);
}

bool StringValue::Equals(const Value* other) const {
  if (other->GetType() != GetType())
    return false;
  std::string lhs, rhs;
  return GetAsString(&lhs) && other->GetAsString(&rhs) && lhs == rhs;
}

/////////////BinaryValue/////////////////
//static
BinaryValue* BinaryValue::Create(char* buffer, size_t size) {
  if (!buffer)
    return NULL;
  return new BinaryValue(buffer, size);
}

//static
BinaryValue* BinaryValue::CreateWithCopiedBuffer(const char* buffer,
                                                 size_t size) {
  if (!buffer)
    return NULL;
  char* buffer_copy = new char[size];
  memcpy(buffer_copy, buffer, size);
  return new BinaryValue(buffer_copy, size);
}

BinaryValue::BinaryValue(char* buffer, size_t size)
    : Value(TYPE_BINARY),
      buffer_(buffer),
      size_(size) {

}

BinaryValue::~BinaryValue() {

}
Value* BinaryValue::DeepCopy() const {
  return CreateWithCopiedBuffer(buffer_, size_);
}

bool BinaryValue::Equals(const Value* other) const {
  if (other->GetType() != GetType())
    return false;
  const BinaryValue* other_binary = static_cast<const BinaryValue*>(other);
  if (other_binary->size_ != size_)
    return false;
  return !memcmp(buffer_, other_binary->buffer_, size_);
}

///////////////////// DictionaryValue ////////////////////
DictionaryValue::~DictionaryValue() {
  Clear();
}

Value* DictionaryValue::DeepCopy() const {
  DictionaryValue* result = new DictionaryValue;
  return result;
}

bool DictionaryValue::Equals(const Value* other) const {
  if (other->GetType() != GetType())
    return false;
  return true;
}

bool DictionaryValue::HasKey(const std::wstring& key) const {
  ValueMap::const_iterator current_entry = dictionary_.find(key);
  return current_entry != dictionary_.end();
}

bool DictionaryValue::HasKeyASCII(const std::string& key) const {
  return HasKey(base::BasicUtil::StringConversions::ASCIIToWide(key));
}

void DictionaryValue::Clear() {
  ValueMap::iterator dict_iterator = dictionary_.begin();
  while (dict_iterator != dictionary_.end()) {
    delete dict_iterator->second;
    dict_iterator->second = NULL;
    ++dict_iterator;
  }
  dictionary_.clear();
}

void DictionaryValue::Set(const std::string& path, Value* in_value) {
  if (in_value == NULL)
    return;
  std::wstring wpath = base::BasicUtil::StringConversions::UTF8ToWide(path);
  Set(wpath, in_value);
}

void DictionaryValue::SetBoolean(const std::string& path, bool in_value) {
  Set(path, CreateBooleanValue(in_value));
}

void DictionaryValue::SetCharInteger(const std::string& path, int8 in_value) {
  Set(path, CreateCharIntegerValue(in_value));
}
void DictionaryValue::SetShortInteger(const std::string& path, int16 in_value) {
  Set(path, CreateShortIntegerValue(in_value));
}

void DictionaryValue::SetInteger(const std::string& path, int32 in_value) {
  Set(path, CreateIntegerValue(in_value));
}

void DictionaryValue::SetBigInteger(const std::string& path, int64 in_value) {
  Set(path, CreateBigIntegerValue(in_value));
}

void DictionaryValue::SetReal(const std::string& path, double in_value) {
  Set(path, CreateRealValue(in_value));
}

void DictionaryValue::SetString(const std::string& path,
                                const std::string& in_value) {
  Set(path, CreateStringValue(in_value));
}

void DictionaryValue::SetString(const std::string& path,
                                const std::wstring& in_value) {
  Set(path, CreateStringValue(in_value));
}

void DictionaryValue::Set(const std::wstring& path, Value* in_value) {
  if (in_value == NULL)
    return;
  std::wstring current_path(path);
  DictionaryValue* current_dictionary = this;
  //查找value 对应的MAP深度
  for (size_t delimiter_position = current_path.find('.');
      delimiter_position != std::wstring::npos; delimiter_position =
          current_path.find('.')) {
    std::wstring key(current_path, 0, delimiter_position);
    DictionaryValue* child_dictionary = NULL;
    if (!current_dictionary->GetDictionary(key, &child_dictionary)) {
      child_dictionary = new DictionaryValue;
      current_dictionary->SetWithoutPathExpansion(key, child_dictionary);
    }
    current_dictionary = child_dictionary;
    current_path.erase(0, delimiter_position + 1);
  }
  current_dictionary->SetWithoutPathExpansion(current_path, in_value);
}

void DictionaryValue::SetBoolean(const std::wstring& path, bool in_value) {
  Set(path, CreateBooleanValue(in_value));
}

void DictionaryValue::SetCharInteger(const std::wstring& path, int8 in_value) {
  Set(path, CreateCharIntegerValue(in_value));
}
void DictionaryValue::SetShortInteger(const std::wstring& path,
                                      int16 in_value) {
  Set(path, CreateShortIntegerValue(in_value));
}
void DictionaryValue::SetInteger(const std::wstring& path, int32 in_value) {
  Set(path, CreateIntegerValue(in_value));
}

void DictionaryValue::SetReal(const std::wstring& path, double in_value) {
  Set(path, CreateRealValue(in_value));
}

void DictionaryValue::SetBigInteger(const std::wstring& path, int64 in_value) {
  Set(path, CreateBigIntegerValue(in_value));
}

void DictionaryValue::SetString(const std::wstring& path,
                                const std::string& in_value) {
  Set(path, CreateStringValue(in_value));
}

void DictionaryValue::SetString(const std::wstring& path,
                                const std::wstring& in_value) {
  Set(path, CreateStringValue(in_value));
}

void DictionaryValue::SetWithoutPathExpansion(const std::wstring& key,
                                              Value* in_value) {

  if (in_value == NULL)
    return;
  if (HasKey(key)) {
    delete dictionary_[key];
  }

  dictionary_[key] = in_value;
}

bool DictionaryValue::Get(const std::wstring& path, Value** out_value) const {
  std::wstring current_path(path);
  const DictionaryValue* current_dictionary = this;
  for (size_t delimiter_position = current_path.find('.');
      delimiter_position != std::string::npos;
      delimiter_position = current_path.find('.')) {
    DictionaryValue* child_dictionary = NULL;
    if (!current_dictionary->GetDictionary(
        current_path.substr(0, delimiter_position), &child_dictionary))
      return false;
    current_dictionary = child_dictionary;
    current_path.erase(0, delimiter_position + 1);
  }
  return current_dictionary->GetWithoutPathExpansion(current_path, out_value);
}

bool DictionaryValue::GetBoolean(const std::wstring& path,
                                 bool* bool_value) const {
  Value* value;
  if (!Get(path, &value))
    return false;

  return value->GetAsBoolean(bool_value);
}

bool DictionaryValue::GetCharInteger(const std::wstring& path,
                                     int8* out_value) const {
  Value* value;
  if (!Get(path, &value))
    return false;
  return value->GetAsCharInteger(out_value);
}

bool DictionaryValue::GetShortInteger(const std::wstring& path,
                                      int16* out_value) const {
  Value* value;
  if (!Get(path, &value))
    return false;
  return value->GetAsShortInteger(out_value);
}

bool DictionaryValue::GetInteger(const std::wstring& path,
                                 int32* out_value) const {
  Value* value;
  if (!Get(path, &value))
    return false;

  return value->GetAsInteger(out_value);
}

bool DictionaryValue::GetBigInteger(const std::wstring& path,
                                    int64* out_value) const {
  Value* value;
  if (!Get(path, &value))
    return false;

  return value->GetAsBigInteger(out_value);
}

bool DictionaryValue::GetReal(const std::wstring& path,
                              double* out_value) const {
  Value* value;
  if (!Get(path, &value))
    return false;

  return value->GetAsReal(out_value);
}

bool DictionaryValue::GetStringASCII(const std::string& path,
                                     std::string* out_value) const {

  std::string out;
  if (!GetString(base::BasicUtil::StringConversions::ASCIIToWide(path), &out))
    return false;

  if (!base::BasicUtil::StringUtil::IsStringASCII(out))
    return false;

  out_value->assign(out);
  return true;
}

bool DictionaryValue::GetString(const std::wstring& path,
                                std::string* out_value) const {
  Value* value;
  if (!Get(path, &value))
    return false;

  return value->GetAsString(out_value);
}

bool DictionaryValue::GetString(const std::wstring& path,
                                std::wstring* out_value) const {
  Value* value;
  if (!Get(path, &value))
    return false;

  return value->GetAsString(out_value);
}

bool DictionaryValue::GetBinary(const std::wstring& path,
                                BinaryValue** out_value) const {
  Value* value;
  bool result = Get(path, &value);
  if (!result || !value->IsType(TYPE_BINARY))
    return false;

  if (out_value)
    *out_value = static_cast<BinaryValue*>(value);

  return true;
}

bool DictionaryValue::GetDictionary(const std::wstring& path,
                                    DictionaryValue** out_value) const {
  Value* value;
  bool result = Get(path, &value);
  if (!result || !value->IsType(TYPE_DICTIONARY))
    return false;

  if (out_value)
    *out_value = static_cast<DictionaryValue*>(value);

  return true;
}

bool DictionaryValue::GetList(const std::wstring& path,
                              ListValue** out_value) const {
  Value* value;
  bool result = Get(path, &value);
  if (!result || !value->IsType(TYPE_LIST))
    return false;

  if (out_value)
    *out_value = static_cast<ListValue*>(value);

  return true;
}

bool DictionaryValue::GetWithoutPathExpansion(const std::wstring& key,
                                              Value** out_value) const {

  ValueMap::const_iterator entry_iterator = dictionary_.find(key);
  if (entry_iterator == dictionary_.end())
    return false;

  Value* entry = entry_iterator->second;
  if (out_value)
    *out_value = entry;
  return true;
}

bool DictionaryValue::GetCharIntegerWithoutPathExpansion(
    const std::wstring& path, int8* out_value) const {
  Value* value;
  if (!GetWithoutPathExpansion(path, &value))
    return false;

  return value->GetAsCharInteger(out_value);
}

bool DictionaryValue::GetShortIntegerWithoutPathExpansion(
    const std::wstring& path, int16* out_value) const {
  Value* value;
  if (!GetWithoutPathExpansion(path, &value))
    return false;

  return value->GetAsShortInteger(out_value);
}

bool DictionaryValue::GetIntegerWithoutPathExpansion(const std::wstring& path,
                                                     int32* out_value) const {
  Value* value;
  if (!GetWithoutPathExpansion(path, &value))
    return false;

  return value->GetAsInteger(out_value);
}

bool DictionaryValue::GetBigIntegerWithoutPathExpansion(
    const std::wstring& path, int64* out_value) const {
  Value* value;
  if (!GetWithoutPathExpansion(path, &value))
    return false;

  return value->GetAsBigInteger(out_value);
}

bool DictionaryValue::GetStringWithoutPathExpansion(
    const std::wstring& path, std::string* out_value) const {
  Value* value;
  if (!GetWithoutPathExpansion(path, &value))
    return false;

  return value->GetAsString(out_value);
}

bool DictionaryValue::GetStringWithoutPathExpansion(
    const std::wstring& path, std::wstring* out_value) const {
  Value* value;
  if (!GetWithoutPathExpansion(path, &value))
    return false;

  return value->GetAsString(out_value);
}

bool DictionaryValue::GetDictionaryWithoutPathExpansion(
    const std::wstring& path, DictionaryValue** out_value) const {
  Value* value;
  bool result = GetWithoutPathExpansion(path, &value);
  if (!result || !value->IsType(TYPE_DICTIONARY))
    return false;

  if (out_value)
    *out_value = static_cast<DictionaryValue*>(value);

  return true;
}

bool DictionaryValue::GetListWithoutPathExpansion(const std::wstring& path,
                                                  ListValue** out_value) const {
  Value* value;
  bool result = GetWithoutPathExpansion(path, &value);
  if (!result || !value->IsType(TYPE_LIST))
    return false;

  if (out_value)
    *out_value = static_cast<ListValue*>(value);

  return true;
}

bool DictionaryValue::Remove(const std::wstring& path, Value** out_value) {
  std::wstring current_path(path);
  DictionaryValue* current_dictionary = this;
  size_t delimiter_position = current_path.rfind('.');
  if (delimiter_position != std::wstring::npos) {
    if (!GetDictionary(current_path.substr(0, delimiter_position),
                       &current_dictionary))
      return false;
    current_path.erase(0, delimiter_position + 1);
  }

  return current_dictionary->RemoveWithoutPathExpansion(current_path, out_value);
}

bool DictionaryValue::RemoveWithoutPathExpansion(const std::wstring& key,
                                                 Value** out_value) {
  ValueMap::iterator entry_iterator = dictionary_.find(key);
  if (entry_iterator == dictionary_.end())
    return false;

  Value* entry = entry_iterator->second;
  if (out_value)
    *out_value = entry;
  else
    delete entry;
  dictionary_.erase(entry_iterator);
  return true;
}

DictionaryValue* DictionaryValue::DeepCopyWithoutEmptyChildren() {
  Value* copy = CopyWithoutEmptyChildren(this);
  return copy ? static_cast<DictionaryValue*>(copy) : new DictionaryValue;
}

void DictionaryValue::MergeDictionary(const DictionaryValue* dictionary) {
  for (DictionaryValue::key_iterator key(dictionary->begin_keys());
      key != dictionary->end_keys(); ++key) {
    Value* merge_value;
    if (dictionary->GetWithoutPathExpansion(*key, &merge_value)) {
      // Check whether we have to merge dictionaries.
      if (merge_value->IsType(Value::TYPE_DICTIONARY)) {
        DictionaryValue* sub_dict;
        if (GetDictionaryWithoutPathExpansion(*key, &sub_dict)) {
          sub_dict->MergeDictionary(
              static_cast<const DictionaryValue*>(merge_value));
          continue;
        }
      }
      // All other cases: Make a copy and hook it up.
      SetWithoutPathExpansion(*key, merge_value->DeepCopy());
    }
  }
}

bool DictionaryValue::GetDicKey(std::vector<std::string> *vec) {
  if (NULL == vec) {
    return false;
  }

  vec->clear();
  ValueMap::iterator it = dictionary_.begin();
  for (; it != dictionary_.end(); ++it) {
    vec->push_back(base::BasicUtil::StringConversions::WideToASCII(it->first));
  }

  return true;
}

///////////////////// ListValue ////////////////////
ListValue::~ListValue() {
  Clear();
}

void ListValue::Clear() {
  for (ValueVector::iterator i(list_.begin()); i != list_.end(); ++i) {
    delete *i;
    *i = NULL;
  }
  list_.clear();
}

bool ListValue::Set(size_t index, Value* in_value) {
  if (in_value == NULL)
    return false;

  if (index >= list_.size()) {
    while (index > list_.size())
      Append(CreateNullValue());
    Append(in_value);
  } else {
    delete list_[index];
    list_[index] = in_value;
  }
  return true;
}

bool ListValue::Get(size_t index, Value** out_value) const {
  if (index >= list_.size())
    return false;

  if (out_value)
    *out_value = list_[index];

  return true;
}

bool ListValue::GetBoolean(size_t index, bool* bool_value) const {
  Value* value;
  if (!Get(index, &value))
    return false;

  return value->GetAsBoolean(bool_value);
}

bool ListValue::GetCharInteger(size_t index, int8* out_value) const {
  Value* value;
  if (!Get(index, &value))
    return false;

  return value->GetAsCharInteger(out_value);
}
bool ListValue::GetShortInteger(size_t index, int16* out_value) const {
  Value* value;
  if (!Get(index, &value))
    return false;

  return value->GetAsShortInteger(out_value);
}

bool ListValue::GetInteger(size_t index, int32* out_value) const {
  Value* value;
  if (!Get(index, &value))
    return false;

  return value->GetAsInteger(out_value);
}

bool ListValue::GetBigInteger(size_t index, int64* out_value) const {
  Value* value;
  if (!Get(index, &value))
    return false;

  return value->GetAsBigInteger(out_value);
}

bool ListValue::GetReal(size_t index, double* out_value) const {
  Value* value;
  if (!Get(index, &value))
    return false;

  return value->GetAsReal(out_value);
}

bool ListValue::GetString(size_t index, std::string* out_value) const {
  Value* value;
  if (!Get(index, &value))
    return false;

  return value->GetAsString(out_value);
}

bool ListValue::GetString(size_t index, std::wstring* out_value) const {
  Value* value;
  if (!Get(index, &value))
    return false;

  return value->GetAsString(out_value);
}

bool ListValue::GetBinary(size_t index, BinaryValue** out_value) const {
  Value* value;
  bool result = Get(index, &value);
  if (!result || !value->IsType(TYPE_BINARY))
    return false;

  if (out_value)
    *out_value = static_cast<BinaryValue*>(value);

  return true;
}

bool ListValue::GetDictionary(size_t index, DictionaryValue** out_value) const {
  Value* value;
  bool result = Get(index, &value);
  if (!result || !value->IsType(TYPE_DICTIONARY))
    return false;

  if (out_value)
    *out_value = static_cast<DictionaryValue*>(value);

  return true;
}

bool ListValue::GetList(size_t index, ListValue** out_value) const {
  Value* value;
  bool result = Get(index, &value);
  if (!result || !value->IsType(TYPE_LIST))
    return false;

  if (out_value)
    *out_value = static_cast<ListValue*>(value);

  return true;
}

bool ListValue::Remove(size_t index, Value** out_value) {
  if (index >= list_.size())
    return false;

  if (out_value)
    *out_value = list_[index];
  else
    delete list_[index];

  list_.erase(list_.begin() + index);
  return true;
}

int ListValue::Remove(const Value& value) {
  for (ValueVector::iterator i(list_.begin()); i != list_.end(); ++i) {
    if ((*i)->Equals(&value)) {
      size_t index = i - list_.begin();
      delete *i;
      list_.erase(i);

      int return_index = static_cast<int>(index);
      return return_index;
    }
  }
  return -1;
}

void ListValue::Append(Value* in_value) {
  if (in_value != NULL)
    list_.push_back(in_value);
}

bool ListValue::AppendIfNotPresent(Value* in_value) {
  if (in_value == NULL)
    return false;
  for (ValueVector::iterator i(list_.begin()); i != list_.end(); ++i) {
    if ((*i)->Equals(in_value))
      return false;
  }
  list_.push_back(in_value);
  return true;
}

bool ListValue::Insert(size_t index, Value* in_value) {

  if (in_value == NULL)
    return false;
  if (index > list_.size())
    return false;

  list_.insert(list_.begin() + index, in_value);
  return true;
}

Value* ListValue::DeepCopy() const {
  ListValue* result = new ListValue;

  for (ValueVector::const_iterator i(list_.begin()); i != list_.end(); ++i)
    result->Append((*i)->DeepCopy());

  return result;
}

bool ListValue::Equals(const Value* other) const {
  if (other->GetType() != GetType())
    return false;

  const ListValue* other_list = static_cast<const ListValue*>(other);
  const_iterator lhs_it, rhs_it;
  for (lhs_it = begin(), rhs_it = other_list->begin();
      lhs_it != end() && rhs_it != other_list->end(); ++lhs_it, ++rhs_it) {
    if (!(*lhs_it)->Equals(*rhs_it))
      return false;
  }
  if (lhs_it != end() || rhs_it != other_list->end())
    return false;

  return true;
}

ValueSerializer* ValueSerializer::Create(int32 type) {
  ValueSerializer* engine = NULL;
  switch (type) {
    case IMPL_JSON: {
      engine = new base_logic::JsonValueSerializer();
      break;
    }
    case IMPL_XML: {
      engine = new base_logic::XMLValueSerializer();
      break;
    }

    case IMPL_HTTP: {
      engine = new base_logic::HttpValueSerializer();
      break;
    }

    case IMPL_JSONP: {
      engine = new base_logic::JsonpValueSerializer();
      break;
    }
  }

  return engine;
}

ValueSerializer* ValueSerializer::Create(int32 type, std::string* str,
                                         bool pretty_print) {
  ValueSerializer* engine = NULL;
  switch (type) {
    case IMPL_JSON:
      engine = new base_logic::JsonValueSerializer(str, pretty_print);
      break;
    case IMPL_XML:
      engine = new base_logic::XMLValueSerializer(str, pretty_print);
      break;
    case IMPL_HTTP:
      engine = new base_logic::HttpValueSerializer(str, pretty_print);
      break;
    case IMPL_JSONP:
      engine = new base_logic::JsonpValueSerializer(str, pretty_print);
      break;
    default:
      break;
  }
  return engine;
}

void ValueSerializer::DeleteSerializer(int32 type,
                                       ValueSerializer* serializer) {
  if (NULL == serializer)
    return;
  switch (type) {
    case IMPL_JSON:
      delete (JsonValueSerializer*) serializer;
      break;
    case IMPL_XML:
      delete (XMLValueSerializer*) serializer;
      break;
    case IMPL_HTTP:
      delete (HttpValueSerializer*) serializer;
      break;
    case IMPL_JSONP:
      delete (JsonpValueSerializer*) serializer;
      break;
    default:
      break;
  }
}

}
