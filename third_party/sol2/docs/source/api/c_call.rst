c_call
======
*templated type to transport functions through templates*


.. code-block:: cpp
	
	template <typename Function, Function f>
	int c_call (lua_State* L);

	template <typename... Functions>
	int c_call (lua_State* L);

The goal of ``sol::c_call<...>`` is to provide a way to wrap a function and transport it through a compile-time context. This enables faster speed at the cost of a much harder to read / poorer interface, and can alleviate some template compilation speed issues. ``sol::c_call`` expects a type for its first template argument, and a value of the previously provided type for the second template argument. To make a compile-time transported overloaded function, specify multiple functions in the same ``type, value`` pairing, but put it inside of a ``sol::wrap``.

.. note::

	This can also be placed into the argument list for a :doc:`usertype<usertype>` as well. 

This pushes a raw ``lua_CFunction`` into whatever you pass the resulting ``c_call`` function pointer into, whether it be a table or a userdata or whatever else using sol2's API. The resulting ``lua_CFunction`` can also be used directly with the lua API, just like many of sol2's types can be intermingled with Lua's API if you know what you're doing.

It is advisable for the user to consider making a macro to do the necessary ``decltype( &function_name, ), function_name``. Sol does not provide one because many codebases already have `one similar to this`_.

Here's an example below of various ways to use ``sol::c_call``:

.. literalinclude:: ../../../examples/c_call.cpp
	:linenos:

.. _one similar to this: http://stackoverflow.com/a/5628222/5280922
