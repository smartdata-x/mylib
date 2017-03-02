/*
 * xml_serializer.h
 *
 *  Created on: 2015年6月17日
 *      Author: pro
 */

#ifndef XML_SERIALIZER_H_
#define XML_SERIALIZER_H_
#include "string_escape.h"
#include "value_serializer.h"

namespace base_logic{
//xml

enum XMLELEMENT{
	NOT_ATTR = 1,
	HAVE_ATTR = 2
};
class XmlNode {
  public:
    XmlNode() :key(""), value(NULL){}

  std::string key;
  base_logic::Value *value;
  std::vector<struct XmlNode *> child;
};
class XMLValueSerializer:public ValueSerializer{
public:
	XMLValueSerializer();
	XMLValueSerializer(std::string* xml,bool pretty_print = true);
	virtual ~XMLValueSerializer();
private:
  Value * CreateValue(std::string &origin_value);
  class XmlNode *XmlToXmlTree(std::string &XmlStr, int *origin_pos);
  bool MergeTree(class XmlNode * XmlRoot);
  Value *XmlTreeToValue(class XmlNode * XmlRoot);
  
public:
  static bool GetXmlAttribute(std::string &key, std::map<std::string, std::string> *attr);
public:
	virtual bool Serialize(const Value& root);


	virtual bool Serialize(const Value& root, std::string*  str);

public:
	virtual Value* Deserialize(int* error_code,std::string* error_str);

	virtual Value* Deserialize(std::string* str,
				  int* error_code, std::string* error_str);

	virtual void FreeValue(base_logic::Value* value){}

private:
	void BuildXMLString(const Value* const node,int depth,bool escape);

	inline void IndentLine(int depth);

	void AppendQuoteString(const std::string& str){StringEscape::XMLDoubleQuote(str,true,xml_string_);}
private:
	Token ParseToken();

	Value* BuildValue(bool is_root);

	bool EatComment();

	void EatWhiesspaceAndComments();

	void SetErrorCode(ParseError error,const wchar_t* error_pos);

private:
	bool ReadHexDigits(Token& token,int digits);

	Token ParseNumberToken();

	Token ParseStringToken();


	bool ReadInt(Token& token, bool can_have_leading_zeros);

	bool NextStringMatch(const std::wstring& str);

	//bool CheckXMLIsAttr(const Value* const node);

	Value* DecodeNumber(const Token& token);
	Value* DecodeString(const Token& token);
private:
	std::string*       xml_string_;

	bool               pretty_print_;

	const wchar_t*     start_pos_;

	const wchar_t*     xml_pos_;
	//Used to keep track of how many nested lists/dicts there are.
	int stack_depth_;

	//A parser flag that allows trailing commas in objects and arrays
	bool allow_trailing_comma_;

	ParseError  error_code_;

	int error_line_;
	int error_col_;

	std::map<int,std::wstring>  key_map_;

	//std::wstring 	pair_key_;
	//std::wstring    parent_key_;

};
}


#endif /* XML_SERIALIZER_H_ */
