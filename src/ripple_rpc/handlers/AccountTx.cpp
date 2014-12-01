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
#include <string>
#include "../../beast/beast/http/curlReader.h"
namespace ripple {

// {
//   account: account,
//   ledger_index_min: ledger_index  // optional, defaults to earliest
//   ledger_index_max: ledger_index, // optional, defaults to latest
//   binary: boolean,                // optional, defaults to false
//   forward: boolean,               // optional, defaults to false
//   limit: integer,                 // optional
//   marker: opaque                  // optional, resume previous query
// }
	
Json::Value RPCHandler::doAccountTx (Json::Value params, Resource::Charge& loadType, Application::ScopedLockType& masterLockHolder)
{
	masterLockHolder.unlock();
	RippleAddress   raAccount;

    if (!params.isMember ("account"))
        return rpcError (rpcINVALID_PARAMS);

    if (!raAccount.setAccountID (params["account"].asString ()))
        return rpcError (rpcACT_MALFORMED);

    if (params.isMember ("ledger_index_min") || params.isMember ("ledger_index_max"))
    {
        std::int64_t iLedgerMin  = params.isMember ("ledger_index_min") ? params["ledger_index_min"].asInt () : -1;
        std::int64_t iLedgerMax  = params.isMember ("ledger_index_max") ? params["ledger_index_max"].asInt () : -1;
		std::uint32_t uLedgerMin = iLedgerMin == -1 ? 0 : iLedgerMin;
		if (iLedgerMax != -1 && iLedgerMax < uLedgerMin)
        {
            return rpcError (rpcLGR_IDXS_INVALID);
        }
    }

#ifndef BEAST_DEBUG

    try
    {
#endif
		Json::Value newParams = RPCHandler::CommandToMethod(params);
		Json::FastWriter writer;
		beast::CurlReader cReader;
		std::string data = cReader.performRequest(getConfig().STELLARD, writer.write(newParams));
		Json::Value jvResp;
		Json::Reader reader;
		reader.parse(data, jvResp);
		return jvResp;
#ifndef BEAST_DEBUG
    }
    catch (...)
    {
        return rpcError (rpcINTERNAL);
    }

#endif
}

} // ripple
