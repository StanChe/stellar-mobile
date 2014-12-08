#include "JSS.h"
#include "Helper.h"
#include "ErrorCategory.h"

namespace stellar_mobile
{
	const ErrorsContainer::ErrorCategory ErrorsContainer::Errors;

	void ErrorInfo::InjectTo(rapidjson::Value& json, rapidjson::MemoryPoolAllocator<>& allocator) const
	{
		Helper::AddMember(json, JSS::ERROR_NAME, token.c_str(), allocator);
		Helper::AddMember(json, JSS::ERROR_CODE, rapidjson::Value(code), allocator);
		Helper::AddMember(json,JSS::ERROR_MESSAGE, message.c_str(), allocator);
	}

	ErrorsContainer::ErrorCategory::ErrorCategory() : m_unknown (UNKNOWN, "UNKNOWN", "An unknown error code.")
	{
		add(UNPARSABLE_RESPONSE_JSON, "unparsableJson", "Can not parse json.");
		add(UNKNOWN_METHOD, "unknownMethod", "Unknown method.");
		add(INVALID_PARAMS, "invalidParams", "Field 'params' has to contain %s.");
		add(INVALID_TX_JSON, "invalidTxJson", "Field 'tx_json' has to contain %s.");
		add(INVALID_FIELD, "invalidField", "Field '%s' is invalid");
		add(INVALID_JSON, "invalidJson", "Json has to contain %s.");
		add(BAD_SEED, "badSeed", "Disallowed seed.");
		add(INVALID_PARAM_TYPE, "invalidParamType", "Fild '%s' has invalid type");
		add(CANT_GET_DATA_FROM_STELLARD, "cantGetDataFromStellard", "Can't get data from stellard server.");
		add(ACT_MALFORMED, "actMalformed", "Account malformed.");
		add(NO_PATH, "noPath", "Unable to find a ripple path.");
		add(FAILED_TO_GET_SEQUENCE, "FailedToGetSequence", "Failed to get sequece.");
	}
	ErrorInfo ErrorsContainer::ErrorCategory::get (error_code_i code, std::string* params, __int32 paramsNum) const
	{
		Map::const_iterator const iter (map.find (code));
		if (iter != map.end()) 
		{
			ErrorInfo error = iter->second;
			if (params != nullptr && paramsNum > 0)
				error.message = Helper::StringFormat(error.message, params, paramsNum);
			return error;
		}
		return m_unknown;
	}
	void ErrorsContainer::ErrorCategory::add(error_code_i code, std::string const& token,  std::string const& message)
	{
		std::pair <Map::iterator, bool> result (
			map.emplace (std::piecewise_construct,
				std::forward_as_tuple (code), std::forward_as_tuple (
					code, token, message)));
		if (! result.second)
			throw std::invalid_argument ("duplicate error code");
	}
}
