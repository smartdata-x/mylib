/*
 * xml_serializer.cc
 *
 *  Created on: 2015年6月18日
 *      Author: pro
 */

#include "xml_serializer.h"
#include "string_escape.h"
#include "logic/logic_comm.h"
#include "basic/basictypes.h"
#include "basic/basic_util.h"
#include "basic/scoped_ptr.h"
#include "logic/logic_comm.h"
#include <math.h>
#include <map>
#include <vector>

namespace base_logic {

static const Token kInvalidToken(Token::INVALID_TOKEN, 0, 0);
static const int kStackLimit = 100;

inline int HexToInt(wchar_t c) {
  if ('0' <= c && c <= '9') {
    return c - '0';
  } else if ('A' <= c && c <= 'F') {
    return c - 'A' + 10;
  } else if ('a' <= c && c <= 'f') {
    return c - 'a' + 10;
  }
  return 0;
}

bool XMLValueSerializer::NextStringMatch(const std::wstring& str) {
  for (size_t i = 0; i < str.length(); ++i) {
    if ('\0' == *xml_pos_)
      return false;
    if (*(xml_pos_ + i) != str[i])
      return false;
  }
  return true;
}

Token XMLValueSerializer::ParseNumberToken() {
  Token token(Token::NUMBER, xml_pos_, 0);
  wchar_t c = *xml_pos_;
  if ('-' == c) {  //负数
    ++token.length;
    c = token.NextChar();
  }

  if (!ReadInt(token, false))
    return kInvalidToken;

  //Optional fraction part
  c = token.NextChar();
  if ('.' == c) {
    ++token.length;
    if (!ReadInt(token, true))
      return kInvalidToken;
    c = token.NextChar();
  }

  //Optional exponent part
  if ('e' == c || 'E' == c) {
    ++token.length;
    c = token.NextChar();
    if ('-' == c || '+' == c) {
      ++token.length;
      c = token.NextChar();
    }
    if (!ReadInt(token, true))
      return kInvalidToken;
  }

  return token;
}

Token XMLValueSerializer::ParseStringToken() {
  Token token(Token::STRING, xml_pos_, 1);
  wchar_t c = token.NextChar();
  while ('\0' != c) {
    if ('\\' == c) {
      ++token.length;
      c = token.NextChar();
      // Make sure the escaped char is valid.
      switch (c) {
        case 'x':
          if (!ReadHexDigits(token, 2)) {
            SetErrorCode(INVALID_ESCAPE, xml_pos_ + token.length);
            return kInvalidToken;
          }
          break;
        case 'u':
          if (!ReadHexDigits(token, 4)) {
            SetErrorCode(INVALID_ESCAPE, xml_pos_ + token.length);
            return kInvalidToken;
          }
          break;
        case '\\':
        case '/':
        case 'b':
        case 'f':
        case 'n':
        case 'r':
        case 't':
        case 'v':
        case '"':
          break;
        default:
          SetErrorCode(INVALID_ESCAPE, xml_pos_ + token.length);
          return kInvalidToken;
      }
    } else if ('>' == c) {
      ++token.length;
      return token;
    }
    ++token.length;
    c = token.NextChar();
  }
  return kInvalidToken;
}

bool XMLValueSerializer::ReadInt(Token& token, bool can_have_leading_zeros) {
  wchar_t first = token.NextChar();
  int len = 0;

  // Read in more digits
  wchar_t c = first;
  while ('\0' != c && '0' <= c && c <= '9') {
    ++token.length;
    ++len;
    c = token.NextChar();
  }
  // We need at least 1 digit.
  if (len == 0)
    return false;

  if (!can_have_leading_zeros && len > 1 && '0' == first)
    return false;

  return true;
}

bool XMLValueSerializer::ReadHexDigits(Token& token, int digits) {
  for (int i = 1; i <= digits; ++i) {
    wchar_t c = *(token.begin + token.length + i);
    if ('\0' == c)
      return false;
    if (!(('0' <= c && c <= '9') || ('a' <= c && c <= 'f')
        || ('A' <= c && c <= 'F')))
      return false;
  }

  token.length += digits;
  return true;
}

Value* XMLValueSerializer::DecodeNumber(const Token& token) {
  const std::wstring num_wstring(token.begin, token.length);
  const std::string num_string =
      base::BasicUtil::StringConversions::WideToASCII(num_wstring);
  int32 num_int;
  //if(base::BasicUtil::StringUtil::StringToInt(num_string,&num_int))
  //return Value::CreateIntegerValue(num_int);

  int64 num_int64;
  if (base::BasicUtil::StringUtil::StringToInt64(num_string, &num_int64))
    return Value::CreateBigIntegerValue(num_int64);

  double num_double;

  if (base::BasicUtil::StringUtil::StringToDouble(num_string, &num_double)
      && finite(num_double))
    return Value::CreateRealValue(num_double);

  return NULL;
}

Value* XMLValueSerializer::DecodeString(const Token& token) {
  std::wstring decoded_str;
  decoded_str.reserve(token.length);
  for (int i = 0; i < token.length; ++i) {
    wchar_t c = *(token.begin + i);
    if ('\\' == c) {
      ++i;
      c = *(token.begin + i);
      switch (c) {
        case '"':
        case '/':
        case '\\':
          decoded_str.push_back(c);
          break;
        case 'b':
          decoded_str.push_back('\b');
          break;
        case 'f':
          decoded_str.push_back('\f');
          break;
        case 'n':
          decoded_str.push_back('\n');
          break;
        case 'r':
          decoded_str.push_back('\r');
          break;
        case 't':
          decoded_str.push_back('\t');
          break;
        case 'v':
          decoded_str.push_back('\v');
          break;
        case 'x':
          decoded_str.push_back(
              (HexToInt(*(token.begin + i + 1)) << 4)
                  + HexToInt(*(token.begin + i + 2)));
          i += 2;
          break;

        case 'u':
          decoded_str.push_back(
              (HexToInt(*(token.begin + i + 1)) << 12)
                  + (HexToInt(*(token.begin + i + 2)) << 8)
                  + (HexToInt(*(token.begin + i + 3)) << 4)
                  + HexToInt(*(token.begin + i + 4)));
          i += 4;
          break;

        default:
          // We should only have valid strings at this point.  If not,
          // ParseStringToken didn't do it's job.
          return NULL;
      }
    } else {
      decoded_str.push_back(c);
    }
  }

  return Value::CreateStringValue(decoded_str);
}

static const char kpretty_print_line_ending[] = "\n";


XMLValueSerializer::XMLValueSerializer()
:pretty_print_(false),
 xml_string_(NULL){}

XMLValueSerializer::XMLValueSerializer(std::string* xml, bool pretty_print)
    : pretty_print_(pretty_print),
      xml_string_(xml) {

}

XMLValueSerializer::~XMLValueSerializer() {

}

//  static
//  有限状态机取出属性
bool XMLValueSerializer::GetXmlAttribute(
    std::string &key, std::map<std::string, std::string> *attr) {
  if (attr == NULL)
    return false;

  attr->clear();

  int len = key.length();
  int start;
  int end;
  std::string map_key;
  std::string map_value;

  //  初始状态为1
  int status = 1;
  for (int i = 0; i < len; ++i) {

    if (key[i] == '<' || key[i] == '>')
      return false;

    switch (status) {
      case 1:
        if (key[i] == ' ') {
          status = 2;
          --i;

        } else if (key[i] == '\"' || key[i] == '=') {
          status = 9;

        } else {
          status = 1;
        }
        break;

      case 2:
        if (key[i] == ' ') {
          status = 2;
        } else if (key[i] == '\"' || key[i] == '=') {
          status = 9;
        } else {
          status = 3;
          start = i;
          --i;
        }
        break;

      case 3:
        if (key[i] == ' ') {
          end = i;
          map_key = key.substr(start, end - start);
          status = 4;
          --i;
        } else if (key[i] == '\"') {
          status = 9;
        } else if (key[i] == '=') {
          end = i;
          map_key = key.substr(start, end - start);
          status = 5;
        } else {
          status = 3;
        }
        break;
      case 4:
        if (key[i] == ' ') {
          status = 4;
        } else if (key[i] == '=') {
          status = 5;
        } else {
          status = 9;
        }
        break;
      case 5:
        if (key[i] == ' ') {
          status = 6;
          --i;
        } else if (key[i] == '\"') {
          start = i + 1;
          status = 7;
        } else {
          status = 9;
        }
        break;
      case 6:
        if (key[i] == ' ') {
          status = 6;
        } else if (key[i] == '\"') {
          start = i + 1;
          status = 7;
        } else {
          status = 9;
        }
        break;
      case 7:
        if (key[i] == '\"') {
          end = i;
          map_value = key.substr(start, end - start);
          (*attr)[map_key] = map_value;
          status = 8;
        } else {
          status = 7;
        }
        break;
      case 8:
        if (key[i] == ' ') {
          status = 2;
          --i;
        } else {
          status = 9;
        }
        break;
      case 9:
        return false;
    }
  }

  return true;
}

bool XMLValueSerializer::Serialize(const Value& root) {
  key_map_.clear();
  xml_string_->append("<xml>");
  key_map_[0] = base::BasicUtil::StringConversions::ASCIIToWide("xml");
  BuildXMLString(&root, 1, false);
  return true;
}

bool XMLValueSerializer::Serialize(const Value& root, std::string*  str) {
  xml_string_ = str;
  Serialize(root);
  return true;
}

void XMLValueSerializer::BuildXMLString(const Value* const node, int depth,
                                        bool escape) {
  switch (node->GetType()) {
    case Value::TYPE_NULL:
      xml_string_->append("null");
      break;
    case Value::TYPE_BOOLEAN: {
      bool value;
      bool result = node->GetAsBoolean(&value);
      xml_string_->append(value ? "true" : "false");
      break;
    }
    case Value::TYPE_INTEGER: {
      int32 value;
      bool result = node->GetAsInteger(&value);
      base::BasicUtil::StringUtil::StringAppendF(xml_string_, "%d", value);
      break;
    }
    case Value::TYPE_BIG_INTEGER: {
      int64 value;
      bool result = node->GetAsBigInteger(&value);
#if __LP64__
      base::BasicUtil::StringUtil::StringAppendF(xml_string_, "%d", value);
#else
      base::BasicUtil::StringUtil::StringAppendF(xml_string_,"%lld",value);
#endif
      break;
    }
    case Value::TYPE_REAL: {
      double value;
      bool result = node->GetAsReal(&value);
      std::string real = base::BasicUtil::StringUtil::DoubleToString(value);
      if (real.find('.') == std::string::npos
          && real.find('e') == std::string::npos
          && real.find('E') == std::string::npos) {
        real.append(".0");
      }
      // The JSON spec requires that non-integer values in the range (-1,1)
      // have a zero before the decimal point - ".52" is not valid, "0.52" is.
      if (real[0] == '.') {
        real.insert(0, "0");
      } else if (real.length() > 1 && real[0] == '-' && real[1] == '.') {
        // "-.1" bad "-0.1" good
        real.insert(1, "0");
      }
      xml_string_->append(real);
      break;
    }

    case Value::TYPE_STRING: {
      std::string value;
      bool result = node->GetAsString(&value);
      if (escape) {
      } else {
        StringEscape::XMLDoubleQuote(value, true, xml_string_);
      }
      break;
    }
    case Value::TYPE_LIST: {
      std::wstring pair_key(
          base::BasicUtil::StringConversions::ASCIIToWide(""));
      int mark = 0;
      if (key_map_.find(depth - 1) != key_map_.end()) {
        pair_key = key_map_[depth - 1];
        mark = 1;
      }

      const ListValue* list = static_cast<const ListValue*>(node);
      for (size_t i = 0; i < list->GetSize(); ++i) {
        if (i != 0) {
          if (pretty_print_)
            xml_string_->append("\n");
        }
        Value* value = NULL;
        bool result = list->Get(i, &value);
        BuildXMLString(value, depth, escape);
        if ((i < list->GetSize() - 1) && mark) {
          xml_string_->append("<");
          AppendQuoteString(
              base::BasicUtil::StringConversions::WideToASCII(pair_key));
          key_map_[depth - 1] = pair_key;
          xml_string_->append(">");
        }
      }
      break;
    }

    case Value::TYPE_DICTIONARY: {
      const DictionaryValue* dict = static_cast<const DictionaryValue*>(node);

      DictionaryValue::key_iterator key_itr = dict->begin_keys();
      for (; key_itr != dict->end_keys(); ++key_itr) {
        Value* value = NULL;
        bool result = dict->GetWithoutPathExpansion(*key_itr, &value);
        key_map_[depth] = (*key_itr);
        xml_string_->append("<");
        AppendQuoteString(
            base::BasicUtil::StringConversions::WideToASCII(*key_itr));
        key_map_[depth] = *key_itr;
        xml_string_->append(">");

        BuildXMLString(value, depth + 1, escape);
      }
      break;
    }

  }

  if (key_map_.find(depth - 1) != key_map_.end()) {
    xml_string_->append("</");
    std::wstring pair_key = key_map_[depth - 1];
    AppendQuoteString(
        base::BasicUtil::StringConversions::WideToASCII(pair_key));
    xml_string_->append(">");
    std::map<int, std::wstring>::iterator it = key_map_.find(depth - 1);
    key_map_.erase(it);
  }
}

Value* XMLValueSerializer::Deserialize(std::string* str,
      int* error_code, std::string* error_str) {
  xml_string_ = str;
  return Deserialize(error_code, error_str);
}

Value* XMLValueSerializer::Deserialize(int* error_code,
                                       std::string* error_str) {
  int pos = 0;
  class XmlNode *root = XmlToXmlTree(*xml_string_, &pos);
  if (NULL == root) {
    return NULL;
  }

  return XmlTreeToValue(root);
}

Value *XMLValueSerializer::CreateValue(std::string &origin_value) {
  return base_logic::Value::CreateStringValue(origin_value);
}

class XmlNode *XMLValueSerializer::XmlToXmlTree(std::string &XmlStr,
                                                int *origin_pos) {
  int start = *origin_pos;
  int end;

  class XmlNode *cur_node = new XmlNode();
  if (cur_node == NULL) {
    LOG_DEBUG2("%s", "内存分配出错");
    return NULL;
  }

  //  寻找key
  start = start + 1;
  end = XmlStr.find('>', start);
  cur_node->key = XmlStr.substr(start, end - start);

  //  设置儿子节点
  start = end + 1;
  end = XmlStr.find('<', start);
  if (XmlStr[end + 1] == '/') {
    std::string in_value = XmlStr.substr(start, end - start);
    cur_node->value = CreateValue(in_value);

    start = end + 1;
    end = XmlStr.find('>', start);
    *origin_pos = end + 1;
    return cur_node;
  } else {
    //<![CDATA[  ]] 标识内容不予解析
    int temp = XmlStr.find("<![CDATA[", start);
    if (temp == end) {
      end = XmlStr.find('<', start+8);
      if (XmlStr[end + 1] == '/') {  // 儿子结点查找结束
        std::string in_value = XmlStr.substr(start, end - start);
        cur_node->value = CreateValue(in_value);
        start = end + 1;
        end = XmlStr.find('>', start);
        *origin_pos = end + 1;
        return cur_node;
      }
    }
  }

  for (;;) {
    end = XmlStr.find('<', start);
    if (XmlStr[end + 1] == '/') {  // 儿子结点查找结束
      start = end + 1;
      end = XmlStr.find('>', start);
      *origin_pos = end + 1;
      break;

    } else {
      //<![CDATA[  ]] 标识内容不予解析
      int temp = XmlStr.find("<![CDATA[", start);
      if (temp == end) {
        end = XmlStr.find('<', start+8);
        if (XmlStr[end + 1] == '/') {  // 儿子结点查找结束
          start = end + 1;
          end = XmlStr.find('>', start);
          *origin_pos = end + 1;
          break;
        }
      }
      class XmlNode * child = XmlToXmlTree(XmlStr, &end);
      cur_node->child.push_back(child);
      start = end;
    }
  }

  return cur_node;
}

bool XMLValueSerializer::MergeTree(class XmlNode * XmlRoot) {
  if (XmlRoot == NULL) {
    LOG_DEBUG2("函数MergeTree:%s", "传入了空指针");
    return false;
  }

  int ChildMax = XmlRoot->child.size();
  for (int i = 0; i < ChildMax; ++i) {
    if (!MergeTree(XmlRoot->child[i])) {
      LOG_DEBUG2("函数MergeTree:%s", "子节点出错");
      return false;
    }
  }

  if (ChildMax == 0) {  //  基本节点
    return true;
  } else {  // 该节点是字典
    XmlRoot->value = new base_logic::DictionaryValue();

    //  将key值相同的子节点放在一起组成list.
    std::map<std::string, std::vector<int> > key_map;
    std::string key;
    for (int i = 0; i < ChildMax; ++i) {
      key = XmlRoot->child[i]->key;
      if (key_map.find(key) == key_map.end()) {
        std::vector<int> tmp;
        tmp.push_back(i);
        key_map[key] = tmp;
      } else {
        key_map[key].push_back(i);
      }
    }

    std::map<std::string, std::vector<int> >::iterator it = key_map.begin();
    for (; it != key_map.end(); ++it) {
      //  子节点是list
      if ((it->second).size() > 1) {
        key = XmlRoot->child[(it->second)[0]]->key;  //  获取list的key
        base_logic::ListValue *l = new base_logic::ListValue();
        if (NULL == l) {
          LOG_DEBUG2("MergeTree:%s", "内存分配出错");
          return false;
        }

        int len = (it->second).size();
        for (int i = 0; i < len; ++i) {
          int pos = (it->second)[i];
          l->Append(XmlRoot->child[pos]->value);
        }

        (reinterpret_cast<DictionaryValue*>(XmlRoot->value))->Set(key, l);

      } else {  //  正常节点
        key = XmlRoot->child[(it->second)[0]]->key;
        (reinterpret_cast<DictionaryValue*>(XmlRoot->value))->Set(
            key, XmlRoot->child[(it->second)[0]]->value);
      }
    }
  }

  return true;
}

Value *XMLValueSerializer::XmlTreeToValue(class XmlNode * XmlRoot) {
  bool r = MergeTree(XmlRoot);
  if (r == false) {
    LOG_DEBUG2("函数 XmlTreeToValue:%s", "合并出错");
    return NULL;
  }

  return XmlRoot->value;
}

void XMLValueSerializer::SetErrorCode(ParseError error,
                                      const wchar_t* error_pos) {
  int line_number = 1;
  int colum_number = 1;

  // Figure out the line and column the error occured at.
  for (const wchar_t* pos = start_pos_; pos != error_pos; ++pos) {
    if (*pos == '\0')
      return;
    if (*pos == '\n') {
      ++line_number;
      colum_number = 1;
    } else {
      ++colum_number;
    }
  }

  error_line_ = line_number;
  error_col_ = colum_number;
  error_code_ = error;

}
/*bool XMLValueSerializer::CheckXMLIsAttr(const Value* const node){
 bool r = false;
 switch(node->GetType()){
 case Value::TYPE_DICTIONARY:
 {
 const DictionaryValue* dict =
 static_cast<const DictionaryValue*>(node);
 for(DictionaryValue::key_iterator key_itr = dict->begin_keys();
 key_itr!=dict->end_keys();++key+itr){
 //类型为字符串 且key 为 #text
 if(*key_itr==L"#text"){
 r = true;
 break;
 }
 }
 break;
 }
 return r;
 }*/

}

