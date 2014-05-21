/*

Copyright (c) 2013, Project OSRM, Dennis Luxen, others
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include <osrm/Reply.h>

#include "../../Util/StringUtil.h"

namespace http
{

void Reply::setSize(const unsigned size)
{
    for (Header &h : headers)
    {
        if ("Content-Length" == h.name)
        {
            h.value = IntToString(size);
        }
    }
}

// Sets the size of the uncompressed output.
void Reply::SetUncompressedSize()
{
    unsigned uncompressed_size = 0;
    uncompressed_size = content.size();
    setSize(uncompressed_size);
}

std::vector<boost::asio::const_buffer> Reply::toBuffers()
{
    std::vector<boost::asio::const_buffer> buffers;
    buffers.push_back(ToBuffer(status));
    for (const Header &h : headers)
    {
        buffers.push_back(boost::asio::buffer(h.name));
        buffers.push_back(boost::asio::buffer(seperators));
        buffers.push_back(boost::asio::buffer(h.value));
        buffers.push_back(boost::asio::buffer(crlf));
    }
    buffers.push_back(boost::asio::buffer(crlf));
    buffers.push_back(boost::asio::buffer(content));
    return buffers;
}

std::vector<boost::asio::const_buffer> Reply::HeaderstoBuffers()
{
    std::vector<boost::asio::const_buffer> buffers;
    buffers.push_back(ToBuffer(status));
    for (std::size_t i = 0; i < headers.size(); ++i)
    {
        Header &current_header = headers[i];
        buffers.push_back(boost::asio::buffer(current_header.name));
        buffers.push_back(boost::asio::buffer(seperators));
        buffers.push_back(boost::asio::buffer(current_header.value));
        buffers.push_back(boost::asio::buffer(crlf));
    }
    buffers.push_back(boost::asio::buffer(crlf));
    return buffers;
}

Reply Reply::StockReply(Reply::status_type status)
{
    Reply rep;
    rep.status = status;
    rep.content.clear();

    const std::string status_string = rep.ToString(status);
    rep.content.insert(rep.content.end(), status_string.begin(), status_string.end());
    rep.headers.resize(3);
    rep.headers[0].name = "Access-Control-Allow-Origin";
    rep.headers[0].value = "*";
    rep.headers[1].name = "Content-Length";

    std::string size_string = IntToString(rep.content.size());

    rep.headers[1].value = size_string;
    rep.headers[2].name = "Content-Type";
    rep.headers[2].value = "text/html";
    return rep;
}

std::string Reply::ToString(Reply::status_type status)
{
    if (Reply::ok == status)
    {
        return okHTML;
    }
    if (Reply::badRequest == status)
    {
        return badRequestHTML;
    }
    return internalServerErrorHTML;
}

boost::asio::const_buffer Reply::ToBuffer(Reply::status_type status)
{
    if (Reply::ok == status)
    {
        return boost::asio::buffer(okString);
    }
    if (Reply::internalServerError == status)
    {
        return boost::asio::buffer(internalServerErrorString);
    }
    return boost::asio::buffer(badRequestString);
}

Reply::Reply() : status(ok) {}
}
