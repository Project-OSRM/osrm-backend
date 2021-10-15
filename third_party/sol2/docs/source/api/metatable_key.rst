metatable_key
=============
*a key for setting and getting an object's metatable*


.. code-block:: cpp

	struct metatable_key_t {};
	const metatable_key_t metatable_key;

You can use this in conjunction with :doc:`sol::table<table>` to set/get a metatable. Lua metatables are powerful ways to override default behavior of objects for various kinds of operators, among other things. Here is an entirely complete example, showing getting and working with a :doc:`usertype<usertype>`'s metatable defined by Sol:

.. literalinclude:: ../../../examples/metatable_key_low_level.cpp
	:caption: messing with metatables
	:linenos:
