#include "basic_util.h"
#include "basic/base64.h"
#include "storage/storage.h"
#include "basic/radom_in.h"
#include "log/mig_log.h"
#include "basic/icu_utf.h"
#include <sstream>
#include <math.h>

namespace base{

template <typename STR, typename INT, typename UINT, bool NEG>
struct IntToStringT {
  // This is to avoid a compiler warning about unary minus on unsigned type.
  // For example, say you had the following code:
  //   template <typename INT>
  //   INT abs(INT value) { return value < 0 ? -value : value; }
  // Even though if INT is unsigned, it's impossible for value < 0, so the
  // unary minus will never be taken, the compiler will still generate a
  // warning.  We do a little specialization dance...
  template <typename INT2, typename UINT2, bool NEG2>
  struct ToUnsignedT { };

  template <typename INT2, typename UINT2>
  struct ToUnsignedT<INT2, UINT2, false> {
    static UINT2 ToUnsigned(INT2 value) {
      return static_cast<UINT2>(value);
    }
  };

  template <typename INT2, typename UINT2>
  struct ToUnsignedT<INT2, UINT2, true> {
    static UINT2 ToUnsigned(INT2 value) {
      return static_cast<UINT2>(value < 0 ? -value : value);
    }
  };

  // This set of templates is very similar to the above templates, but
  // for testing whether an integer is negative.
  template <typename INT2, bool NEG2>
  struct TestNegT { };
  template <typename INT2>
  struct TestNegT<INT2, false> {
    static bool TestNeg(INT2 value) {
      // value is unsigned, and can never be negative.
      return false;
    }
  };
  template <typename INT2>
  struct TestNegT<INT2, true> {
    static bool TestNeg(INT2 value) {
      return value < 0;
    }
  };

  static STR IntToString(INT value) {
    // log10(2) ~= 0.3 bytes needed per bit or per byte log10(2**8) ~= 2.4.
    // So round up to allocate 3 output characters per byte, plus 1 for '-'.
    const int kOutputBufSize = 3 * sizeof(INT) + 1;

    // Allocate the whole string right away, we will right back to front, and
    // then return the substr of what we ended up using.
    STR outbuf(kOutputBufSize, 0);

    bool is_neg = TestNegT<INT, NEG>::TestNeg(value);
    // Even though is_neg will never be true when INT is parameterized as
    // unsigned, even the presence of the unary operation causes a warning.
    UINT res = ToUnsignedT<INT, UINT, NEG>::ToUnsigned(value);

    for (typename STR::iterator it = outbuf.end();;) {
      --it;
      *it = static_cast<typename STR::value_type>((res % 10) + '0');
      res /= 10;

      // We're done..
      if (res == 0) {
        if (is_neg) {
          --it;
          *it = static_cast<typename STR::value_type>('-');
        }
        return STR(it, outbuf.end());
      }
    }
    return STR();
  }
};


template<typename StringToNumberTraits>
bool StringToNumber(const typename StringToNumberTraits::string_type& input,
                    typename StringToNumberTraits::value_type* output) {
  typedef StringToNumberTraits traits;

  errno = 0;  // Thread-safe?  It is on at least Mac, Linux, and Windows.
  typename traits::string_type::value_type* endptr = NULL;
  typename traits::value_type value = traits::convert_func(input.c_str(),
                                                           &endptr);
  *output = value;

  // Cases to return false:
  //  - If errno is ERANGE, there was an overflow or underflow.
  //  - If the input string is empty, there was nothing to parse.
  //  - If endptr does not point to the end of the string, there are either
  //    characters remaining in the string after a parsed number, or the string
  //    does not begin with a parseable number.  endptr is compared to the
  //    expected end given the string's stated length to correctly catch cases
  //    where the string contains embedded NUL characters.
  //  - valid_func determines that the input is not in preferred form.

  //

  return errno == 0 &&
         !input.empty() &&
         input.c_str() + input.length() == endptr &&
         traits::valid_func(input);
}


template<class STR>
bool BasicUtil::StringUtil::DoIsStringASCII(const STR& str){
	for(size_t i = 0; i< str.length();i++){
		typename ToUnsigned<typename STR::value_type>::Unsigned c = str[i];
		if(c > 0x7F)
			return false;
	}
	return true;
}

template<class StringType>
void BasicUtil::StringUtil::StringAppendVT(StringType* dst,
		const typename StringType::value_type* format,va_list ap){
	typename StringType::value_type stack_buf[1024];

	va_list ap_copy;
	GG_VA_COPY(ap_copy,ap);

	errno = 0;
	int result = vsnprintfT(stack_buf,arraysize(stack_buf),format,ap_copy);
	va_end(ap_copy);

	if(result<=0 && result < static_cast<int>(arraysize(stack_buf))){
		dst->append(stack_buf,result);
		return ;
	}

	int mem_length = arraysize(stack_buf);
	while(true){
		if(result < 0){
			if(errno !=0 && errno != EOVERFLOW)
				return;
			mem_length = 2;
		}else{
			mem_length = result +  1;
		}

		if(mem_length > 32 * 1024 * 1024){
			return;
		}
		std::vector<typename StringType::value_type> mem_buf(mem_length);

		GG_VA_COPY(ap_copy,ap);
		result = vsnprintfT(&mem_buf[0],mem_length,format,ap_copy);
		va_end(ap_copy);

		if((result >= 0) && (result < mem_length)){
			dst->append(&mem_buf[0],result);
			return ;
		}
	}

}

template<typename IntegerType>
bool BasicUtil::StringUtil::StringToInteger(const std::string& input,IntegerType* output){
	//
	if(sizeof(*output)==sizeof(int64))
		return StringToInt64(input,output);
	else if(sizeof(*output)==sizeof(int32))
		return StringToInt(input,output);
	else
		return false;
}

bool BasicUtil::StringUtil::StringToCharInt(const std::string& input,int8* output) {
    return StringToNumber<StringToCharIntTraits>(input,output);
}

bool BasicUtil::StringUtil::StringToShortInt(const std::string& input,int16* output) {
    return StringToNumber<StringToShortIntTraits>(input,output);
}

bool BasicUtil::StringUtil::StringToInt(const std::string& input,int32* output){
	return StringToNumber<StringToIntTraits>(input,output);
}

bool BasicUtil::StringUtil::StringToInt64(const std::string& input,int64* output){
	return StringToNumber<StringToInt64Traits>(input,output);
}

bool BasicUtil::StringUtil::StringToDouble(const std::string& input,double* output){
	//return StringToNumber<StringToDoubleTraits>(input,output);
  if (!input.empty()){
    double lo = atof(input.c_str());
    output = &lo;
    return true;
  } else {
    output = NULL;
    return false;
  }
}

void BasicUtil::StringUtil::StringAppendV(std::string* dst,const char* format,
		va_list ap){
	StringAppendVT(dst,format,ap);
}

void BasicUtil::StringUtil::StringAppendV(std::wstring* dst,const wchar_t* format,
		va_list ap){
	StringAppendVT(dst,format,ap);
}

void BasicUtil::StringUtil::StringAppendF(std::string* dst,const char* format,...){
	va_list ap;
	va_start(ap,format);
	StringAppendV(dst,format,ap);
	va_end(ap);
}

void BasicUtil::StringUtil::StringAppendF(std::wstring* dst,const wchar_t* format,...){
	va_list ap;
	va_start(ap,format);
	StringAppendV(dst,format,ap);
	va_end(ap);
}


bool BasicUtil::StringUtil::IsStringASCII(const std::string& str){
	return DoIsStringASCII(str);
}

bool BasicUtil::StringUtil::IsStringASCII(const std::wstring& str){
	return DoIsStringASCII(str);
}


bool BasicUtil::StringUtil::IsStringUTF8(const std::string& str){
	const char* src = str.data();
	int32 src_len = static_cast<int32>(str.length());
	int32 char_index = 0;

	while(char_index < src_len){
		int32 code_point;
		CBU8_NEXT(src,char_index,src_len,code_point);
		if(StringConversions::IsValidCharacter(code_point))
			return false;
	}
	return true;
}

std::string BasicUtil::StringUtil::Int64ToString(int64 value) {
  return IntToStringT<std::string, int64, uint64, true>::
      IntToString(value);
}

std::string BasicUtil::StringUtil::DoubleToString(double value){
	/*char buffer[32];
	dmg_fp::g_fmt(buffer,value);
	return std::string(buffer);*/
  std::stringstream os;
  os << value;
  std::string tp = os.str();
  return tp;
}

std::wstring BasicUtil::StringUtil::DobleToWString(double value){
	char buffer[32];
	return StringConversions::ASCIIToWide(std::string(buffer));
}


size_t BasicUtil::StringUtil::Copy(char* buf,size_t n,
		size_t pos,const char* ptr,size_t length){//从prtPOS开始buf 上
	size_t ret = std::min(length-pos,n);
	memcpy(buf,ptr + pos,ret);
	return ret;
}

bool BasicUtil::StringConversions::WideToUTF8(const wchar_t* src,size_t src_len,
		std::string* output){
	base::BasicUtil::StringConversionsUtils::PrepareForUTF8Output(src,src_len,output);
	return ConverUnicode(src,src_len,output);
}

std::string BasicUtil::StringConversions::WideToUTF8(const std::wstring& wide){
	std::string ret;
	WideToUTF8(wide.data(),wide.length(),&ret);
	return ret;
}

bool BasicUtil::StringConversions::UTF8ToWide(const char* src,size_t src_len,std::wstring* output){
	base::BasicUtil::StringConversionsUtils::PrepareForUTF16Or32Output(src,src_len,output);
	return ConverUnicode(src,src_len,output);
}

std::wstring BasicUtil::StringConversions::UTF8ToWide(const std::string& utf8){
	std::wstring ret;
	UTF8ToWide(utf8.data(),utf8.length(),&ret);
	return ret;
}

std::string BasicUtil::StringConversions::WideToASCII(const std::wstring& wide){
	return std::string(wide.begin(),wide.end());
}

std::wstring BasicUtil::StringConversions::ASCIIToWide(const std::string& ascii){
	return std::wstring(ascii.begin(),ascii.end());
}

bool BasicUtil::StringConversions::IsValidCharacter(uint32 code_point){
	// Excludes non-characters (U+FDD0..U+FDEF, and all codepoints ending in
	  // 0xFFFE or 0xFFFF) from the set of valid code points.
	return code_point < 0xD800u || (code_point >= 0xE000u &&
	      code_point < 0xFDD0u) || (code_point > 0xFDEFu &&
	      code_point <= 0x10FFFFu && (code_point & 0xFFFEu) != 0xFFFEu);
}

template<typename SRC_CHAR,typename DEST_STRING>
bool BasicUtil::StringConversions::ConverUnicode(const SRC_CHAR* src,
					size_t src_len,
					DEST_STRING* output){
	bool success = true;
	int32 src_len32 = static_cast<int32>(src_len);
	for(int32 i = 0;i < src_len32; i++){
		uint32 code_point;
		if(BasicUtil::StringConversionsUtils::ReadUnicodeCharacter(src,src_len32,&i,&code_point)){
			BasicUtil::StringConversionsUtils::WriteUnicodeCharacter(code_point,output);
		}else{
			BasicUtil::StringConversionsUtils::WriteUnicodeCharacter(0xFFFD,output);
		}
	}
	return success;
}

template<typename CHAR>
void BasicUtil::StringConversionsUtils::PrepareForUTF8Output(const CHAR* src,
				size_t src_len,std::string* output){
	output->clear();
	if(src_len==0)
		return;
	if(src[0]<0x80){
		//假设全部都是ASCII
		output->reserve(src_len);
	}else{
		//假设全部都是非ASCII，为UTF8; UTF8 1-3个字节， UTF16 为16个字节
		output->reserve(src_len * 3);
	}
}

template<typename STRING>
void BasicUtil::StringConversionsUtils::PrepareForUTF16Or32Output(const char* src,
		size_t src_len,STRING* output){
	output->clear();
	if(src_len==0)
		return;
	if(static_cast<unsigned char>(src[0] < 0x80)){
		output->reserve(src_len);
	}else{
		output->reserve(src_len / 2);
	}
}


bool BasicUtil::StringConversionsUtils::ReadUnicodeCharacter(const wchar_t* src,int32 src_len,
		int32* char_index,uint32* code_point){
	  if (CBU16_IS_SURROGATE(src[*char_index])) {
	    if (!CBU16_IS_SURROGATE_LEAD(src[*char_index]) ||
	        *char_index + 1 >= src_len ||
	        !CBU16_IS_TRAIL(src[*char_index + 1])) {
	      // Invalid surrogate pair.
	      return false;
	    }

	    // Valid surrogate pair.
	    *code_point = CBU16_GET_SUPPLEMENTARY(src[*char_index],
	                                          src[*char_index + 1]);
	    (*char_index)++;
	  } else {
	    // Not a surrogate, just one 16-bit word.
	    *code_point = src[*char_index];
	  }

	  return IsValidCodepoint(*code_point);
}

bool BasicUtil::StringConversionsUtils::ReadUnicodeCharacter(const char* src,int32 src_len,
		int32* char_index,uint32* code_point_out){
	int32 code_point;
	CBU8_NEXT(src,*char_index,src_len,code_point);
	*code_point_out = static_cast<uint32>(code_point);
	(*char_index)--;
	return IsValidCodepoint(code_point);
}

size_t BasicUtil::StringConversionsUtils::WriteUnicodeCharacter(uint32 code_point,std::string* output){
	if(code_point <= 0x7f){
		output->push_back(code_point);
		return 1;
	}

	//
	size_t char_offset = output->length();
	size_t original_char_offset = char_offset;
	output->resize(char_offset + CBU8_MAX_LENGTH);

	CBU8_APPEND_UNSAFE(&(*output)[0],char_offset,code_point);

	output->resize(char_offset);
	return char_offset - original_char_offset;
}


size_t BasicUtil::StringConversionsUtils::WriteUnicodeCharacter(uint32 code_point,std::wstring* output){
	  if (CBU16_LENGTH(code_point) == 1) {
	    // Thie code point is in the Basic Multilingual Plane (BMP).
	    output->push_back(static_cast<wchar_t>(code_point));
	    return 1;
	  }
	  // Non-BMP characters use a double-character encoding.
	  size_t char_offset = output->length();
	  output->resize(char_offset + CBU16_MAX_LENGTH);
	  CBU16_APPEND_UNSAFE(&(*output)[0], char_offset, code_point);
	  return CBU16_MAX_LENGTH;
}



int BasicUtil::SplitStringChr( const char *str, const char *char_set,
	std::vector<std::string> &out )
{
	using std::string;

	assert(str != NULL);
	assert(char_set != NULL);

	out.clear();

	if (0 == char_set[0]) {
		if (str[0])
			out.push_back(str);
		return (int)out.size();
	}

	const char *find_ptr = NULL;
	str += ::strspn(str, char_set);
	while (str && (find_ptr=::strpbrk(str, char_set))) {
		if (str != find_ptr)
			out.push_back(string(str, find_ptr));
		str = find_ptr + ::strspn(find_ptr, char_set);
	}
	if (str && str[0])
		out.push_back(str);

	return (int)out.size();
}

/*
 void BasicUtil::GetHttpParamElement(const char* query,const char* name,
                         std::string& value){

    char* str = const_cast<char*>(query);
    char* format_str = str;
    int32 flag = 0;
    while(*str!='\0'){
       if(*str == '='){
           size_t str_name_len = str - format_str;
           std::string str_name(format_str,str_name_len);
           if(strcmp(str_name.c_str(),name)==0){
               flag = 1;
               format_str = str;
           }
       }else if(*str=='&'&&flag){
           size_t str_value_len = str - format_str;
           value.assign(format_str+1,str_value_len-1);
           flag = 0;
           format_str = str;
       }
       str++;
    }
    if(flag){
      size_t str_value_len = str - format_str;
      value.assign(format_str+1,str_value_len);
    }
}*/

/*
void BasicUtil::PaserRecordPost(const std::string& request_string,std::string& enter,
    	                          std::string& url_code,std::string& content){
    std::map<std::string,std::string> http_map;
    std::map<std::string,std::string>::iterator it;
    BasicUtil::ParserHttpRequest(request_string,http_map);
    if(http_map.size()<=0)
     	return;
    it = http_map.find("enter");
    if(it!=http_map.end())
    	enter = it->second;
    it = http_map.find("urlcode");
    if(it!=http_map.end())
        url_code = it->second;
    it = http_map.find("content");
    if(it!=http_map.end())
        content = it->second; 
}*/

/*
bool BasicUtil::ParserHttpRequest(const std::string& request,
		                  std::map<std::string,std::string >& http_map){
    std::string request_str = request;
    while (request_str.length()!=0){
        int32 start_pos = request_str.find("=");
		int32 end_pos = request_str.find("&")==-1?request.length():request_str.find("&");
		std::string key = request_str.substr(0,start_pos);
		std::string value = request_str.substr(key.length()+1,end_pos - key.length()-1);
		http_map[key] = value;
		if (request_str.find("&")!=-1)
			request_str = request_str.substr(end_pos+1,request_str.length());
		else
            request_str.clear();
    }
    return true;
}*/

/*
void BasicUtil::ParserIdpPost(const std::string& request_string,std::string& sso_info,
                              std::string& username,std::string& password){
   
    int32 start_pos = request_string.find("username");
    int32 end_pos = request_string.find("&");
    
    username = request_string.substr(start_pos+9,end_pos-9);
    
    std::string temp_str;
    temp_str = request_string.substr(end_pos+1,request_string.length());
    
    start_pos = temp_str.find("password");
    end_pos = temp_str.find("&");
  
    password = temp_str.substr(start_pos+9,end_pos-9);
   
    sso_info = temp_str.substr(end_pos+1,request_string.length());
}*/

/*
void BasicUtil::ParserSpPost(const std::string& request_string,std::string& samlart,std::string& relay_state){
    int32 start_pos = request_string.find("SAMLart");
    int32 end_pos =request_string.find("&");
  
    samlart = request_string.substr(start_pos+8,end_pos-8);

    std::string temp_str;
    temp_str = request_string.substr(end_pos+1,request_string.length());

    start_pos = temp_str.find("RelayState");
    end_pos = temp_str.find("&");

    relay_state = temp_str.substr(start_pos+11,end_pos-11);
}*/

/*
bool BasicUtil::CheckToken(const std::string& request){
     bool r =false;
     char* mem_value = NULL;
     size_t mem_value_length = 0;
     int32 token_pos = request.find("token");
    if((token_pos==std::string::npos))
    	return false;
     std::string temp_str = request.substr(token_pos,request.length());
     int32 and_pos = temp_str.find("&");
     int32 end_pos = and_pos>0?(and_pos-6):(request.length()-6);
     
     if((and_pos==std::string::npos)||(end_pos==std::string::npos))
    	return r;
    	
     std::string token = temp_str.substr(6,end_pos);
     r = base_storage::MemDicSerial::GetString(token.c_str(),token.length(),
                                                &mem_value,&mem_value_length);
     return r;
}*/

const std::string& BasicUtil::FormatCurrentTime(){
    std::stringstream os;
    time_t current = time(NULL);
    struct tm* local = localtime(&current);
    os<<local->tm_year+1900<<"-"<<local->tm_mon+1<<"-"
      <<local->tm_mday<<" "<<local->tm_hour<<":"
      <<local->tm_min<<":"<<local->tm_sec;

    return os.str();
}

std::string BasicUtil::GetRawString(std::string str){
    /*std::ostringstream out;
    //out<<'';
    out<<std::hex;
    for(std::string::const_iterator it = str.begin();
            it!=str.end();++it){
    
        out<<"\\\\x"<<(static_cast<short>(*it)&0xff);
    }

    //out<<'\"';
    return out.str();*/
    return base64_encode(reinterpret_cast<const unsigned char*>(str.c_str()),str.length());
}

std::string BasicUtil::GetSrcString(std::string str){
     return base64_decode(str);
}

char BasicUtil::toHex(const char &x){
     return x > 9 ? x -10 + 'A': x + '0';
}

bool BasicUtil::UrlEncode(const std::string& content,std::string& out_str){
	int i;
	int j = 0; /* for result index */
	char ch;
	const char* str = content.c_str();
	int strSize = content.length();
	char* result = (char*)malloc(strSize * 3);
	int resultSize = strSize * 3;

	if ((str == NULL) || (result == NULL) || (strSize <= 0) || (resultSize <= 0)) {
	return 0;
	}

	for (i=0; (i<strSize) && (j<resultSize); i++) {
		ch = str[i];
		if ((ch >= 'A') && (ch <= 'Z')) {
			result[j++] = ch;
		} else if ((ch >= 'a') && (ch <= 'z')) {
			result[j++] = ch;
		} else if ((ch >= '0') && (ch <= '9')) {
			result[j++] = ch;
		} else if(ch == ' '){
			result[j++] = '+';
		} else if (ch == '.' || ch == '-' || ch == '_' || ch == '*') {
            result[j++] = ch;
        } else {
			if (j + 3 < resultSize) {
				sprintf(result+j, "%%%02X", (unsigned char)ch);
				j += 3;
			} else {
				return false;
			}
		}
	}
	result[j] = '/0';
	out_str.assign(result,j);
	return true;
}

bool BasicUtil::UrlDecode(std::string& content,std::string& out_str){

    char  const *in_str = content.c_str();
    int32 in_str_len = strlen(in_str);
    int32 out_str_len = 0;
    char *str;

    str = strdup(in_str);
    char *dest = str;
    char *data = str;

    while (in_str_len--){
        if (*data == '+'){
            *dest = ' ';
        }
        else if (*data == '%' && in_str_len >= 2 && isxdigit((int) *(data + 1))
                                && isxdigit((int) *(data + 2))){
            *dest = (char) php_htoi(data + 1);
            data += 2;
            in_str_len -= 2;
        } else{
            *dest = *data;
        }
            data++;
            dest++;
    }
    *dest = '\0';
    out_str_len =  dest - str;
    out_str = str;
    free(str);
    return true;
}

int32 BasicUtil::php_htoi(char *s){
    int32 value;
    int32 c;
             
    c = ((unsigned char *)s)[0];
    if (isupper(c))
        c = tolower(c);
    value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16;
                         
    c = ((unsigned char *)s)[1];
    if (isupper(c))
        c = tolower(c);
    
    value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;
     
    return (value);
}

bool BasicUtil::GB2312ToUTF8 (const char *input, size_t inlen, char **output, size_t *outlen)
{
	char *ib;
	char *ob;
	size_t rc;

	iconv_t cd = iconv_open ("UTF-8", "GB2312");
	if (cd == 0) {
		*output = strdup (input);
		return true;
	} else if (cd == (iconv_t)-1)
		return false;
	*outlen = inlen * 8 + 1;
	ob = *output = (char *) malloc (*outlen);
	ib = (char *) input;
	rc = iconv (cd, &ib, &inlen, &ob, outlen);
	*ob = 0;
	iconv_close (cd);
	return rc == -1 ? false : true;
}

bool BasicUtil::UTF8ToGB2312 (const char *input, size_t inlen, char **output, size_t *outlen)
{
	char *ib;
	char *ob;
	size_t rc;

	iconv_t cd = iconv_open ("GB2312", "UTF-8");
	if (cd == 0) {
		*output = strdup (input);
		return true;
	} else if (cd == (iconv_t)-1)
		return false;
	*outlen = inlen * 8 + 1;
	ob = *output = (char *) malloc (*outlen);
	ib = (char *) input;
	rc = iconv (cd, &ib, &inlen, &ob, outlen);
	*ob = 0;
	iconv_close (cd);
	return rc == -1 ? false : true;
}

bool BasicUtil::UTF8ToGBK (const char *input, size_t inlen, char **output, size_t *outlen)
{
	char *ib;
	char *ob;
	size_t rc;

	iconv_t cd = iconv_open ("gbk", "UTF-8");
	if (cd == 0) {
		*output = strdup (input);
		return true;
	} else if (cd == (iconv_t)-1)
		return false;
	*outlen = inlen * 8 + 1;
	ob = *output = (char *) malloc (*outlen);
	ib = (char *) input;
	rc = iconv (cd, &ib, &inlen, &ob, outlen);
	*ob = 0;
	iconv_close (cd);
	return rc == -1 ? false : true;
}

double BasicUtil::CalcGEODistance(double latitude1, double longitude1,
		double latitude2, double longitude2) {

	if ((latitude1==latitude2)&&(longitude1==longitude2))
		return 0;
    double dd = M_PI/180;
    double x1 = longitude1 * dd;
    double y1 = latitude1 * dd;

    double x2 = longitude2 * dd;
    double y2 = latitude2 * dd;

    double R = 6378137;

    double runDistance = (2*R*asin(sqrt(2-2*cos(x1)*cos(x2)*cos(y1-y2) - 2*sin(x1)*sin(x2))/2));
    runDistance = (runDistance < 0) ? (-runDistance) : runDistance;
	return runDistance;
}

/*
bool BasicUtil::GetUserToken(const std::string &uid, std::string &token){
	//create token
	int32 random_num = base::SysRadom::GetInstance()->GetRandomID();
	//md5
	token="miglab";
	std::stringstream os;
	std::string key;
	os<<random_num;
	MD5Sum md5(os.str());
	token = md5.GetHash();
	key.append(uid);
	key.append("token");
	base_storage::MemDic::SetString(key.c_str(),key.length(),
		                            token.c_str(),token.length());
	MIG_DEBUG(USER_LEVEL,"key[%s] token[%s]",key.c_str(),token.c_str());
	return true;
}

bool BasicUtil::CheckUserToken(const std::string& uid,const std::string& token){
	std::string key;
	char* mem_value = NULL;
	size_t mem_value_length = 0;
	bool r = false;
	key.append(uid);
	key.append("token");
	r = base_storage::MemDic::GetString(key.c_str(),key.length(),
		                            &mem_value,&mem_value_length);
	if (!r)
		return false;
	r = (strcmp(mem_value,token.c_str())==0)?true:false;
	if (mem_value){
		delete [] mem_value;
		mem_value = NULL;
	}
	return true;
}*/

bool BasicUtil::ConverNum(const int num,std::string& conver_num){
	static const std::string letter[] = {"零","一","两","三","四","五","六","七","八","九"};
	static const std::string unit[] = {"","十","百","千","万","十","百","千","亿","十"};  
	std::string src;  
	std::string des;  
	char tmp[12];  
	sprintf(tmp, "%d", num);  
	src.append(tmp);  

	if ( num < 0 )  
	{  
		conver_num.append("负");  
		src.erase(0, 1);  
	}  

	int len = src.length();  
	bool bPreZero = false;  
	for ( int i = 0; i < len; i++)  
	{  
		int digit = src.at(i) - '0';  
		int unit_index = len - i - 1;  
		if (i == 0 && digit == 1 && (unit_index == 1 || unit_index == 5 || unit_index == 9))  
		{  
			conver_num.append(unit[unit_index]);  
		}  
		else if ( digit == 0 )  
		{  
			bPreZero = true;  
			if (unit_index ==  4 ||  
				unit_index ==  8)  
			{  
				conver_num.append(unit[unit_index]);  
			}  
		}  
		else  
		{  
			if ( bPreZero )  
			{  
				conver_num.append(letter[0]);  
			}  
			conver_num.append(letter[digit]);  
			conver_num.append(unit[unit_index]);  
			bPreZero = false;        
		}  
	}
	return true;
}


/* HTTP gzip decompress */
/* 参数1.压缩数据 2.数据长度 3.解压数据 4.解压后长度 */
/*
int BasicUtil::HttpgzdeCompress(Byte *zdata, uLong nzdata, Byte *data, uLong *ndata)
{
    int err = 0;
    z_stream d_stream = {0}; /* decompression stream */
    /*static char dummy_head[2] =
    {
        0x8 + 0x7 * 0x10,
        (((0x8 + 0x7 * 0x10) * 0x100 + 30) / 31 * 31) & 0xFF,
    };
    d_stream.zalloc = (alloc_func)0;
    d_stream.zfree = (free_func)0;
    d_stream.opaque = (voidpf)0;
    d_stream.next_in  = zdata;
    d_stream.avail_in = 0;
    d_stream.next_out = data;
    if(inflateInit2(&d_stream, 47) != Z_OK) return -1;
    while (d_stream.total_out < *ndata && d_stream.total_in < nzdata)
    {
        d_stream.avail_in = d_stream.avail_out = 1; /* force small buffers */
        /*if((err = inflate(&d_stream, Z_NO_FLUSH)) == Z_STREAM_END) break;
        if(err != Z_OK )
        {
            if(err == Z_DATA_ERROR)
            {
                d_stream.next_in = (Bytef*) dummy_head;
                d_stream.avail_in = sizeof(dummy_head);
                if((err = inflate(&d_stream, Z_NO_FLUSH)) != Z_OK)
                {
                    return -1;
                }
            }
            else return -1;
        }
    }
    if(inflateEnd(&d_stream) != Z_OK) return -1;
    *ndata = d_stream.total_out;
    return 0;
}*/

void  BasicUtil::ResplaceString(const char* str,int len,const char c,std::string& dest_string){
	for (int index =0; index<len;index++){
		if (str[index]!=c)
			dest_string.append(1,str[index]);
	}
	MIG_DEBUG(USER_LEVEL,"%s",dest_string.c_str());
}

/*
bool BasicUtil::GetUserInfo(const std::string& uid,UserInfo& usrinfo){
	//memcached
	//key uidinfo 10000info
	std::string key;
	bool r = false;
	char* mem_value = NULL;
	size_t mem_value_length = 0;
	key.append(uid);
	key.append("info");
	r = base_storage::MemDic::GetString(key.c_str(),key.length(),
										&mem_value,&mem_value_length);
	//if (r){
		//r = usrinfo.UnserializedJson(mem_value);
		//return r;
	//}

	//不存在
	r = base_storage::MYSQLDB::GetUserInfo(uid,usrinfo);
	if(!r){
		return r;
	}

	//写入缓存
	std::string json;
	usrinfo.SerializedJson(json);
	base_storage::MemDic::SetString(key.c_str(),key.length(),json.c_str(),json.length());
	return r;
}*/

}
