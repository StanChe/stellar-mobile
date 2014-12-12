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

#include <sodium.h>
#include "../ripple_data/crypto/StellarPrivateKey.h"
#include "../ripple_data/crypto/StellarPublicKey.h"
#include "../../ripple/common/UnorderedContainers.h"

namespace ripple {


RippleAddress::RippleAddress ()
    : mIsValid (false)
{
    nVersion = VER_NONE;
}

RippleAddress::RippleAddress(const RippleAddress& naSeed, VersionEncoding type)
{
	assert(0);
}

void RippleAddress::clear ()
{
    nVersion = VER_NONE;
    vchData.clear ();
    mIsValid = false;
}

bool RippleAddress::isSet () const
{
    return nVersion != VER_NONE;
}

std::string RippleAddress::humanAddressType () const
{
    switch (nVersion)
    {
    case VER_NONE:
        return "VER_NONE";

    case VER_NODE_PUBLIC:
        return "VER_NODE_PUBLIC";

    case VER_NODE_PRIVATE:
        return "VER_NODE_PRIVATE";

    case VER_ACCOUNT_ID:
        return "VER_ACCOUNT_ID";

    case VER_ACCOUNT_PUBLIC:
        return "VER_ACCOUNT_PUBLIC";

    case VER_ACCOUNT_PRIVATE:
        return "VER_ACCOUNT_PRIVATE";


	case VER_SEED:
        return "VER_SEED";
    }

    return "unknown";
}

//
// NodePublic
//

RippleAddress RippleAddress::createNodePublic (const RippleAddress& naSeed)
{
    EdKeyPair ckSeed (naSeed.getSeed ());
    RippleAddress   naNew;

    // YYY Should there be a GetPubKey() equiv that returns a uint256?
    naNew.setNodePublic (ckSeed.getPubKey ());

    return naNew;
}

RippleAddress RippleAddress::createNodePublic (Blob const& vPublic)
{
    RippleAddress   naNew;

    naNew.setNodePublic (vPublic);

    return naNew;
}

RippleAddress RippleAddress::createNodePublic (const std::string& strPublic)
{
    RippleAddress   naNew;

    naNew.setNodePublic (strPublic);

    return naNew;
}

uint160 RippleAddress::getNodeID () const
{
    switch (nVersion)
    {
    case VER_NONE:
        throw std::runtime_error ("unset source - getNodeID");

    case VER_NODE_PUBLIC:
        // Note, we are encoding the left.
        return Hash160 (vchData);

    default:
		throw std::runtime_error(("bad source: ") + std::to_string(int(nVersion)));
    }
}
Blob const& RippleAddress::getNodePublic () const
{
    switch (nVersion)
    {
    case VER_NONE:
        throw std::runtime_error ("unset source - getNodePublic");

    case VER_NODE_PUBLIC:
        return vchData;

    default:
        throw std::runtime_error(("bad source: ") + std::to_string(int(nVersion)));
    }
}

std::string RippleAddress::human(Blob& blob, VersionEncoding type)
{
	Blob vch(1, type);
	
	vch.insert(vch.end(), blob.begin(), blob.end());

	return Base58::encodeWithCheck(vch);
}

std::string RippleAddress::humanNodePublic () const
{
    switch (nVersion)
    {
    case VER_NONE:
        throw std::runtime_error ("unset source - humanNodePublic");

    case VER_NODE_PUBLIC:
        return ToString ();

    default:
        throw std::runtime_error(("bad source: ") + std::to_string(int(nVersion)));
    }
}

bool RippleAddress::setNodePublic (const std::string& strPublic)
{
    mIsValid = SetString (strPublic, VER_NODE_PUBLIC, Base58::getRippleAlphabet ());

    return mIsValid;
}

void RippleAddress::setNodePublic (Blob const& vPublic)
{
    mIsValid        = true;

    SetData (VER_NODE_PUBLIC, vPublic);
}


bool RippleAddress::verifySignature(uint256 const& hash, const std::string& strSig) const
{
	Blob vchSig(strSig.begin(), strSig.end());
	return(verifySignature(hash, vchSig));
}

bool RippleAddress::verifySignature(uint256 const& hash, Blob const& vchSig) const
{
    if (vchData.size() != crypto_sign_PUBLICKEYBYTES
        || vchSig.size () != crypto_sign_BYTES)
        throw std::runtime_error("bad inputs to verifySignature");

    bool verified = crypto_sign_verify_detached(vchSig.data(),
                 hash.data(), hash.bytes, vchData.data()) == 0;
    bool canonical = signatureIsCanonical (vchSig);
    return verified && canonical;
}

/**
 * This is a helper function that appeared briedly in libsodium:
 *
 *   https://github.com/jedisct1/libsodium/commit/4099618de2cce5099ac2ec5ce8f2d80f4585606e
 *
 * and was subsequently removed to maintain backward compatibility:
 *
 *   https://github.com/jedisct1/libsodium/commit/cb07df046f19ee0d5ad600c579df97aaa4295cc3
 *
 * It is used to check canonical representation of signatures; that is, to
 * prevent anyone from presenting multiple forms of valid signature, which
 * might permit a "transaction malleability" attack on certain gateways or
 * wallets that treat transactions as identified by their hash.
 *
 * At present (2014-10-22, v1.0.0) libsodium is still deciding on the
 * interface with which to expose this functionality (likely to add an
 * is_standard()/is_canonical() function) but has not done so yet, so for
 * the time being we copy-and-paste their code here. We contacted Frank
 * Denis (jedisct1) about this and this was his recommended course of
 * action.
 *
 * When libsodium presents such an interface, delete this code and
 * use their interface instead.
 */
static int
crypto_sign_check_S_lt_l(const unsigned char *S)
{
    static const unsigned char l[32] =
      { 0xed, 0xd3, 0xf5, 0x5c, 0x1a, 0x63, 0x12, 0x58,
        0xd6, 0x9c, 0xf7, 0xa2, 0xde, 0xf9, 0xde, 0x14,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10 };
    unsigned char c = 0;
    unsigned char n = 1;
    unsigned int  i = 32;

    do {
        i--;
        c |= ((S[i] - l[i]) >> 8) & n;
        n &= ((S[i] ^ l[i]) - 1) >> 8;
    } while (i != 0);

    return -(c == 0);
}

bool RippleAddress::signatureIsCanonical(Blob const& vchSig)
{
    return crypto_sign_check_S_lt_l(
        ((const unsigned char*) vchSig.data ()) + 32
        ) == 0;
}

void RippleAddress::sign(uint256 const& message, Blob& retSignature) const
{
	assert(vchData.size() == 64);

	const unsigned char *key = vchData.data();
	retSignature.resize(crypto_sign_BYTES);
	crypto_sign_detached(&retSignature[0], NULL,
		(unsigned char*)message.begin(), message.size(),
		key);
}


//
// NodePrivate
//



Blob const& RippleAddress::getNodePrivateData () const
{
    switch (nVersion)
    {
    case VER_NONE:
        throw std::runtime_error ("unset source - getNodePrivateData");

    case VER_NODE_PRIVATE:
        return vchData;

    default:
        throw std::runtime_error(("bad source: ") + std::to_string(int(nVersion)));
    }
}

uint256 RippleAddress::getNodePrivate () const
{
    switch (nVersion)
    {
    case VER_NONE:
        throw std::runtime_error ("unset source = getNodePrivate");

    case VER_NODE_PRIVATE:
        return uint256 (vchData);

    default:
        throw std::runtime_error(("bad source: ") + std::to_string(int(nVersion)));
    }
}

std::string RippleAddress::humanNodePrivate () const
{
    switch (nVersion)
    {
    case VER_NONE:
        throw std::runtime_error ("unset source - humanNodePrivate");

    case VER_NODE_PRIVATE:
        return ToString ();

    default:
        throw std::runtime_error(("bad source: ") + std::to_string(int(nVersion)));
    }
}

bool RippleAddress::setNodePrivate (const std::string& strPrivate)
{
    mIsValid = SetString (strPrivate, VER_NODE_PRIVATE, Base58::getRippleAlphabet ());

	if (mIsValid) mIsValid = (vchData.size() == crypto_sign_SECRETKEYBYTES);

    return mIsValid;
}

void RippleAddress::setNodePrivate (Blob const& vPrivate)
{
    mIsValid = true;
	

    SetData (VER_NODE_PRIVATE, vPrivate);
	assert(vchData.size() == crypto_sign_SECRETKEYBYTES);
}

void RippleAddress::setNodePrivate (uint256 hash256)
{
    mIsValid = true;

    SetData (VER_NODE_PRIVATE, hash256);
	assert(vchData.size() == crypto_sign_SECRETKEYBYTES);
}





//
// AccountID
//

uint160 RippleAddress::getAccountID () const
{
    switch (nVersion)
    {
    case VER_NONE:
        throw std::runtime_error ("unset source - getAccountID");

    case VER_ACCOUNT_ID:
        return uint160 (vchData);

    case VER_ACCOUNT_PUBLIC:
        // Note, we are encoding the left.
        return Hash160 (vchData);

    default:
        throw std::runtime_error(("bad source: ") + std::to_string(int(nVersion)));
    }
}

typedef RippleMutex StaticLockType;
typedef std::lock_guard <StaticLockType> StaticScopedLockType;
static StaticLockType s_lock;

static ripple::unordered_map< Blob , std::string > rncMap;

std::string RippleAddress::humanAccountID () const
{
    switch (nVersion)
    {
    case VER_NONE:
        throw std::runtime_error ("unset source - humanAccountID");

    case VER_ACCOUNT_ID:
    {
        StaticScopedLockType sl (s_lock);

        auto it = rncMap.find (vchData);

        if (it != rncMap.end ())
            return it->second;

        // VFALCO NOTE Why do we throw everything out? We could keep two maps
        //             here, switch back and forth keep one of them full and clear the
        //             other on a swap - but always check both maps for cache hits.
        //
        if (rncMap.size () > 250000)
            rncMap.clear ();

        return rncMap[vchData] = ToString ();
    }

    case VER_ACCOUNT_PUBLIC:
    {
        RippleAddress   accountID;

        (void) accountID.setAccountID (getAccountID ());

        return accountID.ToString ();
    }

    default:
        throw std::runtime_error(("bad source: ") + std::to_string(int(nVersion)));
    }
}

bool RippleAddress::setAccountID (const std::string& strAccountID, Base58::Alphabet const& alphabet)
{
    if (strAccountID.empty ())
    {
        setAccountID (uint160 ());

        mIsValid    = true;
    }
    else
    {
        mIsValid = SetString (strAccountID, VER_ACCOUNT_ID, alphabet);
    }

    return mIsValid;
}

void RippleAddress::setAccountID (const uint160& hash160)
{
    mIsValid        = true;

    SetData (VER_ACCOUNT_ID, hash160);
}

//
// AccountPublic
//

RippleAddress RippleAddress::createAccountPublic (const RippleAddress& seed)
{
	EdKeyPair       ckPub(seed.getSeed());
    RippleAddress   naNew;

    naNew.setAccountPublic (ckPub.getPubKey ());

    return naNew;
}

Blob const& RippleAddress::getAccountPublic () const
{
    switch (nVersion)
    {
    case VER_NONE:
        throw std::runtime_error ("unset source - getAccountPublic");

    case VER_ACCOUNT_ID:
        throw std::runtime_error ("public not available from account id");
        break;

    case VER_ACCOUNT_PUBLIC:
        return vchData;

    default:
        throw std::runtime_error(("bad source: ") + std::to_string(int(nVersion)));
    }
}

std::string RippleAddress::humanAccountPublic () const
{
    switch (nVersion)
    {
    case VER_NONE:
        throw std::runtime_error ("unset source - humanAccountPublic");

    case VER_ACCOUNT_ID:
        throw std::runtime_error ("public not available from account id");

    case VER_ACCOUNT_PUBLIC:
        return ToString ();

    default:
        throw std::runtime_error(("bad source: ") + std::to_string(int(nVersion)));
    }
}

bool RippleAddress::setAccountPublic (const std::string& strPublic)
{
    mIsValid = SetString (strPublic, VER_ACCOUNT_PUBLIC, Base58::getRippleAlphabet ());

    return mIsValid;
}

void RippleAddress::setAccountPublic (Blob const& vPublic)
{
    mIsValid = true;

    SetData (VER_ACCOUNT_PUBLIC, vPublic);
}


void RippleAddress::setAccountPublic (const RippleAddress& seed )
{
	EdKeyPair pubkey(seed.getSeed());

    setAccountPublic (pubkey.getPubKey ());
}


RippleAddress RippleAddress::createAccountID (const uint160& uiAccountID)
{
    RippleAddress   na;

    na.setAccountID (uiAccountID);

    return na;
}

//
// AccountPrivate
//

RippleAddress RippleAddress::createAccountPrivate (const RippleAddress& naSeed)
{
    RippleAddress   naNew;
	EdKeyPair pair(naSeed.getSeed());

    naNew.setAccountPrivate (pair.mPrivateKey);

    return naNew;
}

RippleAddress RippleAddress::createNodePrivate(const RippleAddress& naSeed)
{
	RippleAddress   naNew;
	EdKeyPair pair(naSeed.getSeed());

	naNew.setNodePrivate(pair.mPrivateKey);

	return naNew;
}

uint256 RippleAddress::getAccountPrivate () const
{
    switch (nVersion)
    {
    case VER_NONE:
        throw std::runtime_error ("unset source - getAccountPrivate");

    case VER_ACCOUNT_PRIVATE:
        return uint256 (vchData);

    default:
        throw std::runtime_error(("bad source: ") + std::to_string(int(nVersion)));
    }
}

std::string RippleAddress::humanAccountPrivate () const
{
    switch (nVersion)
    {
    case VER_NONE:
        throw std::runtime_error ("unset source - humanAccountPrivate");

    case VER_ACCOUNT_PRIVATE:
        return ToString ();

    default:
        throw std::runtime_error(("bad source: ") + std::to_string(int(nVersion)));
    }
}

bool RippleAddress::setAccountPrivate (const std::string& strPrivate)
{
    mIsValid = SetString (strPrivate, VER_ACCOUNT_PRIVATE, Base58::getRippleAlphabet ());
	if(mIsValid) mIsValid=(vchData.size() == crypto_sign_SECRETKEYBYTES);

    return mIsValid;
}

void RippleAddress::setAccountPrivate (Blob const& vPrivate)
{
    mIsValid        = true;

    SetData (VER_ACCOUNT_PRIVATE, vPrivate);
	assert(vchData.size() == crypto_sign_SECRETKEYBYTES);
}

void RippleAddress::setAccountPrivate (uint256 hash256)
{
    mIsValid = true;

    SetData (VER_ACCOUNT_PRIVATE, hash256);
	assert(vchData.size() == crypto_sign_SECRETKEYBYTES);
}
/*
void RippleAddress::setAccountPrivate (const RippleAddress& naGenerator, const RippleAddress& naSeed)
{
	assert(0);
	
    CKey    ckPubkey    = CKey (naSeed.getSeed ());
    CKey    ckPrivkey   = CKey (naGenerator, ckPubkey.GetSecretBN (), seq);
    uint256 uPrivKey;

    ckPrivkey.GetPrivateKeyU (uPrivKey);

    setAccountPrivate (uPrivKey);

	
}
*/




Blob RippleAddress::accountPrivateDecrypt (const RippleAddress& naPublicFrom, Blob const& vucCipherText) const
{

    Blob    vucPlainText;
	assert(0);

	/* TEMP 
    if (!ckPublic.SetPubKey (naPublicFrom.getAccountPublic ()))
    {
        // Bad public key.
        WriteLog (lsWARNING, RippleAddress) << "accountPrivateDecrypt: Bad public key.";
    }
    else if (!ckPrivate.SetPrivateKeyU (getAccountPrivate ()))
    {
        // Bad private key.
        WriteLog (lsWARNING, RippleAddress) << "accountPrivateDecrypt: Bad private key.";
    }
    else
    {
        try
        {
            vucPlainText = ckPrivate.decryptECIES (ckPublic, vucCipherText);
        }
        catch (...)
        {
            nothing ();
        }
    }
	*/

    return vucPlainText;
}



//
// Seed
//

uint256 RippleAddress::getSeed() const
{
    switch (nVersion)
    {
    case VER_NONE:
        throw std::runtime_error ("unset source - getSeed");

	case VER_SEED:
		return uint256(vchData);

    default:
        throw std::runtime_error(("bad source: ") + std::to_string(int(nVersion)));
    }
}



std::string RippleAddress::humanSeed () const
{
    switch (nVersion)
    {
    case VER_NONE:
        throw std::runtime_error ("unset source - humanSeed");

	case VER_SEED:
        return ToString ();

    default:
        throw std::runtime_error(("bad source: ") + std::to_string(int(nVersion)));
    }
}


bool RippleAddress::setSeed (const std::string& strSeed)
{
	mIsValid = SetString(strSeed, VER_SEED, Base58::getRippleAlphabet());

    return mIsValid;
}

bool RippleAddress::setSeedGeneric (const std::string& strText)
{
    RippleAddress   naTemp;
    bool            bResult = true;
	uint256         uSeed;

    if (strText.empty ()
            || naTemp.setAccountID (strText)
            || naTemp.setAccountPublic (strText)
            || naTemp.setAccountPrivate (strText)
            || naTemp.setNodePublic (strText)
            || naTemp.setNodePrivate (strText))
    {
        bResult = false;
    }
    else if (strText.length () == 32 && uSeed.SetHex (strText, true))
    {
        setSeed (uSeed);
    }
    else if (setSeed (strText))
    {
        // Log::out() << "Recognized seed.";
        nothing ();
    }
    else
    {
        // Log::out() << "Creating seed from pass phrase.";
        setSeed (EdKeyPair::passPhraseToKey (strText));
    }

    return bResult;
}

void RippleAddress::setSeed(uint256 seed)
{
    mIsValid = true;

	SetData(VER_SEED, seed);
}

void RippleAddress::setSeedRandom ()
{
    // XXX Maybe we should call MakeNewKey
	uint256 key;

    RandomNumbers::getInstance ().fillBytes (key.begin (), key.size ());

    RippleAddress::setSeed (key);
}

RippleAddress RippleAddress::createSeedRandom ()
{
    RippleAddress   naNew;

    naNew.setSeedRandom ();

    return naNew;
}

RippleAddress RippleAddress::createSeedGeneric (const std::string& strText)
{
    RippleAddress   naNew;

    naNew.setSeedGeneric (strText);

    return naNew;
}

//------------------------------------------------------------------------------
} // ripple
