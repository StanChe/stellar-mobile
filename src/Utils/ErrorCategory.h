#ifndef STELLAR_MOBILE_ERROR_CATEGORY_H
#define STELLAR_MOBILE_ERROR_CATEGORY_H

#include <string>
#include <map>
#include <tuple>

#include "../rapidjson/include/rapidjson/document.h"


namespace stellar_mobile
{
	enum error_code_i
	{
		UNKNOWN = 0,     // Represents codes not listed in this enumeration
		UNPARSABLE_RESPONSE_JSON = 1,
		UNKNOWN_METHOD = 2,
		INVALID_PARAMS = 4,
		INVALID_JSON = 8,
		INVALID_PARAM_TYPE = 16,
		CANT_GET_DATA_FROM_STELLARD = 32,
		INVALID_RESPONSE_FORMAT = 64,
		BAD_SEED = 128,
		ACT_MALFORMED = 256,
		INVALID_TX_JSON = 512,
		INVALID_FIELD = 1024,
		NO_PATH = 2048,
		FAILED_TO_GET_SEQUENCE = 4096
	};
	struct ErrorInfo
	{
		ErrorInfo(error_code_i code_, std::string const& token_,
			std::string const& message_)
			: code(code_)
			, token(token_)
			, message(message_)
		{ }

		error_code_i code;
		std::string token;
		std::string message;

	void ErrorInfo::InjectTo(rapidjson::Value& json, rapidjson::MemoryPoolAllocator<>& allocator) const;

	};
	class ErrorsContainer
	{
		friend class ErrorCategory;
	public :
		class ErrorCategory
		{
			public:
				ErrorCategory();
				ErrorInfo ErrorsContainer::ErrorCategory::get (error_code_i code, std::string* params = nullptr, __int32 paramsNum = 0) const;
			private :
				typedef std::map <error_code_i, ErrorInfo> Map;
				ErrorInfo m_unknown;
				Map map;
				void add (error_code_i code, std::string const& token, std::string const& message);
		};
		static const ErrorCategory Errors;
		
	};
}
#endif

