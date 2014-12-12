//------------------------------------------------------------------------------
/*
    This file is part of Beast: https://github.com/vinniefalco/Beast
    Copyright 2013, Vinnie Falco <vinnie.falco@gmail.com>

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


namespace beast {

RandomAccessFile::RandomAccessFile () noexcept
    : fileHandle (nullptr)
    , currentPosition (0)
{
}

RandomAccessFile::~RandomAccessFile ()
{
    close ();
}

Result RandomAccessFile::open (File const& path, Mode mode)
{
    close ();

    return nativeOpen (path, mode);
}

void RandomAccessFile::close ()
{
    if (isOpen ())
    {
        nativeFlush ();
        nativeClose ();
    }
}

Result RandomAccessFile::setPosition (FileOffset newPosition)
{
    if (newPosition != currentPosition)
    {
        // VFALCO NOTE I dislike return from the middle but
        //             Result::ok() is showing up in the profile
        //
        return nativeSetPosition (newPosition);
    }

    return Result::ok ();
}

Result RandomAccessFile::read (void* buffer, ByteCount numBytes, ByteCount* pActualAmount)
{
    return nativeRead (buffer, numBytes, pActualAmount);
}

Result RandomAccessFile::write (const void* data, ByteCount numBytes, ByteCount* pActualAmount)
{
    bassert (data != nullptr && ((std::ptrdiff_t) numBytes) >= 0);

    Result result (Result::ok ());

    ByteCount amountWritten = 0;

    result = nativeWrite (data, numBytes, &amountWritten);

    if (result.wasOk ())
        currentPosition += amountWritten;

    if (pActualAmount != nullptr)
        *pActualAmount = amountWritten;

    return result;
}

Result RandomAccessFile::truncate ()
{
    Result result = flush ();

    if (result.wasOk ())
        result = nativeTruncate ();

    return result;
}

Result RandomAccessFile::flush ()
{
    return nativeFlush ();
}

//------------------------------------------------------------------------------

} // beast
