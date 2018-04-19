
# Writing vector tiles

Writing vector tiles start with creating a `tile_builder`. This builder will
then be used to add layers and features in those layers. Once all this is done,
you call `serialize()` to actually build the vector tile from the data you
provided to the builders:

```cpp
#include <vtzero/builder.hpp> // always needed when writing vector tiles

vtzero::tile_builder tbuilder;
// add lots of data to builder...
std::string buffer = tbuilder.serialize();
```

You can also serialize the data into an existing buffer instead:

```cpp
std::string buffer; // got buffer from somewhere
tbuilder.serialize(buffer);
```

## Adding layers to tiles

Once you have a tile builder, you'll first need some layers:

```cpp
vtzero::tile_builder tbuilder;
vtzero::layer_builder layer_pois{tbuilder, "pois", 2, 4096};
vtzero::layer_builder layer_roads{tbuilder, "roads"};
vtzero::layer_builder layer_forests{tbuilder, "forests"};
```

Here three layers called "pois", "roads", and "forests" are added. The first
one explicitly specifies the vector tile version used and the extent. The
values specified here are the default, so all layers in this example will have
a version of 2 and an extent of 4096.

If you have read a layer from an existing vector tile and want to copy over
some of the data, you can use this layer to initialize the new layer in the
new vector tile with the name, version and extent from the existing layer like
this:

```cpp
vtzero::layer some_layer = ...;
vtzero::layer_builder layer_pois{tbuilder, some_layer};
// same as...
vtzero::layer_builder layer_pois{tbuilder, some_layer.name(),
                                           some_layer.version(),
                                           some_layer.extent()};
```

If you want to copy over an existing layer completely, you can use the
`add_existing_layer()` function instead:

```cpp
vtzero::layer some_layer = ...;
vtzero::tile_builder tbuilder;
tbuilder.add_existing_layer(some_layer);
```

Or, if you have the encoded layer data available in a `data_view` this also
works:

```cpp
vtzero::data_view layer_data = ...;
vtzero::tile_builder tbuilder;
tbuilder.add_existing_layer(layer_data);
```

Note that this call will only store a reference to the data to be added in the
tile builder. The data will only be copied when the final `serialize()` is
called, so the input data must still be available then!

You can mix any of the ways of adding a layer to the tile mentioned above. The
layers will be added to the tile in the order you add them to the
`tile_builder`.

The tile builder is smart enough to not add empty layers, so you can start
out with all the layers you might need and if some of them stay empty, they
will not be added to the tile when `serialize()` is called.

## Adding features to layers

Once we have one or more `layer_builder`s instantiated, we can add features
to them. This is done through the following feature builder classes:

* `point_feature_builder` to add a feature with a (multi)point geometry,
* `linestring_feature_builder` to add a feature with a (multi)linestring
  geometry,
* `polygon_feature_builder` to add a feature with a (multi)polygon geometry, or
* `geometry_feature_builder` to add a feature with an existing geometry you
  got from reading a vector tile.

In all cases you need to instantiate the feature builder class, optionally
add the feature ID using the `set_id()` method, add the geometry and then
add all the properties of this feature. You have to keep to this order!

```cpp
...
vtzero::layer_builder lbuilder{...};
{
    vtzero::point_feature_builder fbuilder{lbuilder};
    // optionally set the ID
    fbuilder.set_id(23);
    // add the geometry (exact calls are different for different feature builders)
    fbuilder.add_point(99, 33);
    // add the properties
    fbuilder.add_property("amenity", "restaurant");
    // call commit() when you are done
    fbuilder.commit()
}
```

You have to call `commit()` on the feature builder object after you set all the
data to actually add it to the layer. If you don't do this, the feature will
not be added to the layer! This can be useful, for instance, if you detect that
you have an invalid geometry while you are adding the geometry to the feature
builder. In that case you can call `rollback()` explicitly or just let the
feature builder go out of scope and it will do the rollback automatically.

Only the first call to `commit()` or `rollback()` will take effect, any further
calls to these functions on the same feature builder object are ignored.

## Adding a geometry to the feature

There are different ways of adding the geometry to the feature, depending on
the geometry type.

### Adding a point geometry

Simply call `add_point()` to set the point geometry. There are three different
overloads for this function. One takes a `vtzero::point`, one takes two
`uint32_t`s with the x and y coordinates and one takes any type `T` that can
be converted to a `vtzero::point` using the `create_vtzero_point` function.
This templated function works on any type that has `x` and `y` members and
you can create your own overload of this function. See the
[advanced.md](advanced topics documentation).

### Adding a multipoint geometry

Call `add_points()` with the number of points in the geometry as only argument.
After that call `set_point()` for each of those points. `set_point()` has
multiple overloads just like the `add_point()` method described above.

There is also the `add_points_from_container()` function which copies the
point from any container type supporting the `size()` function and which
iterator yields a `vtzero::point` or something convertible to it.

### Adding a linestring geometry

Call `add_linestring()` with the number of points in the linestring as only
argument. After that call `set_point()` for each of those points. `set_point()`
has multiple overloads just like the `add_point()` method described above.

```cpp
...
vtzero::layer_builder lbuilder{...};
try {
    vtzero::linestring_feature_builder fbuilder{lbuilder};
    // optionally set the ID
    fbuilder.set_id(23);
    // add the geometry
    fbuilder.add_linestring(2);
    fbuilder.set_point(1, 2);
    fbuilder.set_point(3, 4);
    // add the properties
    fbuilder.add_property("highway", "primary");
    fbuilder.add_property("maxspeed", 80);
    // call commit() when you are done
    fbuilder.commit()
} catch (const vtzero::geometry_exception& e) {
    // if we are here, something was wrong with the geometry.
}
```

Note that we have wrapped the feature builder in a try-catch-block here. This
will ignore all geometry errors (which can happen if two consective points
are the same creating a zero-length segment).

There are two other versions of the `add_linestring()` function. They take two
iterators defining a range to get the points from. Dereferencing those
iterators must yield a `vtzero::point` or something convertible to it. One of
these functions takes a third argument, the number of points the iterator will
yield. If this is not available `std::distance(begin, end)` is called which
internally by the `add_linestring()` function which might be slow depending on
your iterator type.

### Adding a multilinestring geometry

Adding a multilinestring works just like adding a linestring, just do the
calls to `add_linestring()` etc. repeatedly for each of the linestrings.

### Adding a polygon geometry

A polygon consists of one outer ring and zero or more inner rings. You have
to first add the outer ring and then the inner rings, if any.

Call `add_ring()` with the number of points in the ring as only argument. After
that call `set_point()` for each of those points. `set_point()` has multiple
overloads just like the `add_point()` method described above. The minimum
number of points is 4 and the last point must be the same as the first point
(or call `close_ring()` instead of the last `set_point()`).

```cpp
...
vtzero::layer_builder lbuilder{...};
try {
    vtzero::polygon_feature_builder fbuilder{lbuilder};
    // optionally set the ID
    fbuilder.set_id(23);
    // add the geometry
    fbuilder.add_ring(5);
    fbuilder.set_point(1, 1);
    fbuilder.set_point(1, 2);
    fbuilder.set_point(2, 2);
    fbuilder.set_point(2, 1);
    fbuilder.set_point(1, 1); // or call fbuilder.close_ring() instead
    // add the properties
    fbuilder.add_property("landuse", "forest");
    // call commit() when you are done
    fbuilder.commit()
} catch (const vtzero::geometry_exception& e) {
    // if we are here, something was wrong with the geometry.
}
```

Note that we have wrapped the feature builder in a try-catch-block here. This
will ignore all geometry errors (which can happen if two consective points
are the same creating a zero-length segment or if the last point is not the
same as the first point).

There are two other versions of the `add_ring()` function. They take two
iterators defining a range to get the points from. Dereferencing those
iterators must yield a `vtzero::point` or something convertible to it. One of
these functions takes a third argument, the number of points the iterator will
yield. If this is not available `std::distance(begin, end)` is called which
internally by the `add_ring()` function which might be slow depending on your
iterator type.

### Adding a multipolygon geometry

Adding a multipolygon works just like adding a polygon, just do the calls to
`add_ring()` etc. repeatedly for each of the rings. Make sure to always first
add an outer ring, then the inner rings in this outer ring, then the next
outer ring and so on.

### Adding an existing geometry

The `geometry_feature_builder` class is used to add geometries you got from
reading a vector tile. This is useful when you want to copy over a geometry
from a feature without decoding it.

```cpp
auto geom = ... // get geometry from a feature you are reading
...
vtzero::tile_builder tb;
vtzero::layer_builder lb{tb};
vtzero::geometry_feature_builder fb{lb};
fb.set_id(123); // optionally set ID
fb.add_geometry(geom) // add geometry
fb.add_property("foo", "bar"); // add properties
fb.commit();
...
```

## Adding properties to the feature

A feature can have any number of properties. They are added with the
`add_property()` method called on the feature builder. There are two different
ways of doing this. The *simple approach* which does all the work for you and
the *advanced approach* which can be more efficient, but you have to to some
more work. It is recommended that you start out with the simple approach and
only switch to the advanced approach once you have a working program and want
to get the last bit of performance out of it.

The difference stems from the way properties are encoded in vector tiles. While
properties "belong" to features, they are really stored in two tables (for the
keys and values) in the layer. The individual feature only references the
entries in those tables by index. This make the encoded tile smaller, but it
means somebody has to manage those tables. In the simple approach this is done
behind the scenes by the `layer_builder` object, in the advanced approach you
handle that yourself.

Do not mix the simple and the advanced approach unless you know what you are
doing.

### The simple approach to adding properties

For the simple approach call `add_property()` with two arguments. The first is
the key, it must have some kind of string type (`std::string`, `const char*`,
`vtzero::data_view`, anything really that converts to a `data_view`). The
second argument is the value, for which most basic C++ types are allowed
(string types, integer types, double, ...). See the API documentation for the
constructors of the `encoded_property_value` class for a list.

```cpp
vtzero::layer_builder lb{...};
vtzero::linestring_feature_builder fb{lb};
...
fb.add_property("waterway", "stream"); // string value
fb.add_property("name", "Little Creek");
fb.add_property("width", 1.5); // double value
...
```

Sometimes you need to specify exactly which type should be used in the
encoding. The `encoded_property_value` constructor can take special types for
that like in the following example, where you force the `sint` encoding:

```cpp
fb.add_property("layer", vtzero::sint_value_type(2));
```

You can also call `add_property()` with a single `vtzero::property` argument
(which is handy if you are copying this property over from a tile you are
reading):

```cpp
while (auto property = feature.next_property()) {
    if (property.key() == "name") {
        feature_builder.add_property(property);
    }
}
```

### The advanced approach to adding properties

In the advanced approach you have to do the indexing yourself. Here is a very
basic example:

```cpp
vtzero::tile_builder tbuilder;
vtzero::layer_builder lbuilder{tbuilder, "test"};
const vtzero::index_value highway = lbuilder.add_key("highway");
const vtzero::index_value primary = lbuilder.add_value("primary");
...
vtzero::point_feature_builder fbuilder{lbuilder};
...
fbuilder.add_property(highway, primary);
...
```

The methods `add_key()` and `add_value()` on the layer builder are used to add
keys and values to the tables in the layer. They both return the index (of type
`vtzero::index_value`) of those keys or values in the tables. You store
those index values somewhere (in this case in the `highway` and `primary`
variables) and use them when calling `add_property()` on the feature builder.

In some cases you only have a few property keys and know them beforehand,
then storing the key indexes in individual variables might work. But for
values this usually doesn't make much sense, and if all your keys and values
are only known at runtime, it doesn't work either. For this you need some kind
of index data structure mapping from keys/values to index values. You can
implement this yourself, but it is easier to use some classes provided by
vtzero. Then the code looks like this:

```cpp
#include <vtzero/index.hpp> // use this include to get the index classes
...
vtzero::layer_builder lb{...};
vtzero::key_index<std::map> key_index{lb};
vtzero::value_index_internal<std::unordered_map> value_index{lb};
...
vtzero::point_feature_builder fb{lb};
...
fb.add_property(key_index("highway"), value_index("primary"));
...
```

In this example the `key_index` template class is used for keys, it uses
`std::map` internally as can be seen by its template argument. The
`value_index_internal` template class is used for values, it uses
`std::unordered_map` internally in this example. Whether you specify `std::map`
or `std::unordered_map` or something else (that needs to be compatible to those
classes) is up to you. Benchmark your use case and decide then.

Keys are always strings, so they are easy to handle. For keys there is only the
single `key_index` in vtzero.

For values this is more difficult. Basically there are two choices:

1. Encode the value according to the vector tile encoding rules which results
   in a string and store this in the index. This is what the
   `value_index_internal` class does.
2. Store the un-encoded value in the index. The index lookup will be faster,
   but you need a different index type for each value type. This is what the
   `value_index` classes do.

The `value_index` template classes need three template arguments: The type
used internally to encode the value, the type used externally, and the map
type.

In this example the user program has the values as `int`, the index will store
them in a `std::map<int>`. The integer value is then encoded in an `sint`
int the vector tile:

```cpp
vtzero::value_index<vtzero::sint_value:type, int, std::map> index;
```

Sometimes these generic indexes based on `std::map` or `std::unordered_map`
are inefficient, that's why there are specialized indexes for special cases:

* The `value_index_bool` class can only index boolean values.
* The `value_index_small_uint` class can only index small unsigned integer
  values (up to `uint16_t`). It uses a vector internally, so if all your
  numbers are small and densely packed, this is very efficient. This is
  especially useful for `enum` types.

## The `add_property()` function.

The last chapters already talked about the `add_property()` function of the
`feature_builder` class. But because it is a bit difficult to see all the
different ways `add_property()` can be called, here is some more information.

The `add_property()` function is called with either two parameters for the
key and value or with one parameter that combines the key and value.

If it is called with an `index_value` for the key or value, that index value is
stored directly into the feature. If it is called with an `index_value_pair`,
the index values in the `index_value_pair` are stored directly in the feature.

If it is called with something that is not an `index_value` or
`index_value_pair`, the function will interpret the data as keys or values.
It will add those keys and values to the layer (if they are not already there),
find the corresponding index values and store them in the feature.

You can mix index-use with non-index use. For instance

```cpp
index_value key_maxspeed = lbuilder.add_key("maxspeed");
...
fbuilder.add_property(key_maxspeed, 30);
```

In this case the key ("maxspeed") was added to the layer once and its index
value (`key_maxspeed`) can later be reused. The value (30), on the other hand,
is only added to the layer in the `add_property()` call.

So for keys, you can have as argument:
* An `index_value`.
* A `data_view` or something that converts to it like a `const char*` or `std::string`.

For values, you can have as argument:
* An `index_value`.
* A `property_value`.
* An `encoded_property_value` or anything that converts to it.

For combined keys and values, you can have as argument:
* An `index_value_pair`.
* A `property`.


## Deriving from `layer_builder` and `feature_builder`

The `vtzero::layer_builder` and `vtzero::feature_builder` classes have been
designed in a way that they can be derived from easily. This allows you to
encapsulate part of your vector tile writing code if some aspects of your
layers/features are always the same, such as the layer name and the names
and types of properties.

Say you want to write a layer named "restaurants" with point geometries.
Each feature should have a name and a 5-star-rating. First you create a
class derived from the `layer_builder` with all the indexes you want to use.
For the keys you don't need indexes in this case, because there are only
two keys for which we can easily store the index values in the layer.

```cpp
class restaurant_layer_builder : public vtzero::layer_builder {

public:

    // The index we'll use for the "name" property values
    vtzero::value_index<vtzero::string_value_type, std::string, std::unordered_map> string_index;

    // The index we'll use for the "stars" property values
    vtzero::value_index_small_uint stars_index;

    // The index value of the "name" key
    vtzero::index_value key_name;

    // The index value of the "stars" key
    vtzero::index_value key_stars;

    restaurant_layer_builder(vtzero::tile_builder& tile) :
        layer_builder(tile, "restaurants"), // the name of the layer
        string_index(*this),
        stars_index(*this),
        key_name(add_key_without_dup_check("name")),
        key_stars(add_key_without_dup_check("stars")) {
    }

};
```

The we'll add a class derived from `feature_builder` to help with adding
features:

```cpp
class restaurant_feature_builder : public vtzero::feature_builder {

    restaurant_layer_builder& m_layer;

public:

    restaurant_feature_builder(restaurant_layer_builder& layer, uint64_t id) :
        vtzero::point_feature_builder(layer), // always a point geometry
        m_layer(layer) {
        set_id(id); // we always have an ID in this case
    }

    void add_location(mylocation& loc) { // restaurant location is stored in your own type
        add_point(loc.lon(), loc.lat());
    }

    void set_name(const std::string& name) {
        add_property(m_layer.key_name,
                     m_layer.string_index(vtzero::encoded_property_value{name}));
    }

    void set_stars(stars s) { // your own "stars" type
        vtzero::encoded_property_value svalue{ s.num_stars() }; // convert stars type to small integer

        add_property(m_layer.key_stars,
                     m_layer.stars_index(svalue));
    }

};
```

This example only shows a general pattern you can follow to derive from the
`layer_builder` and `feature_builder` classes. In some cases this makes more
sense then in others. The derived classes make it easy for you to mix your
own functions (for instance when you need to convert from your own types to
vtzero types like with the `mylocation` and `stars` types above) or just use
the functions in the base classes.

