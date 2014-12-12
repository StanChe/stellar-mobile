//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012, 2013 Ripple Labs Inc.

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

#include <boost\make_shared.hpp>
#include "Transaction.h"
#include <locale>

namespace ripple {

Transaction::Transaction (SerializedTransaction::ref sit, bool bValidate)
: mInLedger(0), mStatus(INVALID), mResult(temUNCERTAIN), mSerializedTransaction(sit)
{
    try
    {
		mFromPubKey.setAccountPublic(mSerializedTransaction->getSigningPubKey());
		mTransactionID = mSerializedTransaction->getTransactionID();
		mAccountFrom = mSerializedTransaction->getSourceAccount();
    }
    catch (...)
    {
        return;
    }

	mStatus = NEW;
}

//
// Generic transaction construction
//

Transaction::Transaction (
    TxType ttKind,
    const RippleAddress&    naPublicKey,
    const RippleAddress&    naSourceAccount,
    std::uint32_t           uSeq,
    const STAmount&         saFee,
    std::uint32_t           uSourceTag) :
    mAccountFrom (naSourceAccount), mFromPubKey (naPublicKey), mInLedger (0), mStatus (NEW), mResult (temUNCERTAIN)
{
    assert (mFromPubKey.isValid ());

	mSerializedTransaction = boost::make_shared<SerializedTransaction>(ttKind);

    // Log(lsINFO) << str(boost::format("Transaction: account: %s") % naSourceAccount.humanAccountID());
    // Log(lsINFO) << str(boost::format("Transaction: mAccountFrom: %s") % mAccountFrom.humanAccountID());

	mSerializedTransaction->setSigningPubKey(mFromPubKey);
	mSerializedTransaction->setSourceAccount(mAccountFrom);
	mSerializedTransaction->setSequence(uSeq);
	mSerializedTransaction->setTransactionFee(saFee);

    if (uSourceTag)
    {
		mSerializedTransaction->makeFieldPresent(sfSourceTag);
		mSerializedTransaction->setFieldU32(sfSourceTag, uSourceTag);
    }
}

bool Transaction::checkCoherent()
{
	if (!passesLocalChecks(*mSerializedTransaction)) return(false);
	
	if (!mFromPubKey.isValid())
	{
		return false;
	}

	if (!mSerializedTransaction->checkSign(mFromPubKey)) return(false);
	// TODO: check that it doesn't have weird fields

	return(true);
}

bool Transaction::sign (const RippleAddress& naAccountPrivate)
{
    bool    bResult = true;

    if (!naAccountPrivate.isValid ())
    {
        bResult = false;
    }

    getSTransaction ()->sign (naAccountPrivate);

    if (bResult)
    {
        updateID ();
    }
    else
    {
        mStatus = INCOMPLETE;
    }

    return bResult;
}





//
// Misc.
//


void Transaction::setStatus (TransStatus ts, std::uint32_t lseq)
{
    mStatus     = ts;
    mInLedger   = lseq;
}

// options 1 to include the date of the transaction
Json::Value Transaction::getJson (int options, bool binary) const
{
	Json::Value ret(mSerializedTransaction->getJson(0, binary));
    return ret;
}

bool Transaction::isHexTxID (const std::string& txid)
{
    if (txid.size () != 64) 
        return false;

    auto const ret = std::find_if_not (txid.begin (), txid.end (),
        std::bind (
            std::isxdigit <std::string::value_type>,
            std::placeholders::_1,
            std::locale ()));

    return (ret == txid.end ());
}

} // ripple
