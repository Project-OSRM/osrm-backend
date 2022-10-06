
# Protozero Tutorial

## Getting to know Protocol Buffers

Protozero is a very low level library. You really have to know some of the
insides of Protocol Buffers to work with it!

So before reading any further in this document, read the following from the
Protocol Buffer documentation:

* [Developer Guide - Overview](https://developers.google.com/protocol-buffers/docs/overview)
* [Language Guide](https://developers.google.com/protocol-buffers/docs/proto)
* [Encoding](https://developers.google.com/protocol-buffers/docs/encoding)

Make sure you understand the basic types of values supported by Protocol
Buffers. Refer to this
[handy table](https://developers.google.com/protocol-buffers/docs/proto#scalar)
and [the cheat sheet](cheatsheet.md) if you are getting lost.


## Prerequisites

You need a C++11-capable compiler for Protozero to work. Copy the files in the
`include/protozero` directory somewhere where your build system can find them.
Keep the `protozero` directory and include the files in the form

```cpp
#include <protozero/FILENAME.hpp>
```


## Parsing protobuf-encoded messages

### Using `pbf_reader`

To use the `pbf_reader` class, add this include to your C++ program:

```cpp
#include <protozero/pbf_reader.hpp>
```

The `pbf_reader` class contains asserts that will detect some programming
errors. We encourage you to compile with asserts enabled in your debug builds.


### An introductory example

Lets say you have a protocol description in a `.proto` file like this:

```cpp
message Example1 {
    required uint32 x  =  1;
    optional string s  =  2;
    repeated fixed64 r = 17;
}
```

To read messages created according to that description, you will have code that
looks somewhat like this:

```cpp
#include <protozero/pbf_reader.hpp>

// get data from somewhere into the input string
std::string input = get_input_data();

// initialize pbf message with this data
protozero::pbf_reader message{input};

// iterate over fields in the message
while (message.next()) {

    // switch depending on the field tag (the field name is not available)
    switch (message.tag()) {
        case 1:
            // get data for tag 1 (in this case an uint32)
            auto x = message.get_uint32();
            break;
        case 2:
            // get data for tag 2 (in this case a string)
            std::string s = message.get_string();
            break;
        case 17:
            // ignore data for tag 17
            message.skip();
            break;
        default:
            // ignore data for unknown tags to allow for future extensions
            message.skip();
    }
}
```

You always have to call `next()` and then either one of the accessor functions
(like `get_uint32()` or `get_string()`) to get the field value or `skip()` to
ignore this field. Then call `next()` again, and so forth. Never call `next()`
twice in a row or any if the accessor or skip functions twice in a row.

Because the `pbf_reader` class doesn't know the `.proto` file it doesn't know
which field names or tags there are and it doesn't known the types of the
fields. You have to make sure to call the right `get_...()` function for each
tag. Some `assert()s` are done to check you are calling the right functions,
but not all errors can be detected.

Note that it doesn't matter whether a field is defined as `required`,
`optional`, or `repeated`. You always have to be prepared to get zero, one, or
more instances of a field and you always have to be prepared to get other
fields, too, unless you want your program to break if somebody adds a new
field.


### If you only need a single field

If, out of a protocol buffer message, you only need the value of a single
field, you can use the version of the `next()` function with a parameter:

```cpp
// same .proto file and initialization as above

// get all fields with tag 17, skip all others
while (message.next(17)) {
    auto r = message.get_fixed64();
    std::cout << r << "\n";
}
```

### Handling scalar fields

As you saw in the example, handling scalar field types is reasonably easy. You
just check the `.proto` file for the type of a field and call the corresponding
function called `get_` + _field type_.

For `string` and `bytes` types the internal handling is exactly the same, but
both `get_string()` and `get_bytes()` are provided to make the code
self-documenting. Both theses calls allocate and return a `std::string` which
can add some overhead. You can call the `get_view()` function instead which
returns a `data_view` containing a pointer into the data (access with `data()`)
and the length of the data (access with `size()`).


### Handling repeated packed fields

Fields that are marked as `[packed=true]` in the `.proto` file are handled
somewhat differently. `get_packed_...()` functions returning an iterator range
are used to access the data.

So, for example, if you have a protocol description in a `.proto` file like
this:

```cpp
message Example2 {
    repeated sint32 i = 1 [packed=true];
}
```

You can get to the data like this:

```cpp
protozero::pbf_reader message{input.data(), input.size()};

// set current field
message.next(1);

// get an iterator range
auto pi = message.get_packed_sint32();

// iterate to get to all values
for (auto it = pi.begin(); it != pi.end(); ++it) {
    std::cout << *it << '\n';
}
```

Or, with a range-based for-loop:

```cpp
for (auto value : pi) {
    std::cout << v << '\n';
}
```

So you are getting a pair of normal forward iterators wrapped in an iterator
range object. The iterators can be used with any STL algorithms etc.

Note that the previous only applies to repeated **packed** fields, normal
repeated fields are handled in the usual way for scalar fields.


### Handling embedded messages

Protocol Buffers can embed any message inside another message. To access an
embedded message use the `get_message()` function. So for this description:

```cpp
message Point {
    required double x = 1;
    required double y = 2;
}

message Example3 {
    repeated Point point = 10;
}
```

you can parse with this code:

```cpp
protozero::pbf_reader message{input};

while (message.next(10)) {
    protozero::pbf_reader point = message.get_message();
    double x, y;
    while (point.next()) {
        switch (point.tag()) {
            case 1:
                x = point.get_double();
                break;
            case 2:
                y = point.get_double();
                break;
            default:
                point.skip();
        }
    }
    std::cout << "x=" << x << " y=" << y << "\n";
}
```

### Handling enums

Enums are stored as varints and they can't be differentiated from them. Use
the `get_enum()` function to get the value of the enum, you have to translate
this into the symbolic name yourself. See the `enum` test case for an example.


### Asserts and exceptions in the Protozero library

Protozero uses `assert()` liberally to help you find bugs in your own code when
compiled in debug mode (ie with `NDEBUG` not set). If such an assert "fires",
this is a very strong indication that there is a bug in your code somewhere.

(Protozero will disable those asserts and "convert" them into exception in its
own test code. This is done to make sure the asserts actually work as intended.
Your test code will not need this!)

Exceptions, on the other hand, are thrown by Protozero if some kind of data
corruption was detected while it is trying to parse the data. This could also
be an indicator for a bug in the user code, but because it can happen if the
data was (intentionally or not intentionally) been messed with, it is reported
to the user code using exceptions.

Most of the functions on the writer side can throw a `std::bad_alloc`
exception if there is no space to grow a buffer. Other than that no exceptions
can occur on the writer side.

All exceptions thrown by the reader side derive from `protozero::exception`.

Note that all exceptions can also happen if you are expecting a data field of
a certain type in your code but the field actually has a different type. In
that case the `pbf_reader` class might interpret the bytes in the buffer in
the wrong way and anything can happen.

#### `end_of_buffer_exception`

This will be thrown whenever any of the functions "runs out of input data".
It means you either have an incomplete message in your input or some other
data corruption has taken place.

#### `unknown_pbf_wire_type_exception`

This will be thrown if an unsupported wire type is encountered. Either your
input data is corrupted or it was written with an unsupported version of a
Protocol Buffers implementation.

#### `varint_too_long_exception`

This exception indicates an illegal encoding of a varint. It means your input
data is corrupted in some way.

#### `invalid_tag_exception`

This exception is thrown when a tag has an invalid value. Tags must be
unsigned integers between 1 and 2^29-1. Tags between 19000 and 19999 are not
allowed. See
https://developers.google.com/protocol-buffers/docs/proto#assigning-tags

#### `invalid_length_exception`

This exception is thrown when a length field of a packed repeated field is
invalid. For fixed size types the length must be a multiple of the size of
the type.

### The `pbf_reader` class

The `pbf_reader` class behaves like a value type. Objects are reasonably small
(two pointers and two `uint32_t`, so 24 bytes on a 64bit system) and they can
be copied and moved around trivially.

`pbf_reader` objects can be constructed from a `std::string` or a `const char*`
and a length field (either supplied as separate arguments or as a `std::pair`).
In all cases objects of the `pbf_reader` class store a pointer into the input
data that was given to the constructor. You have to make sure this pointer
stays valid for the duration of the objects lifetime.

## Parsing protobuf-encoded messages using `pbf_message`

One problem in the code above are the "magic numbers" used as tags for the
different fields that you got from the `.proto` file. Instead of spreading
these magic numbers around your code you can define them once in an `enum
class` and then use the `pbf_message` template class instead of the
`pbf_reader` class.

Here is the first example again, this time using this new technique. So you
have the following in a `.proto` file:

```cpp
message Example1 {
    required uint32 x  =  1;
    optional string s  =  2;
    repeated fixed64 r = 17;
}
```

Add the following declaration in one of your header files:

```cpp
enum class Example1 : protozero::pbf_tag_type {
    required_uint32_x  =  1,
    optional_string_s  =  2,
    repeated_fixed64_r = 17
};
```

The message name becomes the name of the `enum class` which is always built
on top of the `protozero::pbf_tag_type` type. Each field in the message
becomes one value of the enum. In this case the name is created from the
type (including the modifiers like `required` or `optional`) and the name of
the field. You can use any name you want, but this convention makes it easier
later, to get everything right.

To read messages created according to that description, you will have code that
looks somewhat like this, this time using `pbf_message` instead of
`pbf_reader`:

```cpp
#include <protozero/pbf_message.hpp>

// get data from somewhere into the input string
std::string input = get_input_data();

// initialize pbf message with this data
protozero::pbf_message<Example1> message{input};

// iterate over fields in the message
while (message.next()) {

    // switch depending on the field tag (the field name is not available)
    switch (message.tag()) {
        case Example1::required_uint32_x:
            auto x = message.get_uint32();
            break;
        case Example1::optional_string_s:
            std::string s = message.get_string();
            break;
        case Example1::repeated_fixed64_r:
            message.skip();
            break;
        default:
            // ignore data for unknown tags to allow for future extensions
            message.skip();
    }
}
```

Note the correspondance between the enum value (for instance
`required_uint32_x`) and the name of the getter function (for instance
`get_uint32()`). This makes it easier to get the correct types. Also the
naming makes it easier to keep different message types apart if you have
multiple (or embedded) messages.

See the `test/t/complex` test case for a complete example using this interface.

Using `pbf_message` in favour of `pbf_reader` is recommended for all code.
Note that `pbf_message` derives from `pbf_reader`, so you can always fall
back to the more generic interface if necessary.

One problem you might run into is the following: The enum class lists all
possible values you know about and you'll have lots of `switch` statements
checking those values. Some compilers will know that your `switch` covers
all possible cases and warn you if you have a `default` case that looks
unneccessary to the compiler. But you still want that `default` case to allow
for future extension of those messages (and maybe also to detect corrupted
data). You can switch of this warning with `-Wno-covered-switch-default`).


## Writing protobuf-encoded messages

### Using `pbf_writer`

To use the `pbf_writer` class, add this include to your C++ program:

```cpp
#include <protozero/pbf_writer.hpp>
```

The `pbf_writer` class contains asserts that will detect some programming
errors. We encourage you to compile with asserts enabled in your debug builds.


### An introductory example

Lets say you have a protocol description in a `.proto` file like this:

```cpp
message Example {
    required uint32 x = 1;
    optional string s = 2;
    repeated fixed64 r = 17;
}
```

To write messages created according to that description, you will have code
that looks somewhat like this:

```cpp
#include <protozero/pbf_writer.hpp>

std::string data;
protozero::pbf_writer pbf_example{data};

pbf_example.add_uint32(1, 27);       // uint32_t x
pbf_example.add_fixed64(17, 1);      // fixed64 r
pbf_example.add_fixed64(17, 2);
pbf_example.add_fixed64(17, 3);
pbf_example.add_string(2, "foobar"); // string s
```

First you need a string which will be used as buffer to assemble the
protobuf-formatted message. The `pbf_writer` object contains a reference to
this string buffer and through it you add data to that buffer piece by piece.
The buffer doesn't have to be empty, the `pbf_writer` will simply append its
data to whatever is there already.


### Handling scalar fields

As you could see in the introductory example handling any kind of scalar field
is easy. The type of field doesn't matter and it doesn't matter whether it is
optional, required or repeated. You always call one of the `add_TYPE()` method
on the pbf writer object.

The first parameter of these methods is always the *tag* of the field (the
field number) from the `.proto` file. The second parameter is the value you
want to set. For the `bytes` and `string` types several versions of the add
method are available taking a `const std::string&` or a `const char*` and a
length.

For `enum` types you have to use the numeric value as the symbolic names from
the `.proto` file are not available.


### Handling repeated packed fields

Repeated packed fields can easily be set from a pair of iterators:

```cpp
std::string data;
protozero::pbf_writer pw{data};

std::vector<int> v = { 1, 4, 9, 16, 25, 36 };
pw.add_packed_int32(1, std::begin(v), std::end(v));
```

If you don't have an iterator you can use the alternative form:

```cpp
std::string data;
protozero::pbf_writer pw{data};
{
    protozero::packed_field_int32 field{pw, 1};
    field.add_element(1);
    field.add_element(10);
    field.add_element(100);
}
```

Of course you can add as many elements as you want. If you add no elements
at all, this code will still work, Protozero detects this special case and
pretends you never even initialized this field.

The nested scope is important in this case, because the destructor of the
`field` object will make sure the length stored inside the field is set to
the right value. You must close that scope before adding other fields to the
`pw` pbf writer.

If you know how many elements you will add to the field and your field contains
fixed length elements, you can tell Protozero and it can optimize this case:

```cpp
std::string data;
protozero::pbf_writer pw{data};
{
    protozero::packed_field_fixed32 field{pw, 1, 2}; // exactly two elements
    field.add_element(42);
    field.add_element(13);
}
```

In this case you have to supply exactly as many elements as you promised,
otherwise you will get a broken protobuf message.

This works for `packed_field_fixed32`, `packed_field_sfixed32`,
`packed_field_fixed64`, `packed_field_sfixed64`, `packed_field_float`, and
`packed_field_double`.

You can abandon writing of the packed field if this becomes necessary by
calling `rollback()`:

```cpp
std::string data;
protozero::pbf_writer pw{data};
{
    protozero::packed_field_int32 field{pw, 1};
    field.add_element(42);
    // some error occurs, you don't want to have this field at all
    field.rollback();
}
```

The result is the same as if the lines inside the nested brackets had never
been called. Do not try to call `add_element()` after a rollback.


### Handling sub-messages

Nested sub-messages can be handled by first creating the submessage and then
adding to the parent message:

```cpp
std::string buffer_sub;
protozero::pbf_writer pbf_sub{buffer_sub};

// add fields to sub-message
pbf_sub.add_...(...);
// ...

// sub-message is finished here

std::string buffer_parent;
protozero::pbf_writer pbf_parent{buffer_parent};
pbf_parent.add_message(1, buffer_sub);
```

This is easy to do but it has the drawback of needing a separate `std::string`
buffer. If this concerns you (and why would you use protozero and not the
Google protobuf library if it doesn't?) there is another way:

```cpp
std::string data;
protozero::pbf_writer pbf_parent{data};

// optionally add fields to parent here
pbf_parent.add_...(...);

// open a new scope
{
    // create new pbf_writer with parent and the tag (field number)
    // as parameters
    protozero::pbf_writer pbf_sub{pbf_parent, 1};

    // add fields to sub here...
    pbf_sub.add_...(...);

} // closing the scope will close the sub-message

// optionally add more fields to parent here
pbf_parent.add_...(...);
```

This can be nested arbitrarily deep.

Internally the sub-message writer re-uses the buffer from the parent. It
reserves enough space in the buffer to later write the length of the submessage
into it. It then adds the contents of the submessage to the buffer. When the
`pbf_sub` writer is destructed the length of the submessage is calculated and
written in the reserved space. If less space was needed for the length field
than was available, the rest of the buffer is moved over a few bytes.

You can abandon writing of submessage if this becomes necessary by
calling `rollback()`:

```cpp
std::string data;
protozero::pbf_writer pbf_parent{data};

// open a new scope
{
    // create new pbf_writer with parent and the tag (field number)
    // as parameters
    protozero::pbf_writer pbf_sub{pbf_parent, 1};

    // add fields to sub here...
    pbf_sub.add_...(...);

    // some problem occurs and you want to abandon the submessage:
    pbf_sub.rollback();
}

// optionally add more fields to parent here
pbf_parent.add_...(...);
```

The result is the same as if the lines inside the nested brackets had never
been called. Do not try to call any of the `add_*` functions on the submessage
after a rollback.

## Writing protobuf-encoded messages using `pbf_builder`

Just like the `pbf_message` template class wraps the `pbf_reader` class, there
is a `pbf_builder` template class wrapping the `pbf_writer` class. It is
instantiated using the same `enum class` described above and used exactly
like the `pbf_writer` class but using the values of the enum instead of bare
integers.

See the `test/t/complex` test case for a complete example using this interface.

