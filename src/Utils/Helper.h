#ifndef STELLAR_MOBILE_HELPER_H
#define STELLAR_MOBILE_HELPER_H

#include <string>
#include "../rapidjson/include/rapidjson/document.h"
#include <vector>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>


namespace stellar_mobile
{
	class Helper
	{
			static std::string StringFormat(char* buff, std::string format, const char* param1);
		public:
			static std::string AsString(rapidjson::Document& d);
			static std::string StringFormat(std::string format, std::string* params, __int32 paramsNum);
			static std::string replace(char c, std::string replacement, std::string const& s);
			static void split(const std::string& s, char delim, std::vector<std::string>& elems);
			static inline std::string &ltrim(std::string &s) {
				s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
				return s;
			}
			// trim from end
			static inline std::string &rtrim(std::string &s) {
					s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
					return s;
			}
			// trim from both ends
			static inline std::string &trim(std::string &s) {
					return ltrim(rtrim(s));
			}

			static std::string toLower(std::string &s);
			static std::string Helper::AsString(rapidjson::Value& d);
			static inline void AddMember(rapidjson::Value& obj, const char* nameCh, const char* valueCh, rapidjson::MemoryPoolAllocator<>& allocator)
			{
				using namespace rapidjson;
				Value value;
				value.SetString(valueCh, allocator);
				AddMember(obj, nameCh, value, allocator);
			}

			static inline void AddMember(rapidjson::Value& obj, const char* nameCh, rapidjson::Value& value, rapidjson::MemoryPoolAllocator<>& allocator)
			{
				using namespace rapidjson;
				Value name;
				name.SetString(nameCh, allocator);
				obj.AddMember(name, value, allocator);
			}

			static void redirectRequest(rapidjson::Document& params, rapidjson::Value& result, rapidjson::MemoryPoolAllocator<>& allocator);
	};

}
#endif
