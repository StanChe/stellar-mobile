
#include "TransactionSign.h"
#include "../../Utils/JSS.h"
#include "../../Utils/ErrorCategory.h"
#include "../../ripple_data/protocol/RippleAddress.h"
#include "../../ripple_data/protocol/TxFlags.h"
namespace ripple {

#define DEFAULT_FEE_DEFAULT 10

	namespace RPC {
		//------------------------------------------------------------------------------

		int64_t GetSequence(std::string account)
		{
			/*using namespace rapidjson;
			Document request;
			request[stellar_mobile::JSS::METHOD_PAR] = "account_info";
			Value params;
			params.SetObject();
			params[stellar_mobile::JSS::PARAM_ACCOUNT] = account;
			request[stellar_mobile::JSS::PARAMS] = Value(kArrayType);
			request[stellar_mobile::JSS::PARAMS].PushBack(params, request.GetAllocator());
			Document response;
			stellar_mobile::Helper::redirectRequest(request, response, response.GetAllocator());
			if (response.HasMember(stellar_mobile::JSS::RESULT_FIELD)) {
				Value& result = response[stellar_mobile::JSS::RESULT_FIELD];
				if (result.HasMember(stellar_mobile::JSS::ACCOUNT_DATA_FIELD)) {
					Value& accountData = result[stellar_mobile::JSS::ACCOUNT_DATA_FIELD];
					if (accountData.HasMember(stellar_mobile::JSS::ACCOUNT_DATA_SEQUENCE))
						return accountData[stellar_mobile::JSS::ACCOUNT_DATA_SEQUENCE].GetInt64();
				}
			}*/
			return  -1;
		}

		bool GetPathFindReques(const RippleAddress& srcAccountID, const RippleAddress& dstAccountID, rapidjson::Value& dstAmount, rapidjson::Value& uPaths) {
			/*rapidjson::Document request;
			rapidjson::MemoryPoolAllocator<>& requestAllocator = request.GetAllocator();
			stellar_mobile::Helper::AddMember(request, stellar_mobile::JSS::METHOD_PAR, "static_path_find", requestAllocator);
			rapidjson::Value params;
			params.SetObject();
			params[stellar_mobile::JSS::PARAM_SOURCE_ACCOUNT] = srcAccountID.humanAccountID();
			params[stellar_mobile::JSS::PARAM_DESTINATION_ACCOUNT] = dstAccountID.humanAccountID();
			stellar_mobile::Helper::AddMember(params, stellar_mobile::JSS::PARAM_DESTINATION_AMOUNT, dstAmount, requestAllocator);
			rapidjson::Value paramsCont;
			paramsCont.SetArray();
			paramsCont.PushBack(params, requestAllocator);
			stellar_mobile::Helper::AddMember(request, stellar_mobile::JSS::PARAMS, paramsCont, requestAllocator);
			rapidjson::Document response;
			rapidjson::Value result;
			rapidjson::MemoryPoolAllocator<>& allocator = response.GetAllocator();
			stellar_mobile::Helper::redirectRequest(request, result, allocator);
			if (result.HasMember(stellar_mobile::JSS::ERROR_NAME))
				return false;
			uPaths = rapidjson::Value(response[stellar_mobile::JSS::RESULT_FIELD], allocator);*/
			return true;
		}

		bool signPayment(
			rapidjson::Document& params,
			rapidjson::Value& result,
			RippleAddress const& raSrcAddressID,
			rapidjson::MemoryPoolAllocator<>& allocator)
		{
			RippleAddress dstAccountID;
			rapidjson::Value& tx_json = params[stellar_mobile::JSS::PARAM_TX_JSON];
			if (!tx_json.HasMember(stellar_mobile::JSS::PARAM_TX_JSON_AMOUNT))
			{
				std::string errorParams[1] = { stellar_mobile::JSS::PARAM_TX_JSON_AMOUNT };
				stellar_mobile::ErrorsContainer::Errors.get(stellar_mobile::INVALID_TX_JSON, errorParams, 1).InjectTo(result, allocator);
				return false;
			}

			if (!tx_json.HasMember(stellar_mobile::JSS::PARAM_TX_JSON_DESTINATION))
			{
				std::string errorParams[1] = { stellar_mobile::JSS::PARAM_TX_JSON_DESTINATION };
				stellar_mobile::ErrorsContainer::Errors.get(stellar_mobile::INVALID_TX_JSON, errorParams, 1).InjectTo(result, allocator);
				return false;
			}

			if (!dstAccountID.setAccountID(tx_json[stellar_mobile::JSS::PARAM_TX_JSON_DESTINATION].GetString()))
			{
				std::string errorParams[1] = { stellar_mobile::JSS::PARAM_TX_JSON_DESTINATION };
				stellar_mobile::ErrorsContainer::Errors.get(stellar_mobile::INVALID_FIELD, errorParams, 1).InjectTo(result, allocator);
				return false;
			}

			if (tx_json.HasMember(stellar_mobile::JSS::PARAM_TX_JSON_PATHS) && params.HasMember(stellar_mobile::JSS::PARAM_BUILD_PATH))
			{
				std::string errorParams[1] = { "either " + std::string(stellar_mobile::JSS::PARAM_TX_JSON_PATHS) + " or " + stellar_mobile::JSS::PARAM_BUILD_PATH };
				stellar_mobile::ErrorsContainer::Errors.get(stellar_mobile::INVALID_FIELD, errorParams, 1).InjectTo(result, allocator);
				return false;
			}

			if (!tx_json.HasMember(stellar_mobile::JSS::PARAM_TX_JSON_PATHS)
				&& tx_json.HasMember(stellar_mobile::JSS::PARAM_TX_JSON_AMOUNT)
				&& params.HasMember(stellar_mobile::JSS::PARAM_BUILD_PATH))
			{
				// Need a ripple path.
				uint160     uSrcCurrencyID;
				uint160     uSrcIssuerID;

				rapidjson::Value spsPaths;

				bool bValid = GetPathFindReques(raSrcAddressID, dstAccountID,
					tx_json[stellar_mobile::JSS::PARAM_TX_JSON_AMOUNT], spsPaths);
				if (!bValid || spsPaths.Empty())
				{
					stellar_mobile::ErrorsContainer::Errors.get(stellar_mobile::NO_PATH).InjectTo(result, allocator);
					return false;
				}
				else
					tx_json["Paths"] = rapidjson::Value(spsPaths, params.GetAllocator());
			}
			return true;
		}

		void transactionSign(rapidjson::Value& params, rapidjson::Value& result, rapidjson::MemoryPoolAllocator<>& allocator, bool bFailHard)
		{
			/*if (!params.HasMember(stellar_mobile::JSS::PARAM_SECRET))
			{
				std::string errorParams [1]= {stellar_mobile::JSS::PARAM_SECRET};
				stellar_mobile::ErrorsContainer::Errors.get(stellar_mobile::INVALID_JSON, errorParams, 1).InjectTo(result, allocator);
				return;
			}

			if (!params.HasMember(stellar_mobile::JSS::PARAM_TX_JSON))
			{
				std::string errorParams [1]= {stellar_mobile::JSS::PARAM_TX_JSON};
				stellar_mobile::ErrorsContainer::Errors.get(stellar_mobile::INVALID_JSON, errorParams, 1).InjectTo(result, allocator);
				return;
			}

			RippleAddress naSeed;

			if (!naSeed.setSeedGeneric(params[stellar_mobile::JSS::PARAM_SECRET].GetString()))
			{
				stellar_mobile::ErrorsContainer::Errors.get(stellar_mobile::BAD_SEED).InjectTo(result, allocator);
				return;
			}

			rapidjson::Value& tx_json = params[stellar_mobile::JSS::PARAM_TX_JSON];

			if (!tx_json.IsObject())
			{
				std::string errorParams[2] = { stellar_mobile::JSS::PARAM_TX_JSON, "object" };
				stellar_mobile::ErrorsContainer::Errors.get(stellar_mobile::INVALID_PARAM_TYPE, errorParams, 2).InjectTo(result, allocator);
				return;
			}

			if (!tx_json.HasMember(stellar_mobile::JSS::PARAM_TRANSACTION_TYPE))
			{
				std::string errorParams[1] = { stellar_mobile::JSS::PARAM_TRANSACTION_TYPE };
				stellar_mobile::ErrorsContainer::Errors.get(stellar_mobile::INVALID_JSON, errorParams, 1).InjectTo(result, allocator);
				return;
			}
			if (!tx_json.HasMember(stellar_mobile::JSS::PARAM_ACCOUNT))
			{
				std::string errorParams[1] = { stellar_mobile::JSS::PARAM_ACCOUNT };
				stellar_mobile::ErrorsContainer::Errors.get(stellar_mobile::INVALID_JSON, errorParams, 1).InjectTo(result, allocator);
				return;
			}
			RippleAddress raSrcAddressID;
			if (!raSrcAddressID.setAccountID(tx_json[stellar_mobile::JSS::PARAM_ACCOUNT].GetString()))
			{
				stellar_mobile::ErrorsContainer::Errors.get(stellar_mobile::ACT_MALFORMED).InjectTo(result, allocator);
				return;
			}
			stellar_mobile::Helper::AddMember(tx_json, stellar_mobile::JSS::PARAM_TX_JSON_FEE, 0, allocator);

			std::string const sType = tx_json[stellar_mobile::JSS::PARAM_TRANSACTION_TYPE].GetString();

			if ("Payment" == sType)
			{
				if (!signPayment(params, result, raSrcAddressID, allocator))
					return;
			}

			if (!tx_json.HasMember(stellar_mobile::JSS::PARAM_TX_JSON_FEE)) {
				auto const& transactionType = tx_json[stellar_mobile::JSS::PARAM_TRANSACTION_TYPE].GetString();
				if ("AccountSet" == transactionType
					|| "OfferCreate" == transactionType
					|| "OfferCancel" == transactionType
					|| "TrustSet" == transactionType)
				{
					tx_json[stellar_mobile::JSS::PARAM_TX_JSON_FEE] = DEFAULT_FEE_DEFAULT;
				}
			}

			if (!tx_json.HasMember(stellar_mobile::JSS::PARAMS_TX_JSON_SEQUENCE))
			{
				int seq = GetSequence(tx_json[stellar_mobile::JSS::PARAM_ACCOUNT].GetString());
				if (seq < 0)
				{
					stellar_mobile::ErrorsContainer::Errors.get(stellar_mobile::FAILED_TO_GET_SEQUENCE).InjectTo(result, allocator);
					return;
				}
				tx_json[stellar_mobile::JSS::PARAMS_TX_JSON_SEQUENCE] = seq;

			}

			if (!tx_json.HasMember ("Flags"))
				tx_json["Flags"] = tfFullyCanonicalSig;*/

			//if (verify)
			//{
			//    SLE::pointer sleAccountRoot = netOps.getSLEi (lSnapshot,
			//        Ledger::getAccountRootIndex (raSrcAddressID.getAccountID ()));

			//    if (!sleAccountRoot)
			//        // XXX Ignore transactions for accounts not created.
			//        return rpcError (rpcSRC_ACT_NOT_FOUND);
			//}

			RippleAddress   naSecret = RippleAddress::createSeedGeneric (params[stellar_mobile::JSS::PARAM_SECRET].GetString ());
  
			RippleAddress masterAccountPublic = RippleAddress::createAccountPublic(naSecret);

			//STParsedJSON parsed ("tx_json", tx_json);
			//if (!parsed.object.get())
			//{
			//	jvResult ["error"] = parsed.error ["error"];
			//	jvResult ["error_code"] = parsed.error ["error_code"];
			//	jvResult ["error_message"] = parsed.error ["error_message"];
			//	return jvResult;
			//}
			//jvResult["done"] = "parsed tx_json";
			//std::unique_ptr<STObject> sopTrans = std::move(parsed.object);
			//sopTrans->setFieldVL (sfSigningPubKey, masterAccountPublic.getAccountPublic ());

			//SerializedTransaction::pointer stpTrans;

			//try
			//{
			//	stpTrans = boost::make_shared<SerializedTransaction> (*sopTrans);
			//}
			//catch (std::exception&)
			//{
			//	return RPC::make_error (rpcINTERNAL,
			//		"Exception occurred during transaction");
			//}
			//std::string reason;
			//if (!passesLocalChecks (*stpTrans, reason))
			//	return RPC::make_error (rpcINVALID_PARAMS, reason);

			//if (params.isMember ("debug_signing"))
			//{
			//	jvResult["tx_unsigned"] = strHex (
			//		stpTrans->getSerializer ().peekData ());
			//	jvResult["tx_signing_hash"] = to_string (stpTrans->getSigningHash ());
			//}

			//// FIXME: For performance, transactions should not be signed in this code path.
			//RippleAddress naAccountPrivate = RippleAddress::createAccountPrivate (naSecret);

			//stpTrans->sign (naAccountPrivate);

			//Transaction::pointer tpTrans;

			//try
			//{
			//	tpTrans     = boost::make_shared<Transaction> (stpTrans, false);
			//}
			//catch (const exception & exception)
			//{
			//	cout << exception.what() << endl;
			//	return RPC::make_error (rpcINTERNAL,
			//		"Exception occurred during transaction");
			//}
			//try
			//{
			//	// FIXME: For performance, should use asynch interface
			//	Serializer s;
			//	tpTrans->getSTransaction()->add(s);

			//	tpTrans = Transaction::sharedTransaction(s.getData(), true);
			//	if (!tpTrans)
			//	{
			//		return RPC::make_error (rpcINTERNAL,
			//			"Unable to sterilize transaction.");
			//	}
			//}
			//catch (std::exception&)
			//{
			//	return RPC::make_error (rpcINTERNAL,
			//		"Exception occurred during transaction submission.");
			//}
			//try
			//{
			//	jvResult["tx_json"] = tpTrans->getJson (0);
			//	jvResult["tx_blob"] = strHex (
			//		tpTrans->getSTransaction ()->getSerializer ().peekData ());

			//	if (temUNCERTAIN != tpTrans->getResult ())
			//	{
			//		std::string sToken;
			//		std::string sHuman;

			//		transResultInfo (tpTrans->getResult (), sToken, sHuman);

			//		jvResult["engine_result"]           = sToken;
			//		jvResult["engine_result_code"]      = tpTrans->getResult ();
			//		jvResult["engine_result_message"]   = sHuman;
			//	}
			//	return jvResult;
			//}
			//catch (std::exception&)
			//{
			//	/*return RPC::make_error (rpcINTERNAL,
			//		"Exception occurred during JSON handling.");*/
			//	return jvResult;
			//}
		}
	}
}