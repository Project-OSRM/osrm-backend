#ifndef OSRM_WRITER_HPP
#define OSRM_WRITER_HPP

#include "../data_structures/import_edge.hpp"
#include "../data_structures/external_memory_node.hpp"

#include "../util/fingerprint.hpp"

#include <ostream>
#include <cstddef>
#include <type_traits>

// The OSRMWriter is fully customizable by providing policies for:
//
//  - a header, written once at the beginning
//  - writing each item or no item at all
//  - a finalizer, run once after writing is done
//
//  As of now the policies as stateless; this may change though
//  in order to support e.g. profiling policies that hold timers.

template <typename HeaderWritePolicy, typename TypeWritePolicy, typename FinalizeWritePolicy>
class OSRMWriter final
{
  public:
    template <typename Header>
    OSRMWriter(std::ostream &stream, const Header &header)
        : stream_{stream}, segment_start_(stream_.tellp()), count_{0}
    {
        header_offset_ = HeaderWritePolicy::Write(header, stream_, segment_start_, count_);

        unsigned reserve_prefix = 0;
        const auto written =
            TypeWritePolicy::Write(reserve_prefix, stream_, segment_start_, header_offset_, count_);
        (void)written; // unused, as this is reserved space
    }

    ~OSRMWriter()
    {
        const auto len = FinalizeWritePolicy::Write(stream_, segment_start_, header_offset_, count_);
        (void)len; // unused
    }

    template <typename T> void Write(const T &item)
    {
        const auto written =
            TypeWritePolicy::Write(item, stream_, segment_start_, header_offset_, count_);
        count_ += written;
    }

    std::size_t Count() const { return count_; }

  private:
    std::ostream &stream_;
    std::size_t segment_start_;
    std::size_t header_offset_; // should be compile time constant
    std::size_t count_;
};

// Silent Policies
struct NoHeaderWritePolicy final
{
    template <typename T>
    static std::size_t Write(const T &, std::ostream &, std::size_t, std::size_t)
    {
        return 0;
    }
};

struct NoTypeWritePolicy final
{
    template <typename T>
    static std::size_t Write(const T &, std::ostream &, std::size_t, std::size_t, std::size_t)
    {
        return 0;
    }
};

struct NoFinalizeWritePolicy final
{
    static std::size_t Write(std::ostream &, std::size_t, std::size_t, std::size_t) { return 0; }
};

// TODO: Debug Policies, i.e. diagnostics to stderr

// Concrete Policies
struct TrivialHeaderWritePolicy final
{
    template <typename T>
    static std::size_t
    Write(const T &header, std::ostream &stream, std::size_t segment_start, std::size_t count)
    {
        // TODO: strictly speaking we need a trivial type, but most are not; check this on callside
        // static_assert(std::is_trivial<T>::value, "T is not a trivial type");
        const auto offset = sizeof(T);
        stream.write(reinterpret_cast<const char *>(&header), offset);
        return offset;
    }
};

struct TrivialTypeWritePolicy final
{
    template <typename T>
    static std::size_t Write(const T &item,
                             std::ostream &stream,
                             std::size_t segment_start,
                             std::size_t header_off,
                             std::size_t count)
    {
        // TODO: strictly speaking we need a trivial type, but most are not; check this on callside
        // static_assert(std::is_trivial<T>::value, "T is not a trivial type");
        stream.write(reinterpret_cast<const char *>(&item), sizeof(decltype(item)));
        return 1u;
    }
};

struct LengthPrefixFinalizeWritePolicy final
{
    static std::size_t Write(std::ostream &stream,
                             std::size_t segment_start,
                             std::size_t header_offset,
                             std::size_t count)
    {
        const auto here = stream.tellp();
        stream.seekp(segment_start + header_offset);
        // XXX: why do we write unsigned; what about overflow?
        unsigned len = static_cast<unsigned>(count);
        stream.write(reinterpret_cast<const char *>(&len), sizeof(decltype(len)));
        stream.seekp(here);
        return 1u;
    }
};

using HeaderWriter = OSRMWriter<TrivialHeaderWritePolicy, NoTypeWritePolicy, NoFinalizeWritePolicy>;
using EdgeWriter = OSRMWriter<NoHeaderWritePolicy, TrivialTypeWritePolicy, LengthPrefixFinalizeWritePolicy>;
using NodeWriter = OSRMWriter<NoHeaderWritePolicy, TrivialTypeWritePolicy, LengthPrefixFinalizeWritePolicy>;

#endif
