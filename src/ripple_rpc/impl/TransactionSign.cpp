
#include "TransactionSign.h"
#include "../../Utils/JSS.h"
#include "../../Utils/ErrorCategory.h"
#include "../../ripple_data/protocol/RippleAddress.h"
#include "../../ripple_data/protocol/TxFlags.h"
#include "../../ripple/json/ripple_json.h"
#include "../../ripple_data/ripple_data.h"
#include "../../ripple_app/tx/Transaction.h"
#include "../../ripple_app/misc/SerializedTransaction.h"
#include <boost\make_shared.hpp>

namespace ripple {

#define DEFAULT_FEE_DEFAULT 10

	namespace RPC {
		//------------------------------------------------------------------------------

		int64_t GetSequence(std::string account)
		{
			using namespace rapidjson;
			Document request;
			request.SetObject();
			stellar_mobile::Helper::AddMember(request, stellar_mobile::JSS::METHOD_PAR, "account_info", request.GetAllocator());
			Value params;
			params.SetObject();
			stellar_mobile::Helper::AddMember(params, stellar_mobile::JSS::PARAM_ACCOUNT, account.c_str(), request.GetAllocator());
			stellar_mobile::Helper::AddMember(request, stellar_mobile::JSS::PARAMS, Value(kArrayType), request.GetAllocator());
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
			}
		}

		bool GetPathFindReques(const RippleAddress& srcAccountID, const RippleAddress& dstAccountID, rapidjson::Value& dstAmount, rapidjson::Value& uPaths, rapidjson::MemoryPoolAllocator<>& uPathsAllocator) {
			rapidjson::Document request;
			request.SetObject();
			rapidjson::MemoryPoolAllocator<>& requestAllocator = request.GetAllocator();
			stellar_mobile::Helper::AddMember(request, stellar_mobile::JSS::METHOD_PAR, "static_path_find", requestAllocator);
			rapidjson::Value params;
			params.SetObject();
			stellar_mobile::Helper::AddMember(params, stellar_mobile::JSS::PARAM_SOURCE_ACCOUNT, srcAccountID.humanAccountID().c_str(), requestAllocator);
			stellar_mobile::Helper::AddMember(params, stellar_mobile::JSS::PARAM_DESTINATION_ACCOUNT, dstAccountID.humanAccountID().c_str(), requestAllocator);
			stellar_mobile::Helper::AddMember(params, stellar_mobile::JSS::PARAM_DESTINATION_AMOUNT, dstAmount, requestAllocator);
			rapidjson::Value paramsCont;
			paramsCont.SetArray();
			paramsCont.PushBack(params, requestAllocator);
			stellar_mobile::Helper::AddMember(request, stellar_mobile::JSS::PARAMS, paramsCont, requestAllocator);
			rapidjson::Document response;
			rapidjson::Value result;
			stellar_mobile::Helper::redirectRequest(request, result, response.GetAllocator());
			if (result.HasMember(stellar_mobile::JSS::ERROR_NAME))
				return false;
			std::cout << stellar_mobile::Helper::AsString(result) << std::endl;
			uPaths = rapidjson::Value(result, uPathsAllocator);
			return true;
		}

		bool signPayment(
			rapidjson::Value& params,
			rapidjson::Value& result,
			RippleAddress const& raSrcAddressID,
			rapidjson::MemoryPoolAllocator<>& allocator,
			rapidjson::MemoryPoolAllocator<>& paramsAllocator)
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
				&& tx_json.HasMember(stellar_mobile::JSS::PARAM_TX_JSON_AMOUNT) && tx_json[stellar_mobile::JSS::PARAM_TX_JSON_AMOUNT].IsObject())
			{
				// Need a ripple path.
				uint160     uSrcCurrencyID;
				uint160     uSrcIssuerID;

				rapidjson::Value spsPaths;
				stellar_mobile::Helper::AddMember(tx_json, "Paths", spsPaths, paramsAllocator);
				bool bValid = GetPathFindReques(raSrcAddressID, dstAccountID,
					tx_json[stellar_mobile::JSS::PARAM_TX_JSON_AMOUNT], spsPaths, paramsAllocator);
				if (!bValid || spsPaths.Empty())
				{
					stellar_mobile::ErrorsContainer::Errors.get(stellar_mobile::NO_PATH).InjectTo(result, allocator);
					return false;
				}
			}
			return true;
		}

		void transactionSign(rapidjson::Document& request, rapidjson::Value& result, rapidjson::MemoryPoolAllocator<>& allocator, bool bFailHard)
		{
			rapidjson::Value& params = request[stellar_mobile::JSS::PARAMS][0];
			if (!params.HasMember(stellar_mobile::JSS::PARAM_SECRET))
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
			stellar_mobile::Helper::AddMember(tx_json, stellar_mobile::JSS::PARAM_TX_JSON_FEE, rapidjson::Value(0), request.GetAllocator());
			std::string const sType = tx_json[stellar_mobile::JSS::PARAM_TRANSACTION_TYPE].GetString();

			if ("Payment" == sType)
			{
				if (!signPayment(params, result, raSrcAddressID, allocator, request.GetAllocator()))
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
				stellar_mobile::Helper::AddMember(tx_json, stellar_mobile::JSS::PARAMS_TX_JSON_SEQUENCE, rapidjson::Value(seq + 1), request.GetAllocator());
			}

			if (!tx_json.HasMember (stellar_mobile::JSS::PARAM_TX_JSON_FLAGS))
				stellar_mobile::Helper::AddMember(tx_json, stellar_mobile::JSS::PARAM_TX_JSON_FLAGS, rapidjson::Value(tfFullyCanonicalSig), request.GetAllocator());

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
			std::string paramsString = stellar_mobile::Helper::AsString(params);
			Json::Value ripParams;
			Json::Reader reader;
			reader.parse(paramsString, ripParams);
			Json::Value& ripTx_json(ripParams[stellar_mobile::JSS::PARAM_TX_JSON]);
			Json::FastWriter w;
			std::cout << w.write(ripTx_json);
			STParsedJSON parsed(stellar_mobile::JSS::PARAM_TX_JSON, ripTx_json);
			if (!parsed.object.get())
			{
				std::string errorParams[1] = { stellar_mobile::JSS::PARAM_TX_JSON };
				stellar_mobile::ErrorsContainer::Errors.get(stellar_mobile::INVALID_JSON, errorParams, 1).InjectTo(result, allocator);
				return;
			}
			std::unique_ptr<STObject> sopTrans = std::move(parsed.object);
			sopTrans->setFieldVL (sfSigningPubKey, masterAccountPublic.getAccountPublic ());

			SerializedTransaction::pointer stpTrans = boost::make_shared<SerializedTransaction>(*sopTrans);
			if (params.HasMember(stellar_mobile::JSS::PARAM_DEBUG_SIGNING))
			{
				stellar_mobile::Helper::AddMember(result, stellar_mobile::JSS::TX_UNSIGNED, strHex(
					stpTrans->getSerializer().peekData()).c_str(), allocator);
				stellar_mobile::Helper::AddMember(result, stellar_mobile::JSS::TX_SIGNING_HASH, to_string(stpTrans->getSigningHash()).c_str(), allocator);
			}

			//// FIXME: For performance, transactions should not be signed in this code path.
			RippleAddress naAccountPrivate = RippleAddress::createAccountPrivate (naSecret);

			stpTrans->sign (naAccountPrivate);

			Transaction::pointer tpTrans;

			try
			{
				// FIXME: For performance, should use asynch interface
				Serializer s;
				stpTrans->add(s);
				rapidjson::Value paramTxJson(strHex(s.peekData()).c_str(), allocator);
				stellar_mobile::Helper::AddMember(result, stellar_mobile::JSS::PARAM_TX_BLOB, paramTxJson, allocator);
				std::string dep = stellar_mobile::Helper::AsString(paramTxJson);
			}
			catch (std::exception&)
			{
				stellar_mobile::ErrorsContainer::Errors.get(stellar_mobile::INVALID_TX_JSON).InjectTo(result, allocator);
				return;
			}
		}
	}
}