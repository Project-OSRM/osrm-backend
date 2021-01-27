
# Protozero Advanced Topics

This documentation contains some mixed advanced topics for Protozero users.
Read the [tutorial](tutorial.md) first if you are new to Protozero.


## Limitations of Protozero

* A protobuf message has to fit into memory completely, otherwise it can not
  be parsed with this library. There is no streaming support.
* The length of a string, bytes, or submessage can't be more than 2^31-1.
* There is no specific support for maps but they can be used as described in
  the "Backwards compatibility" section of
  https://developers.google.com/protocol-buffers/docs/proto3#maps.


## Checking the Protozero version number

If `protozero/version.hpp` is included, the following macros are set:

| Macro                      | Example | Description                                    |
| -------------------------- | ------- | ---------------------------------------------- |
| `PROTOZERO_VERSION_MAJOR`  | 1       | Major version number                           |
| `PROTOZERO_VERSION_MINOR`  | 3       | Minor version number                           |
| `PROTOZERO_VERSION_PATCH`  | 2       | Patch number                                   |
| `PROTOZERO_VERSION_CODE`   | 10302   | Version (major * 10,000 + minor * 100 + patch) |
| `PROTOZERO_VERSION_STRING` | "1.3.2" | Version string                                 |


## Changing Protozero behaviour with macros

The behaviour of Protozero can be changed by defining the following macros.
They have to be set before including any of the Protozero headers.

### `PROTOZERO_STRICT_API`

If this is set, you will get some extra warnings or errors during compilation
if you are using an old (deprecated) interface to Protozero. Enable this if
you want to make sure your code will work with future versions of Protozero.

### `PROTOZERO_USE_VIEW`

Protozero uses the class `protozero::data_view` as the return type of the
`pbf_reader::get_view()` method and a few other functions take a
`protozero::data_view` as parameter.

If `PROTOZERO_USE_VIEW` is unset, `protozero::data_view` is Protozero's own
implementation of a *string view* class.

Set this macro if you want to use a different implementation such as the C++17
`std::string_view` class. In this case `protozero::data_view` will simply be
an alias to the class you specify.

    #define PROTOZERO_USE_VIEW std::string_view


## Repeated fields in messages

The Google Protobuf spec documents that a non-repeated field can actually
appear several times in a message and the implementation is required to return
the value of the last version of that field in this case. `pbf_reader.hpp` does
not enforce this. If this feature is needed in your case, you have to do this
yourself.

The [spec also
says](https://developers.google.com/protocol-buffers/docs/encoding#packed)
that you must be able to read a packed repeated field where a not-packed
repeated field is expected and vice versa. Also there can be several (packed or
not-packed) repeated fields with the same tag and their contents must be
concatenated. It is your responsibility to do this, Protozero doesn't do that
for you.

### Using `tag_and_type()`

The `tag_and_type()` free function and the method of the same name on the
`pbf_reader` and `pbf_message` classes can be used to access both packed and
unpacked repeated fields. (It can also be used to check that you have the
right type of encoding for other fields.)

Here is the outline:

```cpp
enum class ExampleMsg : protozero::pbf_tag_type {
    repeated_uint32_x = 1
};

std::string data = ...
pbf_message<ExampleMsg> message{data};
while (message.next()) {
    switch (message.tag_and_type()) {
        case tag_and_type(ExampleMsg::repeated_uint32_x, pbf_wire_type::length_delimited): {
                auto xit = message.get_packed_uint32();
                ... // handle the repeated field when it is packed
            }
            break;
        case tag_and_type(ExampleMsg::repeated_uint32_x, pbf_wire_type::varint): {
                auto x = message.get_uint32();
                ... // handle the repeated field when it is not packed
            }
            break;
        default:
            message.skip();
    }
}
```

All this works on `pbf_reader` in the same way as with `pbf_message` with the
usual difference that `pbf_reader` takes a numeric field tag and `pbf_message`
an enum field.

If you only want to check for one specific tag and type you can use the
two-argument version of `pbf_reader::next()`. In this case `17` is the field
tag we are looking for:

```cpp
std::string data = ...
pbf_reader message{data};
while (message.next(17, pbf_wire_type::varint)) {
    auto foo = message.get_int32();
    ...
}
```

See the test under `test/t/tag_and_type/` for a complete example.


## Reserving memory when writing messages

If you know beforehand how large a message will become or can take an educated
guess, you can call the usual `std::string::reserve()` on the underlying string
before you give it to an `pbf_writer` or `pbf_builder` object.

Or you can (at any time) call `reserve()` on the `pbf_writer` or `pbf_builder`.
This will reserve the given amount of bytes *in addition to whatever is already
in that message*. (Note that this behaviour is different then what `reserve()`
does on `std::string` or `std::vector`.)

In the general case it is not easy to figure out how much memory you will need
because of the varint packing of integers. But sometimes you can make at least
a rough estimate. Still, you should probably only use this facility if you have
benchmarks proving that it actually makes your program faster.


## Using the low-level varint and zigzag encoding and decoding functions

Protozero gives you access to the low-level functions for encoding and
decoding varint and zigzag integer encodings, because these functions can
sometimes be useful outside the Protocol Buffer context.

### Using low-level functions

To use the low-level functions, add this include to your C++ program:

```cpp
#include <protozero/varint.hpp>
```

### Functions

The following functions are then available:

```cpp
decode_varint()
write_varint()
encode_zigzag32()
encode_zigzag64()
decode_zigzag32()
decode_zigzag64()
```

See the reference documentation created by `make doc` for details.


## Vectored input for length-delimited fields

Length-delimited fields (like string fields, byte fields and messages) are
usually set by calling `add_string()`, `add_message()`, etc. These functions
have several forms, but they basically all take a *tag*, a *size*, and a
*pointer to the data*. They write the length of the data into the message
and then copy the data over.

Sometimes you have the data not in one place, but spread over several
buffers. In this case you have to consolidate those buffers first, which needs
an extra copy. Say you have two very long strings that should be concatenated
into a message:

```cpp
std::string a{"very long string..."};
std::string b{"another very long string..."};

std::string data;
protozero::pbf_writer writer{data};

a.append(b); // expensive extra copy

writer.add_string(1, a);
```

To avoid this, the function `add_bytes_vectored()` can be used which allows
vectored (or scatter/gather) input like this:

```cpp
std::string a{"very long string..."};
std::string b{"another very long string..."};

std::string data;
protozero::pbf_writer writer{data};

writer.add_bytes_vectored(1, a, b);
```

`add_bytes_vectored()` will add up the sizes of all its arguments and copy over
all the data only once.

The function takes any number of arguments. The arguments must be of a type
supporting the `data()` and `size()` methods like `protozero::data_view()`,
`std::string` or the C++17 `std::string_view`.

Note that there is only one version of the function which can be used for any
length-delimited field including strings, bytes, messages and repeated packed
fields.

The function is also available in the `pbf_builder` class.


## Internal handling of varints

When varints are decoded they are always decoded as 64bit unsigned integers and
after that casted to the type you are requesting (using `static_cast`). This
means that if the protocol buffer message was created with a different integer
type than what you are reading it with, you might get wrong results without any
warning or error. This is the same behaviour as the Google Protocol Buffers
library has.

In normal use, this should never matter, because presumably you are using the
same types to write that data as you are using to read it later. It can happen
if the data is corrupted intentionally or unintentionally in some way. But
this can't be used to feed you any data that it wasn't possible to feed you
without this behaviour, so it doesn't open up any potential problems. You
always have to check anyway that the integers are in the range you expected
them to be in if the expected range is different than the range of the integer
type. This is especially true for enums which protozero will return as
`int32_t`.


## How many items are there in a repeated packed field?

Sometimes it is useful to know how many values there are in a repeated packed
field. For instance when you want to reserve space in a `std::vector`.

```cpp
protozero::pbf_reader message{...};
message.next(...);
const auto range = message.get_packed_sint32();

std::vector<int> myvalues;
myvalues.reserve(range.size());

for (auto value : range) {
    myvalues.push_back(value);
}
```

It depends on the type of range how expensive the `size()` call is. For ranges
derived from packed repeated fixed sized values the effort will be constant,
for ranges derived from packed repeated varints, the effort will be linear, but
still considerably cheaper than decoding the varints. You have to benchmark
your use case to see whether the `reserve()` (or whatever you are using the
`size()` for) is worth it.


## Using a different buffer class than std::string

Normally you are using the `pbf_writer` or `pbf_builder` classes which use a
`std::string` that you supply as their buffer for building the actual protocol
buffers message into. But you can use a different buffer implementation
instead. This might be useful if you want to use a fixed-size buffer for
instance.

The `pbf_writer` and `pbf_builder` classes are actually only aliases for the
`basic_pbf_writer` and `basic_pbf_builder` template classes:

```cpp
using pbf_writer = basic_pbf_writer<std::string>;

template <typename T>
using pbf_builder = basic_pbf_builder<std::string, T>;
```

If you want to use a different buffer type, use the `basic_*` form of the
class and use the buffer class as template parameter. When instantiating the
`basic_pbf_writer` or `basic_pbf_builder`, the only parameter to the
constructor must always be a reference to an object of the buffer class.

```cpp
some_buffer_class buffer;
basic_pbf_writer<some_buffer_class> writer{buffer};
```

For this to work you must supply template specializations for some static
functions in the `protozero::buffer_customization` struct, see
`buffer_tmpl.hpp` for details.

Protozero already supports two buffer types:
* `std::string` (to use include `protozero/buffer_string.hpp`)
* `std::vector<char>` (to use include `protozero/buffer_vector.hpp`)

There is a class `protozero::fixed_size_buffer_adaptor` you can use as adaptor
for any fixed-sized buffer you might have. Include `protozero/buffer_fixed.hpp`
to use it:

```cpp
#include <protozero/buffer_fixed.hpp>

your_buffer_class some_buffer;
protozero::fixed_size_buffer_adaptor buffer_adaptor{some_buffer.data(), some_buffer.size()};
basic_pbf_writer<protozero::fixed_size_buffer_adaptor> writer{buffer_adaptor};
```

The buffer adaptor can be initialized with any container if it supports the
`data()` and `size()` member functions:

```cpp
protozero::fixed_size_buffer_adaptor buffer_adaptor{some_buffer};
```
