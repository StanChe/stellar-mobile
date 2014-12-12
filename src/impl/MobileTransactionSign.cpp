
#include "../StellarMobile.h"
#include "../rapidjson/include/rapidjson/writer.h"
#include "../Utils/JSS.h"

#include "../ripple_rpc/impl/TransactionSign.h"

namespace stellar_mobile
{
	void StellarMobile::doTransactionSign(rapidjson::Document& request, rapidjson::Value& result, rapidjson::MemoryPoolAllocator<>& allocator)
	{
		bool bFailHard = request[JSS::PARAMS][0].HasMember(JSS::PARAM_FAIL_HARD) && request[JSS::PARAMS][0][JSS::PARAM_FAIL_HARD].GetBool();
		ripple::RPC::transactionSign(request, result, allocator, bFailHard);
	}
}