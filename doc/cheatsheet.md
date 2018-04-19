
# Protozero Cheat Sheet

See also this
[handy table](https://developers.google.com/protocol-buffers/docs/proto#scalar)
from the Google Protocol Buffers documentation.

## Scalar types

| PBF Type | Underlying Storage | C++ Type      | Getter           | Notes |
| -------- | ------------------ | ------------- | ---------------- | ----- |
| int32    | varint             | `int32_t`     | `get_int32()`    |       |
| sint32   | varint (zigzag)    | `int32_t`     | `get_sint32()`   |       |
| uint32   | varint             | `uint32_t`    | `get_uint32()`   |       |
| int64    | varint             | `int64_t`     | `get_int64()`    |       |
| sint64   | varint (zigzag)    | `int64_t`     | `get_sint64()`   |       |
| uint64   | varint             | `uint64_t`    | `get_uint64()`   |       |
| bool     | varint             | `bool`        | `get_bool()`     |       |
| enum     | varint             | `int32_t`     | `get_enum()`     |       |
| fixed32  | 32bit fixed        | `uint32_t`    | `get_fixed32()`  |       |
| sfixed32 | 32bit fixed        | `int32_t`     | `get_sfixed32()` |       |
| fixed64  | 64bit fixed        | `uint64_t`    | `get_fixed64()`  |       |
| sfixed64 | 64bit fixed        | `int64_t`     | `get_sfixed64()` |       |
| float    | 32bit fixed        | `float`       | `get_float()`    |       |
| double   | 64bit fixed        | `double`      | `get_double()`   |       |
| string   | length-delimited   | `data_view`   | `get_view()`     | (1)   |
| string   | length-delimited   | pair          | `get_data()`     | (2)   |
| string   | length-delimited   | `std::string` | `get_string()`   |       |
| bytes    | length-delimited   | `data_view`   | `get_view()`     | (1)   |
| bytes    | length-delimited   | pair          | `get_data()`     | (2)   |
| bytes    | length-delimited   | `std::string` | `get_bytes()`    |       |
| message  | length-delimited   | `data_view`   | `get_view()`     | (1)   |
| message  | length-delimited   | pair          | `get_data()`     | (2)   |
| message  | length-delimited   | `pbf_reader`  | `get_message()`  |       |

### Notes:

* (1) preferred form, returns `protozero::data_view` which is convertible to
  `std::string` if needed.
* (2) deprecated form, returns `std::pair<const char*, pbf_length_type>`,
  use `get_view()` instead. This form is only available if
  `PROTOZERO_STRICT_API` is not defined.
* The setter function of `pbf_writer` is always `add_` + the PBF type. Several
  overloads are available.


## Packed repeated fields

| PBF Type | Getter                  |
| -------- | ----------------------- |
| int32    | `get_packed_int32()`    |
| sint32   | `get_packed_sint32()`   |
| uint32   | `get_packed_uint32()`   |
| int64    | `get_packed_int64()`    |
| sint64   | `get_packed_sint64()`   |
| uint64   | `get_packed_uint64()`   |
| bool     | `get_packed_bool()`     |
| enum     | `get_packed_enum()`     |
| fixed32  | `get_packed_fixed32()`  |
| sfixed32 | `get_packed_sfixed32()` |
| fixed64  | `get_packed_fixed64()`  |
| sfixed64 | `get_packed_sfixed64()` |
| float    | `get_packed_float()`    |
| double   | `get_packed_double()`   |

Packed repeated fields for string, bytes, and message types are not possible.

