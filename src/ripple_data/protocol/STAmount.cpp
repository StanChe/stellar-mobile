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

#include "../../beast/beast/cxx14/iterator.h"
#include "../../Utils/Helper.h"
#include <regex>

namespace ripple {

std::uint64_t STAmount::uRateOne  = STAmount::getRate (STAmount (1), STAmount (1));

bool STAmount::issuerFromString (uint160& uDstIssuer, const std::string& sIssuer)
{
    bool    bSuccess    = true;

    if (sIssuer.size () == (160 / 4))
    {
        uDstIssuer.SetHex (sIssuer);
    }
    else
    {
        RippleAddress raIssuer;

        if (raIssuer.setAccountID (sIssuer))
        {
            uDstIssuer  = raIssuer.getAccountID ();
        }
        else
        {
            bSuccess    = false;
        }
    }

    return bSuccess;
}

// --> sCurrency: "", "STR", or three letter ISO code.
bool STAmount::currencyFromString (uint160& uDstCurrency, const std::string& sCurrency)
{
    bool    bSuccess    = true;

    if (sCurrency.empty () || !sCurrency.compare (SYSTEM_CURRENCY_CODE))
    {
        uDstCurrency.zero ();
    }
    else if (3 == sCurrency.size ())
    {
        Blob    vucIso (3);

        std::transform (sCurrency.begin (), sCurrency.end (), vucIso.begin (), ::toupper);

        // std::string  sIso;
        // sIso.assign(vucIso.begin(), vucIso.end());
        // Log::out() << "currency: " << sIso;

        Serializer  s;

        s.addZeros (96 / 8);
        s.addRaw (vucIso);
        s.addZeros (16 / 8);
        s.addZeros (24 / 8);

        s.get160 (uDstCurrency, 0);
    }
    else if (40 == sCurrency.size ())
    {
        bSuccess    = uDstCurrency.SetHex (sCurrency);
    }
    else
    {
        bSuccess    = false;
    }

    return bSuccess;
}

std::string STAmount::getHumanCurrency () const
{
    return createHumanCurrency (mCurrency);
}

bool STAmount::bSetJson (const Json::Value& jvSource)
{
    try
    {
        STAmount    saParsed (sfGeneric, jvSource);

        *this   = saParsed;

        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

STAmount::STAmount (SField::ref n, const Json::Value& v)
    : SerializedType (n), mValue (0), mOffset (0), mIsNegative (false)
{
    Json::Value value, currency, issuer;

    if (v.isObject ())
    {
        value       = v[jss::value];
        currency    = v[jss::currency];
        issuer      = v[jss::issuer];
    }
    else if (v.isArray ())
    {
        value = v.get (Json::UInt (0), 0);
        currency = v.get (Json::UInt (1), Json::nullValue);
        issuer = v.get (Json::UInt (2), Json::nullValue);
    }
    else if (v.isString ())
    {
        std::string val = v.asString ();
        std::vector<std::string> elements = stellar_mobile::Helper::split (val, "\t\n\r ,/");

        if (elements.size () > 3)
            throw std::runtime_error ("invalid amount string");

        value = elements[0];

        if (elements.size () > 1)
            currency = elements[1];

        if (elements.size () > 2)
            issuer = elements[2];
    }
    else
        value = v;

    mIsNative = !currency.isString () || currency.asString ().empty () || (currency.asString () == SYSTEM_CURRENCY_CODE);

    if (mIsNative)
    {
        if (v.isObject ())
            throw std::runtime_error ("STR may not be specified as an object");
    }
    else
    {
        // non-STR
        if (!currencyFromString (mCurrency, currency.asString ()))
            throw std::runtime_error ("invalid currency");

        if (!issuer.isString ()
                || !issuerFromString (mIssuer, issuer.asString ()))
            throw std::runtime_error ("invalid issuer");

        if (mIssuer.isZero ())
            throw std::runtime_error ("invalid issuer");
    }

    if (value.isInt ())
    {
        if (value.asInt () >= 0)
            mValue = value.asInt ();
        else
        {
            mValue = -value.asInt ();
            mIsNegative = true;
        }

        canonicalize ();
    }
    else if (value.isUInt ())
    {
        mValue = v.asUInt ();

        canonicalize ();
    }
    else if (value.isString ())
    {
        if (mIsNative)
        {
            std::int64_t val = beast::lexicalCastThrow <std::int64_t> (value.asString ());

            if (val >= 0)
                mValue = val;
            else
            {
                mValue = -val;
                mIsNegative = true;
            }

            canonicalize ();
        }
        else
        {
            setValue (value.asString ());
        }
    }
    else
        throw std::runtime_error ("invalid amount type");
}

std::string STAmount::createHumanCurrency (const uint160& uCurrency)
{
    static uint160 const sIsoBits ("FFFFFFFFFFFFFFFFFFFFFFFF000000FFFFFFFFFF");

    // Characters we are willing to include the ASCII representation
    // of a three-letter currency code
    static std::string legalASCIICurrencyCharacters =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789"
        "<>(){}[]|?!@#$%^&*";
    
    if (uCurrency.isZero ())
    {
        return SYSTEM_CURRENCY_CODE;
    }

    if (CURRENCY_ONE == uCurrency)
    {
        return "1";
    }

    if ((uCurrency & sIsoBits).isZero ())
    {
        // The offset of the 3 character ISO code in the currency descriptor
        int const isoOffset = 12;

        std::string const iso(
            uCurrency.data () + isoOffset,
            uCurrency.data () + isoOffset + 3);

        // Specifying the system currency code using ISO-style representation
        // is not allowed.
        if ((iso != SYSTEM_CURRENCY_CODE) &&
            (iso.find_first_not_of(legalASCIICurrencyCharacters) == std::string::npos))
        {
            return iso;
        }
    }

    return to_string (uCurrency);
}

bool STAmount::setValue (const std::string& sAmount)
{
    // Note: mIsNative and mCurrency must be set already!

    static std::regex reNumber ("\\`([+-]?)(\\d*)(\\.(\\d*))?([eE]([+-]?)(\\d+))?\\'");
	std::cmatch  smMatch;

    if (!std::regex_match (sAmount.c_str(), smMatch, reNumber))
    {
        return false;
    }

    // Match fields: 0 = whole input, 1 = sign, 2 = integer portion, 3 = whole fraction (with '.')
    // 4 = fraction (without '.'), 5 = whole exponent (with 'e'), 6 = exponent sign, 7 = exponent number

    try
    {
        if ((smMatch[2].length () + smMatch[4].length ()) > 32)
        {
            return false;
        }

        mIsNegative = (smMatch[1].matched && (smMatch[1].str() == "-"));

        if (!smMatch[4].matched) // integer only
        {
            mValue = beast::lexicalCast <std::uint64_t> (smMatch[2].str());
            mOffset = 0;
        }
        else
        {
            // integer and fraction
            mValue = beast::lexicalCast <std::uint64_t> (smMatch[2].str() + smMatch[4].str());
            mOffset = - (smMatch[4].length ());
        }

        if (smMatch[5].matched)
        {
            // we have an exponent
            if (smMatch[6].matched && (smMatch[6].str() == "-"))
                mOffset -= beast::lexicalCast <int> (smMatch[7].str());
            else
                mOffset += beast::lexicalCast <int> (smMatch[7].str());
        }
    }
    catch (...)
    {
        return false;
    }

    if (mIsNative)
    {
        if (smMatch[3].matched)
            mOffset -= SYSTEM_CURRENCY_PRECISION;

        while (mOffset > 0)
        {
            mValue  *= 10;
            --mOffset;
        }

        while (mOffset < 0)
        {
            mValue  /= 10;
            ++mOffset;
        }
    }
    else
        canonicalize ();

    return true;
}

// Not meant to be the ultimate parser.  For use by RPC which is supposed to be sane and trusted.
// Native has special handling:
// - Integer values are in base units.
// - Float values are in float units.
// - To avoid a mistake float value for native are specified with a "^" in place of a "."
// <-- bValid: true = valid
bool STAmount::setFullValue (const std::string& sAmount, const std::string& sCurrency, const std::string& sIssuer)
{
    //
    // Figure out the currency.
    //
    if (!currencyFromString (mCurrency, sCurrency))
    {

        return false;
    }

    mIsNative   = !mCurrency;

    //
    // Figure out the issuer.
    //
    RippleAddress   naIssuerID;

    // Issuer must be "" or a valid account string.
    if (!naIssuerID.setAccountID (sIssuer))
    {
        return false;
    }

    mIssuer = naIssuerID.getAccountID ();

    // Stamps not must have an issuer.
    if (mIsNative && !mIssuer.isZero ())
    {
        return false;
    }

    return setValue (sAmount);
}

// amount = value * [10 ^ offset]
// representation range is 10^80 - 10^(-80)
// on the wire, high 8 bits are (offset+142), low 56 bits are value
// value is zero if amount is zero, otherwise value is 10^15 to (10^16 - 1) inclusive

void STAmount::canonicalize ()
{
    if (mCurrency.isZero ())
    {
        // native currency amounts should always have an offset of zero
        mIsNative = true;

        if (mValue == 0)
        {
            mOffset = 0;
            mIsNegative = false;
            return;
        }

        while (mOffset < 0)
        {
            mValue /= 10;
            ++mOffset;
        }

        while (mOffset > 0)
        {
            mValue *= 10;
            --mOffset;
        }

        if (mValue > cMaxNative)
            throw std::runtime_error ("Native currency amount out of range");

        return;
    }

    mIsNative = false;

    if (mValue == 0)
    {
        mOffset = -100;
        mIsNegative = false;
        return;
    }

    while ((mValue < cMinValue) && (mOffset > cMinOffset))
    {
        mValue *= 10;
        --mOffset;
    }

    while (mValue > cMaxValue)
    {
        if (mOffset >= cMaxOffset)
            throw std::runtime_error ("value overflow");

        mValue /= 10;
        ++mOffset;
    }

    if ((mOffset < cMinOffset) || (mValue < cMinValue))
    {
        mValue = 0;
        mOffset = 0;
        mIsNegative = false;
    }

    if (mOffset > cMaxOffset)
        throw std::runtime_error ("value overflow");

    assert ((mValue == 0) || ((mValue >= cMinValue) && (mValue <= cMaxValue)));
    assert ((mValue == 0) || ((mOffset >= cMinOffset) && (mOffset <= cMaxOffset)));
    assert ((mValue != 0) || (mOffset != -100));
}

void STAmount::add (Serializer& s) const
{
    if (mIsNative)
    {
        assert (mOffset == 0);

        if (!mIsNegative)
            s.add64 (mValue | cPosNative);
        else
            s.add64 (mValue);
    }
    else
    {
        if (*this == zero)
            s.add64 (cNotNative);
        else if (mIsNegative) // 512 = not native
            s.add64 (mValue | (static_cast<std::uint64_t> (mOffset + 512 + 97) << (64 - 10)));
        else // 256 = positive
            s.add64 (mValue | (static_cast<std::uint64_t> (mOffset + 512 + 256 + 97) << (64 - 10)));

        s.add160 (mCurrency);
        s.add160 (mIssuer);
    }
}

STAmount STAmount::createFromInt64 (SField::ref name, std::int64_t value)
{
    return value >= 0
           ? STAmount (name, static_cast<std::uint64_t> (value), false)
           : STAmount (name, static_cast<std::uint64_t> (-value), true);
}

void STAmount::setValue (const STAmount& a)
{
    mCurrency   = a.mCurrency;
    mIssuer     = a.mIssuer;
    mValue      = a.mValue;
    mOffset     = a.mOffset;
    mIsNative   = a.mIsNative;
    mIsNegative = a.mIsNegative;
}

int STAmount::compare (const STAmount& a) const
{
    // Compares the value of a to the value of this STAmount, amounts must be comparable
    if (mIsNegative != a.mIsNegative) return mIsNegative ? -1 : 1;

    if (!mValue)
    {
        if (a.mIsNegative) return 1;

        return a.mValue ? -1 : 0;
    }

    if (!a.mValue) return 1;

    if (mOffset > a.mOffset) return mIsNegative ? -1 : 1;

    if (mOffset < a.mOffset) return mIsNegative ? 1 : -1;

    if (mValue > a.mValue) return mIsNegative ? -1 : 1;

    if (mValue < a.mValue) return mIsNegative ? 1 : -1;

    return 0;
}

STAmount* STAmount::construct (SerializerIterator& sit, SField::ref name)
{
    std::uint64_t value = sit.get64 ();

    if ((value & cNotNative) == 0)
    {
        // native
        if ((value & cPosNative) != 0)
            return new STAmount (name, value & ~cPosNative, false); // positive
        else if (value == 0)
            throw std::runtime_error ("negative zero is not canonical");

        return new STAmount (name, value, true); // negative
    }

    uint160 uCurrencyID = sit.get160 ();

    if (!uCurrencyID)
        throw std::runtime_error ("invalid non-native currency");

    uint160 uIssuerID = sit.get160 ();

    int offset = static_cast<int> (value >> (64 - 10)); // 10 bits for the offset, sign and "not native" flag
    value &= ~ (1023ull << (64 - 10));

    if (value)
    {
        bool isNegative = (offset & 256) == 0;
        offset = (offset & 255) - 97; // center the range

        if ((value < cMinValue) || (value > cMaxValue) || (offset < cMinOffset) || (offset > cMaxOffset))
            throw std::runtime_error ("invalid currency value");

        return new STAmount (name, uCurrencyID, uIssuerID, value, offset, isNegative);
    }

    if (offset != 512)
        throw std::runtime_error ("invalid currency value");

    return new STAmount (name, uCurrencyID, uIssuerID);
}

std::int64_t STAmount::getSNValue () const
{
    // signed native value
    if (!mIsNative) throw std::runtime_error ("not native");

    if (mIsNegative) return - static_cast<std::int64_t> (mValue);

    return static_cast<std::int64_t> (mValue);
}

void STAmount::setSNValue (std::int64_t v)
{
    if (!mIsNative) throw std::runtime_error ("not native");

    if (v > 0)
    {
        mIsNegative = false;
        mValue = static_cast<std::uint64_t> (v);
    }
    else
    {
        mIsNegative = true;
        mValue = static_cast<std::uint64_t> (-v);
    }
}

std::string STAmount::getText () const
{
    // keep full internal accuracy, but make more human friendly if posible
    if (*this == zero)
        return "0";

    std::string const raw_value (std::to_string (mValue));
    std::string ret;

    if (mIsNegative)
        ret.append (1, '-');

    bool const scientific ((mOffset != 0) && ((mOffset < -25) || (mOffset > -5)));

    if (mIsNative || scientific)
    {
        ret.append (raw_value);

        if(scientific)
        {
            ret.append (1, 'e');
            ret.append (std::to_string (mOffset));
        }

        return ret;
    }

    assert (mOffset + 43 > 0);

    size_t const pad_prefix = 27;
    size_t const pad_suffix = 23;

    std::string val;
    val.reserve (raw_value.length () + pad_prefix + pad_suffix);
    val.append (pad_prefix, '0');
    val.append (raw_value);
    val.append (pad_suffix, '0');

    size_t const offset (mOffset + 43);

    auto pre_from (val.begin ());
    auto const pre_to (val.begin () + offset);

    auto const post_from (val.begin () + offset);
    auto post_to (val.end ());

    // Crop leading zeroes. Take advantage of the fact that there's always a
    // fixed amount of leading zeroes and skip them.
    if (std::distance (pre_from, pre_to) > pad_prefix)
        pre_from += pad_prefix;

    assert (post_to >= post_from);

    pre_from = std::find_if (pre_from, pre_to,
        [](char c)
        {
            return c != '0';
        });

    // Crop trailing zeroes. Take advantage of the fact that there's always a
    // fixed amount of trailing zeroes and skip them.
    if (std::distance (post_from, post_to) > pad_suffix)
        post_to -= pad_suffix;

    assert (post_to >= post_from);

    post_to = std::find_if(
        std::make_reverse_iterator (post_to),
        std::make_reverse_iterator (post_from),
        [](char c)
        {
            return c != '0';
        }).base();

    // Assemble the output:
    if (pre_from == pre_to)
        ret.append (1, '0');
    else
        ret.append(pre_from, pre_to);

    if (post_to != post_from)
    {
        ret.append (1, '.');
        ret.append (post_from, post_to);
    }

    return ret;
}

bool STAmount::isComparable (const STAmount& t) const
{
    // are these two STAmount instances in the same currency
    if (mIsNative) return t.mIsNative;

    if (t.mIsNative) return false;

    return mCurrency == t.mCurrency;
}

bool STAmount::isEquivalent (const SerializedType& t) const
{
    const STAmount* v = dynamic_cast<const STAmount*> (&t);

    if (!v) return false;

    return isComparable (*v) && (mIsNegative == v->mIsNegative) && (mValue == v->mValue) && (mOffset == v->mOffset);
}

void STAmount::throwComparable (const STAmount& t) const
{
    // throw an exception if these two STAmount instances are incomparable
    if (!isComparable (t))
        throw std::runtime_error ("amounts are not comparable");
}

bool STAmount::operator== (const STAmount& a) const
{
    return isComparable (a) && (mIsNegative == a.mIsNegative) && (mOffset == a.mOffset) && (mValue == a.mValue);
}

bool STAmount::operator!= (const STAmount& a) const
{
    return (mOffset != a.mOffset) || (mValue != a.mValue) || (mIsNegative != a.mIsNegative) || !isComparable (a);
}

bool STAmount::operator< (const STAmount& a) const
{
    throwComparable (a);
    return compare (a) < 0;
}

bool STAmount::operator> (const STAmount& a) const
{
    throwComparable (a);
    return compare (a) > 0;
}

bool STAmount::operator<= (const STAmount& a) const
{
    throwComparable (a);
    return compare (a) <= 0;
}

bool STAmount::operator>= (const STAmount& a) const
{
    throwComparable (a);
    return compare (a) >= 0;
}

STAmount& STAmount::operator+= (const STAmount& a)
{
    *this = *this + a;
    return *this;
}

STAmount& STAmount::operator-= (const STAmount& a)
{
    *this = *this - a;
    return *this;
}

STAmount STAmount::operator- (void) const
{
    if (mValue == 0) return *this;

    return STAmount (getFName (), mCurrency, mIssuer, mValue, mOffset, mIsNative, !mIsNegative);
}

STAmount& STAmount::operator= (std::uint64_t v)
{
    // does not copy name, does not change currency type
    mOffset = 0;
    mValue = v;
    mIsNegative = false;

    if (!mIsNative) canonicalize ();

    return *this;
}

STAmount& STAmount::operator+= (std::uint64_t v)
{
    assert (mIsNative);

    if (!mIsNative)
        throw std::runtime_error ("not native");

    setSNValue (getSNValue () + static_cast<std::int64_t> (v));
    return *this;
}

STAmount& STAmount::operator-= (std::uint64_t v)
{
    assert (mIsNative);

    if (!mIsNative)
        throw std::runtime_error ("not native");

    setSNValue (getSNValue () - static_cast<std::int64_t> (v));
    return *this;
}

bool STAmount::operator< (std::uint64_t v) const
{
    return getSNValue () < static_cast<std::int64_t> (v);
}

bool STAmount::operator> (std::uint64_t v) const
{
    return getSNValue () > static_cast<std::int64_t> (v);
}

bool STAmount::operator<= (std::uint64_t v) const
{
    return getSNValue () <= static_cast<std::int64_t> (v);
}

bool STAmount::operator>= (std::uint64_t v) const
{
    return getSNValue () >= static_cast<std::int64_t> (v);
}

STAmount STAmount::operator+ (std::uint64_t v) const
{
    return STAmount (getFName (), getSNValue () + static_cast<std::int64_t> (v));
}

STAmount STAmount::operator- (std::uint64_t v) const
{
    return STAmount (getFName (), getSNValue () - static_cast<std::int64_t> (v));
}

STAmount::operator double () const
{
    // Does not keep the precise value. Not recommended
    if (!mValue)
        return 0.0;

    if (mIsNegative) return -1.0 * static_cast<double> (mValue) * pow (10.0, mOffset);

    return static_cast<double> (mValue) * pow (10.0, mOffset);
}

STAmount operator+ (const STAmount& v1, const STAmount& v2)
{
    v1.throwComparable (v2);

    if (v2 == zero)
        return v1;

    if (v1 == zero)
    {
        // Result must be in terms of v1 currency and issuer.
        return STAmount (v1.getFName (), v1.mCurrency, v1.mIssuer, v2.mValue, v2.mOffset, v2.mIsNegative);
    }

    if (v1.mIsNative)
        return STAmount (v1.getFName (), v1.getSNValue () + v2.getSNValue ());

    int ov1 = v1.mOffset, ov2 = v2.mOffset;
    std::int64_t vv1 = static_cast<std::int64_t> (v1.mValue), vv2 = static_cast<std::int64_t> (v2.mValue);

    if (v1.mIsNegative) vv1 = -vv1;

    if (v2.mIsNegative) vv2 = -vv2;

    while (ov1 < ov2)
    {
        vv1 /= 10;
        ++ov1;
    }

    while (ov2 < ov1)
    {
        vv2 /= 10;
        ++ov2;
    }

    // this addition cannot overflow an std::int64_t, it can overflow an STAmount and the constructor will throw

    std::int64_t fv = vv1 + vv2;

    if ((fv >= -10) && (fv <= 10))
        return STAmount (v1.getFName (), v1.mCurrency, v1.mIssuer);
    else if (fv >= 0)
        return STAmount (v1.getFName (), v1.mCurrency, v1.mIssuer, fv, ov1, false);
    else
        return STAmount (v1.getFName (), v1.mCurrency, v1.mIssuer, -fv, ov1, true);
}

STAmount operator- (const STAmount& v1, const STAmount& v2)
{
    v1.throwComparable (v2);

    if (v2 == zero)
        return v1;

    if (v2.mIsNative)
    {
        // XXX This could be better, check for overflow and that maximum range is covered.
        return STAmount::createFromInt64 (v1.getFName (), v1.getSNValue () - v2.getSNValue ());
    }

    int ov1 = v1.mOffset, ov2 = v2.mOffset;
    std::int64_t vv1 = static_cast<std::int64_t> (v1.mValue), vv2 = static_cast<std::int64_t> (v2.mValue);

    if (v1.mIsNegative) vv1 = -vv1;

    if (v2.mIsNegative) vv2 = -vv2;

    while (ov1 < ov2)
    {
        vv1 /= 10;
        ++ov1;
    }

    while (ov2 < ov1)
    {
        vv2 /= 10;
        ++ov2;
    }

    // this subtraction cannot overflow an std::int64_t, it can overflow an STAmount and the constructor will throw

    std::int64_t fv = vv1 - vv2;

    if ((fv >= -10) && (fv <= 10))
        return STAmount (v1.getFName (), v1.mCurrency, v1.mIssuer);
    else if (fv >= 0)
        return STAmount (v1.getFName (), v1.mCurrency, v1.mIssuer, fv, ov1, false);
    else
        return STAmount (v1.getFName (), v1.mCurrency, v1.mIssuer, -fv, ov1, true);
}

STAmount STAmount::divide (const STAmount& num, const STAmount& den, const uint160& uCurrencyID, const uint160& uIssuerID)
{
    if (den == zero)
        throw std::runtime_error ("division by zero");

    if (num == zero)
        return STAmount (uCurrencyID, uIssuerID);

    std::uint64_t numVal = num.mValue, denVal = den.mValue;
    int numOffset = num.mOffset, denOffset = den.mOffset;

    if (num.mIsNative)
        while (numVal < STAmount::cMinValue)
        {
            // Need to bring into range
            numVal *= 10;
            --numOffset;
        }

    if (den.mIsNative)
        while (denVal < STAmount::cMinValue)
        {
            denVal *= 10;
            --denOffset;
        }

    // Compute (numerator * 10^17) / denominator
    CBigNum v;

    if ((BN_add_word64 (&v, numVal) != 1) ||
            (BN_mul_word64 (&v, tenTo17) != 1) ||
            (BN_div_word64 (&v, denVal) == ((std::uint64_t) - 1)))
    {
        throw std::runtime_error ("internal bn error");
    }

    // 10^16 <= quotient <= 10^18
    assert (BN_num_bytes (&v) <= 64);

    return STAmount (uCurrencyID, uIssuerID, v.getuint64 () + 5,
                     numOffset - denOffset - 17, num.mIsNegative != den.mIsNegative);
}

STAmount STAmount::multiply (const STAmount& v1, const STAmount& v2, const uint160& uCurrencyID, const uint160& uIssuerID)
{
    if (v1 == zero || v2 == zero)
        return STAmount (uCurrencyID, uIssuerID);

    if (v1.mIsNative && v2.mIsNative && uCurrencyID.isZero ())
    {
        std::uint64_t minV = (v1.getSNValue () < v2.getSNValue ()) ? v1.getSNValue () : v2.getSNValue ();
        std::uint64_t maxV = (v1.getSNValue () < v2.getSNValue ()) ? v2.getSNValue () : v1.getSNValue ();

        if (minV > 3000000000ull) // sqrt(cMaxNative)
            throw std::runtime_error ("Native value overflow");

        if (((maxV >> 32) * minV) > 2095475792ull) // cMaxNative / 2^32
            throw std::runtime_error ("Native value overflow");

        return STAmount (v1.getFName (), minV * maxV);
    }

    std::uint64_t value1 = v1.mValue, value2 = v2.mValue;
    int offset1 = v1.mOffset, offset2 = v2.mOffset;

    if (v1.mIsNative)
    {
        while (value1 < STAmount::cMinValue)
        {
            value1 *= 10;
            --offset1;
        }
    }

    if (v2.mIsNative)
    {
        while (value2 < STAmount::cMinValue)
        {
            value2 *= 10;
            --offset2;
        }
    }

    // Compute (numerator * denominator) / 10^14 with rounding
    // 10^16 <= result <= 10^18
    CBigNum v;

    if ((BN_add_word64 (&v, value1) != 1) ||
            (BN_mul_word64 (&v, value2) != 1) ||
            (BN_div_word64 (&v, tenTo14) == ((std::uint64_t) - 1)))
    {
        throw std::runtime_error ("internal bn error");
    }

    // 10^16 <= product <= 10^18
    assert (BN_num_bytes (&v) <= 64);

    return STAmount (uCurrencyID, uIssuerID, v.getuint64 () + 7, offset1 + offset2 + 14,
                     v1.mIsNegative != v2.mIsNegative);
}

// Convert an offer into an index amount so they sort by rate.
// A taker will take the best, lowest, rate first.
// (e.g. a taker will prefer pay 1 get 3 over pay 1 get 2.
// --> offerOut: takerGets: How much the offerer is selling to the taker.
// -->  offerIn: takerPays: How much the offerer is receiving from the taker.
// <--    uRate: normalize(offerIn/offerOut)
//             A lower rate is better for the person taking the order.
//             The taker gets more for less with a lower rate.
// Zero is returned if the offer is worthless.
std::uint64_t STAmount::getRate (const STAmount& offerOut, const STAmount& offerIn)
{
    if (offerOut == zero)
        return 0;

    try
    {
        STAmount r = divide (offerIn, offerOut, CURRENCY_ONE, ACCOUNT_ONE);

        if (r == zero) // offer is too good
            return 0;

        assert ((r.getExponent () >= -100) && (r.getExponent () <= 155));

        std::uint64_t ret = r.getExponent () + 100;

        return (ret << (64 - 8)) | r.getMantissa ();
    }
    catch (...)
    {
        // overflow -- very bad offer
        return 0;
    }
}

STAmount STAmount::setRate (std::uint64_t rate)
{
    if (rate == 0)
        return STAmount (CURRENCY_ONE, ACCOUNT_ONE);

    std::uint64_t mantissa = rate & ~ (255ull << (64 - 8));
    int exponent = static_cast<int> (rate >> (64 - 8)) - 100;

    return STAmount (CURRENCY_ONE, ACCOUNT_ONE, mantissa, exponent);
}

STAmount STAmount::getPay (const STAmount& offerOut, const STAmount& offerIn, const STAmount& needed)
{
    // Someone wants to get (needed) out of the offer, how much should they pay in?
    if (offerOut == zero)
        return STAmount (offerIn.getCurrency (), offerIn.getIssuer ());

    if (needed >= offerOut)
    {
        // They need more than offered, pay full amount.
        return needed;
    }

    STAmount ret = divide (multiply (needed, offerIn, CURRENCY_ONE, ACCOUNT_ONE), offerOut, offerIn.getCurrency (), offerIn.getIssuer ());

    return (ret > offerIn) ? offerIn : ret;
}

STAmount STAmount::deserialize (SerializerIterator& it)
{
    std::unique_ptr<STAmount> s (dynamic_cast<STAmount*> (construct (it, sfGeneric)));
    STAmount ret (*s);
    return ret;
}

std::string STAmount::getFullText () const
{
    std::string ret;

    ret.reserve(64);
    ret = getText () + "/" + getHumanCurrency ();

    if (!mIsNative)
    {
        ret += "/";

        if (!mIssuer)
            ret += "0";
        else if (mIssuer == ACCOUNT_ONE)
            ret += "1";
        else
            ret += RippleAddress::createHumanAccountID (mIssuer);
    }

    return ret;
}

STAmount STAmount::getRound () const
{
    if (mIsNative)
        return *this;

    std::uint64_t valueDigits = mValue % 1000000000ull;

    if (valueDigits == 1)
        return STAmount (mCurrency, mIssuer, mValue - 1, mOffset, mIsNegative);
    else if (valueDigits == 999999999ull)
        return STAmount (mCurrency, mIssuer, mValue + 1, mOffset, mIsNegative);

    return *this;
}

void STAmount::roundSelf ()
{
    if (mIsNative)
        return;

    std::uint64_t valueDigits = mValue % 1000000000ull;

    if (valueDigits == 1)
    {
        mValue -= 1;

        if (mValue < cMinValue)
            canonicalize ();
    }
    else if (valueDigits == 999999999ull)
    {
        mValue += 1;

        if (mValue > cMaxValue)
            canonicalize ();
    }
}

void STAmount::setJson (Json::Value& elem) const
{
    elem = Json::objectValue;

    if (!mIsNative)
    {
        // It is an error for currency or issuer not to be specified for valid json.

        elem[jss::value]      = getText ();
        elem[jss::currency]   = getHumanCurrency ();
        elem[jss::issuer]     = RippleAddress::createHumanAccountID (mIssuer);
    }
    else
    {
        elem = getText ();
    }
}

Json::Value STAmount::getJson (int) const
{
    Json::Value elem;
    setJson (elem);
    return elem;
}

//------------------------------------------------------------------------------

} // ripple
