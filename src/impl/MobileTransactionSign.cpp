#include <curl/curl.h>

#include "../StellarMobile.h"
#include "../rapidjson/include/rapidjson/writer.h"
#include "../rapidjson/include/rapidjson/stringbuffer.h"
#include "../Utils/ErrorCategory.h"
#include "../Utils/Helper.h";
#include "../Utils/JSS.h"
#include "../Utils/CurlReader.h"

#include <iostream>
#include "../Utils/Config.h"
#include "../ripple_basics/utility/StringUtilities.h"
#include "../ripple_rpc/impl/TransactionSign.h"

namespace stellar_mobile
{
	void doTransactionSign(rapidjson::Document& params, rapidjson::Value& result, rapidjson::MemoryPoolAllocator<>& allocator)
	{
		bool bFailHard = params.HasMember(JSS::PARAM_FAIL_HARD) && params[JSS::PARAM_FAIL_HARD].GetBool();
		ripple::RPC::transactionSign(params, result, allocator, bFailHard);
	}
}