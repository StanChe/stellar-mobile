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

#include "StringUtilities.h";
#include <string>
#include <cstdarg>

namespace ripple {


	// NIKB NOTE: This function is only used by strUnHex (const std::string& strSrc)
	// which results in a pointless copy from std::string into std::vector. Should
	// we just scrap this function altogether?
	int strUnHex (std::string& strDst, const std::string& strSrc)
	{
		std::string tmp;

		tmp.reserve ((strSrc.size () + 1) / 2);

		auto iter = strSrc.cbegin ();

		if (strSrc.size () & 1)
		{
			int c = charUnHex (*iter);

			if (c < 0)
				return -1;

			tmp.push_back(c);
			++iter;
		}

		while (iter != strSrc.cend ())
		{
			int cHigh = charUnHex (*iter);
			++iter;

			if (cHigh < 0)
				return -1;

			int cLow = charUnHex (*iter);
			++iter;

			if (cLow < 0)
				return -1;

			tmp.push_back (static_cast<char>((cHigh << 4) | cLow));
		}

		strDst = std::move(tmp);

		return strDst.size ();
	}


	std::pair<Blob, bool> strUnHex (const std::string& strSrc)
	{
		std::string strTmp;

		if (strUnHex (strTmp, strSrc) == -1)
			return std::make_pair (Blob (), false);

		return std::make_pair(strCopy (strTmp), true);
	}

	Blob strCopy (const std::string& strSrc)
	{
		Blob vucDst;

		vucDst.resize (strSrc.size ());

		std::copy (strSrc.begin (), strSrc.end (), vucDst.begin ());

		return vucDst;
	}

} // ripple
