nested
======


.. code-block:: cpp
	
	template <typename T>
	struct nested {
		T source;
	};


``sol::nested<...>`` is a template class similar to :doc:`sol::as_table<as_table>`, but with the caveat that every :doc:`container type<../containers>` within the ``sol::nested`` type will be retrieved as a table from lua. This is helpful when you need to receive C++-style vectors, lists, and maps nested within each other: all of them will be deserialized from lua using table properties rather than anything else.

Note that any caveats with Lua tables apply the moment it is serialized, and the data cannot be gotten out back out in C++ as a C++ type. You can deserialize the Lua table into something explicitly using the ``sol::as_table_t`` marker for your get and conversion operations using Sol. At that point, the returned type is deserialized **from** a table, meaning you cannot reference any kind of C++ data directly as you do with regular userdata/usertypes. *All C++ type information is lost upon serialization into Lua.*

The `example`_ provides a very in-depth look at both ``sol::as_table<T>`` and ``sol::nested<T>``, and how the two are equivalent.

.. _example: https://github.com/ThePhD/sol2/blob/develop/examples/containers_as_table.cpp