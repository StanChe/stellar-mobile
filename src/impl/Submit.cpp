#include <curl/curl.h>

#include "../StellarMobile.h"
#include "../rapidjson/include/rapidjson/writer.h"
#include "../rapidjson/include/rapidjson/stringbuffer.h"
#include "../Utils/ErrorCategory.h"
#include "../Utils/Helper.h";
#include "../Utils/JSS.h"
#include "../Utils/CurlReader.h"

#include "../ripple_basics/ripple_basics.h"
#include <iostream>
#include "../Utils/Config.h"

namespace stellar_mobile
{
	void StellarMobile::doSubmit(rapidjson::Document& request, rapidjson::Value& result, rapidjson::MemoryPoolAllocator<>& allocator)
	{
		if (!request.HasMember(JSS::PARAMS))
		{
			std::string errorParams[1] = { JSS::PARAMS };
			ErrorsContainer::Errors.get(INVALID_JSON, errorParams, 1).InjectTo(result, allocator);
			return;
		}
		rapidjson::Value& params = request[JSS::PARAMS];
		if (!params.IsArray() || params.Size() == 0)
		{
			std::string errorParams[1] = { JSS::PARAMS };
			ErrorsContainer::Errors.get(INVALID_PARAM_TYPE, errorParams, 1).InjectTo(result, allocator);
			return;
		}
		rapidjson::Value& param = params[0];
		if (!param.HasMember(JSS::PARAM_TX_BLOB))
		{
			return doTransactionSign(request, result, allocator);
		}
		std::pair<ripple::Blob, bool> ret(ripple::strUnHex(param[JSS::PARAM_TX_BLOB].GetString()));

		if (!ret.second || !ret.first.size())
		{
			std::string errorParams [1]= {JSS::PARAM_TX_BLOB};
			ErrorsContainer::Errors.get(INVALID_JSON, errorParams, 1).InjectTo(result, allocator);
			return;
		}
		Helper::redirectRequest(request, result, allocator);
	}
}