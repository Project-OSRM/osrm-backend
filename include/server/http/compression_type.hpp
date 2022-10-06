#ifndef COMPRESSION_TYPE_HPP
#define COMPRESSION_TYPE_HPP

namespace osrm
{
namespace server
{
namespace http
{

enum compression_type
{
    no_compression,
    gzip_rfc1952,
    deflate_rfc1951
};
}
} // namespace server
} // namespace osrm

#endif // COMPRESSION_TYPE_HPP
