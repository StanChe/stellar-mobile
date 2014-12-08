#include "Helper.h"
#include <curl/curl.h>

#include "../rapidjson/include/rapidjson/writer.h"
#include "../rapidjson/include/rapidjson/stringbuffer.h"
#include "../Utils/ErrorCategory.h"
#include "../Utils/Helper.h";
#include "../Utils/JSS.h"
#include "../Utils/CurlReader.h"

#include <iostream>
#include "../Utils/Config.h"
#include <sstream>

namespace stellar_mobile
{
	std::string Helper::AsString(rapidjson::Document& d)
	{
		rapidjson::StringBuffer strbuf;
		rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
		d.Accept(writer);
	    return strbuf.GetString();
	}

	std::string Helper::AsString(rapidjson::Value& d)
	{
		rapidjson::StringBuffer strbuf;
		rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
		d.Accept(writer);
	    return strbuf.GetString();
	}

	std::string Helper::StringFormat(std::string rawFormat, std::string* params, __int32 paramsNum)
	{
		__int64 buffSize = rawFormat.length();
		for (__int32 i = 0; i < paramsNum; i++)
			buffSize += params[i].length();
		char* buff = new char[buffSize];
		std::string format = rawFormat;
		for (__int32 i = 0; i < paramsNum; i++) 
		{
			format = Helper::StringFormat(buff, format, params[i].c_str());
		}
		std::string str = std::string(buff);
		delete[] buff;
		return str;
	}

	std::string Helper::StringFormat(char* buff, std::string format, const char* param1)
	{
		sprintf(buff, format.c_str(), param1);
		return buff;
	}

	void Helper::redirectRequest(rapidjson::Document& params, rapidjson::Value& result, rapidjson::MemoryPoolAllocator<>& allocator)
	{
		CurlReader cReader;
		std::string data = cReader.performRequest(Config::DataContainer.StellardAddress, Helper::AsString(params));
		rapidjson::Document response;
		if (response.Parse<0>(data.c_str()).HasParseError())
		{
			ErrorsContainer::Errors.get(UNPARSABLE_RESPONSE_JSON).InjectTo(result, allocator);
			return;
		}
		if (!response.HasMember(JSS::RESULT_FIELD) || !response[JSS::RESULT_FIELD].IsObject())
		{
			ErrorsContainer::Errors.get(INVALID_RESPONSE_FORMAT).InjectTo(result, allocator);
			return;
		}
		result = rapidjson::Value(response[JSS::RESULT_FIELD], allocator);
	}

	std::string Helper::replace(char c, std::string replacement, std::string const& s)
	{
		std::string result;
		size_t searchStartPos = 0;

		std::string chars = std::string("\\") + c;
		size_t pos = s.find_first_of(chars);
		if (pos == std::string::npos)
		{
			result = s;
			return result;
		}
		while (pos != std::string::npos)
		{
			result += s.substr(searchStartPos, pos - searchStartPos);
			if (s[pos] == '\\')
			{
				result += std::string("\\") + c;
				searchStartPos = pos + 2;
			}
			else if (s[pos] == c)
			{
				result += replacement;
				searchStartPos = pos + 1;
			}

			pos = s.find_first_of(chars, searchStartPos);
		}

		return result;
	}

	void Helper::split(const std::string& s, char delim, std::vector<std::string>& elems)
	{
		std::stringstream ss(s);
		std::string item;
		while (std::getline(ss, item, delim)) {
			elems.push_back(item);
		}
	}

	std::string Helper::toLower(std::string& s)
	{
		std::string result;
		std::locale loc;
		for (unsigned int i = 0; i < s.length(); ++i)
		{
			result += tolower(s.at(i), loc);
		}
		return result;
	}
}
