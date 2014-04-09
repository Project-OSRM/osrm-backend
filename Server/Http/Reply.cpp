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

#include <boost/foreach.hpp>

#include <osrm/Reply.h>

#include "../../Util/StringUtil.h"

namespace http {

void Reply::setSize(const unsigned size) {
    BOOST_FOREACH ( Header& h,  headers) {
        if("Content-Length" == h.name) {
            std::string sizeString;
            intToString(size,h.value);
        }
    }
}

// Sets the size of the uncompressed output.
void Reply::SetUncompressedSize()
{
    unsigned uncompressed_size = 0;
    BOOST_FOREACH ( const std::string & current_line, content)
    {
        uncompressed_size += current_line.size();
    }
    setSize(uncompressed_size);
}

std::vector<boost::asio::const_buffer> Reply::toBuffers(){
    std::vector<boost::asio::const_buffer> buffers;
    buffers.push_back(ToBuffer(status));
    BOOST_FOREACH(const Header & h, headers) {
        buffers.push_back(boost::asio::buffer(h.name));
        buffers.push_back(boost::asio::buffer(seperators));
        buffers.push_back(boost::asio::buffer(h.value));
        buffers.push_back(boost::asio::buffer(crlf));
    }
    buffers.push_back(boost::asio::buffer(crlf));
    BOOST_FOREACH(const std::string & line, content) {
        buffers.push_back(boost::asio::buffer(line));
    }
    return buffers;
}

std::vector<boost::asio::const_buffer> Reply::HeaderstoBuffers(){
    std::vector<boost::asio::const_buffer> buffers;
    buffers.push_back(ToBuffer(status));
    for (std::size_t i = 0; i < headers.size(); ++i) {
        Header& h = headers[i];
        buffers.push_back(boost::asio::buffer(h.name));
        buffers.push_back(boost::asio::buffer(seperators));
        buffers.push_back(boost::asio::buffer(h.value));
        buffers.push_back(boost::asio::buffer(crlf));
    }
    buffers.push_back(boost::asio::buffer(crlf));
    return buffers;
}

Reply Reply::StockReply(Reply::status_type status, Reply::request_format req_format, std::string filename) {
    Reply rep;
    rep.status = status;
    rep.content.clear();
    rep.content.push_back( ToString(status) );
    rep.headers.resize(3);
    rep.headers[0].name = "Access-Control-Allow-Origin";
    rep.headers[0].value = "*";
    rep.headers[1].name = "Content-Length";

    std::string s;
    intToString(rep.content.size(), s);

    rep.headers[1].value = s;

    http::format_info req_format_info = Reply::GetFormatInfo(req_format);
    rep.headers[2].name  = "Content-Type";
    rep.headers[2].value = req_format_info.content_type;

    if ( req_format != Reply::html ) {
        rep.headers.resize(4);
        rep.headers[3].name = "Content-Disposition";
        rep.headers[3].value = "attachment; filename=\"" + filename + "." + req_format_info.file_extension + "\"";
    }
    
    return rep;
}

http::format_info Reply::GetFormatInfo(Reply::request_format format) {
    switch (format) {
    case Reply::html:
        return htmlFormat;
    case Reply::json:
        return jsonFormat;
    case Reply::jsonp:
        return jsonpFormat;
    case Reply::gpx:
        return gpxFormat;
    default:
        return htmlFormat;
    }
}

std::string Reply::ToString(Reply::status_type status) {
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

boost::asio::const_buffer Reply::ToBuffer(Reply::status_type status) {
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


Reply::Reply() : status(ok) {

}

}
