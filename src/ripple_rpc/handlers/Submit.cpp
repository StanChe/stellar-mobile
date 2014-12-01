//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012-2014 Ripple Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================
#include <iostream>

using namespace std;

namespace ripple {

// {
//   tx_json: <object>,
//   secret: <secret>
// }
	Json::Value RPCHandler::doSubmit(Json::Value params, Resource::Charge& loadType, Application::ScopedLockType& masterLockHolder)
	{
		masterLockHolder.unlock();

		loadType = Resource::feeMediumBurdenRPC;

		if (!params.isMember("tx_blob"))
		{
			bool bFailHard = params.isMember("fail_hard") && params["fail_hard"].asBool();
			return RPC::transactionSign(params, true, bFailHard, masterLockHolder, *mNetOps, mRole);
		}

		Json::Value                 jvResult;

		std::pair<Blob, bool> ret(strUnHex(params["tx_blob"].asString()));

		if (!ret.second || !ret.first.size())
			return rpcError(rpcINVALID_PARAMS);

		Serializer                  sTrans(ret.first);
		SerializerIterator          sitTrans(sTrans);

		SerializedTransaction::pointer stpTrans;

		try
		{
			stpTrans = boost::make_shared<SerializedTransaction>(boost::ref(sitTrans));
		}
		catch (std::exception& e)
		{
			jvResult[jss::error] = "invalidTransaction";
			jvResult["error_exception"] = e.what();

			return jvResult;
		}

		Transaction::pointer            tpTrans;

		try
		{
			tpTrans = boost::make_shared<Transaction>(stpTrans, false);
		}
		catch (std::exception& e)
		{
			jvResult[jss::error] = "internalTransaction";
			jvResult["error_exception"] = e.what();

			return jvResult;
		}

		try
		{
			Json::Value r;
			r["method"] = "submit";
			Json::Value newParams(Json::objectValue);
			newParams["tx_blob"] = params["tx_blob"].asString();
			r["params"] = Json::arrayValue;
			r["params"].append(newParams);
			Json::FastWriter writer;
			beast::CurlReader cReader;
			cout << writer.write(r) << endl;
			std::string data = cReader.performRequest(getConfig().STELLARD, writer.write(r));
			cout << data << endl;
			Json::Value jvResp;
			Json::Reader reader;
			reader.parse(data, jvResp);
			return jvResp;
		}
		catch (std::exception& e)
		{
			jvResult[jss::error] = "internalSubmit";
			jvResult[jss::error_exception] = e.what();

			return jvResult;
		}
	}

} // ripple
