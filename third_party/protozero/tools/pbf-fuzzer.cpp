#include <protozero/pbf_reader.hpp>

// From Google Benchmark library.
// See https://github.com/google/benchmark/blob/main/LICENSE
template <class Tp>
inline __attribute__((always_inline)) void DoNotOptimize(Tp const& value) {
    asm volatile("" : : "r,m"(value) : "memory");
}

template <typename T>
int read_packed(protozero::iterator_range<T> range) {
    try {
        for (const auto& item : range) {
            DoNotOptimize(item);
        }
    } catch (const protozero::exception&) {
        // no-op. This is probably not a packed field of that type.
    }
    return 0;
}

template <typename Fn>
void try_field(const protozero::pbf_reader& reader, Fn&& fn) {
    try {
        DoNotOptimize(fn(reader));
    } catch (const protozero::exception&) {
        // no-op. This is probably not a field of that type.
    }
}

int try_message(protozero::pbf_reader reader) {
    try {
        while (reader.next()) {
            if (reader.has_wire_type(protozero::pbf_wire_type::varint)) {
                // Try to decode this field as any of the types that can be encoded as varint.
                try_field(reader, [](protozero::pbf_reader r) { return r.get_bool(); });
                try_field(reader, [](protozero::pbf_reader r) { return r.get_enum(); });
                try_field(reader, [](protozero::pbf_reader r) { return r.get_int32(); });
                try_field(reader, [](protozero::pbf_reader r) { return r.get_sint32(); });
                try_field(reader, [](protozero::pbf_reader r) { return r.get_uint32(); });
                try_field(reader, [](protozero::pbf_reader r) { return r.get_int64(); });
                try_field(reader, [](protozero::pbf_reader r) { return r.get_sint64(); });
                try_field(reader, [](protozero::pbf_reader r) { return r.get_uint64(); });
            } else if (reader.has_wire_type(protozero::pbf_wire_type::length_delimited)) {
                // Try to decode this field as any of the types that can be encoded as length-delimited.
                try_field(reader, [](protozero::pbf_reader r) { return try_message(r.get_message()); });
                try_field(reader, [](protozero::pbf_reader r) { return read_packed(r.get_packed_bool()); });
                try_field(reader, [](protozero::pbf_reader r) { return read_packed(r.get_packed_double()); });
                try_field(reader, [](protozero::pbf_reader r) { return read_packed(r.get_packed_enum()); });
                try_field(reader, [](protozero::pbf_reader r) { return read_packed(r.get_packed_fixed32()); });
                try_field(reader, [](protozero::pbf_reader r) { return read_packed(r.get_packed_fixed64()); });
                try_field(reader, [](protozero::pbf_reader r) { return read_packed(r.get_packed_float()); });
                try_field(reader, [](protozero::pbf_reader r) { return read_packed(r.get_packed_int32()); });
                try_field(reader, [](protozero::pbf_reader r) { return read_packed(r.get_packed_int64()); });
                try_field(reader, [](protozero::pbf_reader r) { return read_packed(r.get_packed_sfixed32()); });
                try_field(reader, [](protozero::pbf_reader r) { return read_packed(r.get_packed_sfixed64()); });
                try_field(reader, [](protozero::pbf_reader r) { return read_packed(r.get_packed_sint32()); });
                try_field(reader, [](protozero::pbf_reader r) { return read_packed(r.get_packed_sint64()); });
                try_field(reader, [](protozero::pbf_reader r) { return read_packed(r.get_packed_uint32()); });
                try_field(reader, [](protozero::pbf_reader r) { return read_packed(r.get_packed_uint64()); });
            } else if (reader.has_wire_type(protozero::pbf_wire_type::fixed64)) {
                // Try to decode this field as any of the types that can be encoded as fixed64.
                try_field(reader, [](protozero::pbf_reader r) { return r.get_double(); });
                try_field(reader, [](protozero::pbf_reader r) { return r.get_fixed64(); });
                try_field(reader, [](protozero::pbf_reader r) { return r.get_sfixed64(); });
            } else if (reader.has_wire_type(protozero::pbf_wire_type::fixed32)) {
                // Try to decode this field as any of the types that can be encoded as fixed32.
                try_field(reader, [](protozero::pbf_reader r) { return r.get_float(); });
                try_field(reader, [](protozero::pbf_reader r) { return r.get_fixed32(); });
                try_field(reader, [](protozero::pbf_reader r) { return r.get_sfixed32(); });
            }

            reader.skip();
        }
    } catch (const protozero::exception&) {
        // no-op. This is probably not a valid message.
    }

    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    try_message(protozero::pbf_reader{reinterpret_cast<const char*>(data), size});
    return 0;
}
