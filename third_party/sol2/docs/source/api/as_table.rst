as_table
===========
*make sure an object is pushed as a table*


.. code-block:: cpp
	
	template <typename T>
	as_table_t { ... };

	template <typename T>
	as_table_t<T> as_function ( T&& container );

This function serves the purpose of ensuring that an object is pushed -- if possible -- like a table into Lua. The container passed here can be a pointer, a reference, a ``std::reference_wrapper`` around a container, or just a plain container value. It must have a begin/end function, and if it has a ``std::pair<Key, Value>`` as its ``value_type``, it will be pushed as a dictionary. Otherwise, it's pushed as a sequence.

.. literalinclude:: ../../../examples/docs/as_table_ipairs.cpp
	:linenos:

Note that any caveats with Lua tables apply the moment it is serialized, and the data cannot be gotten out back out in C++ as a C++ type. You can deserialize the Lua table into something explicitly using the ``sol::as_table_t`` marker for your get and conversion operations using Sol. At that point, the returned type is deserialized **from** a table, meaning you cannot reference any kind of C++ data directly as you do with regular userdata/usertypes. *All C++ type information is lost upon serialization into Lua.*

If you need this functionality with a member variable, use a :doc:`property on a getter function<property>` that returns the result of ``sol::as_table``.

This marker does NOT apply to :doc:`usertypes<usertype>`.

You can also use this to nest types and retrieve tables within tables as shown by `this example`_.

.. _this example: https://github.com/ThePhD/sol2/blob/develop/examples/containers_as_table.cpp
