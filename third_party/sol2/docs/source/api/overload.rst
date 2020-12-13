overload
========
*calling different functions based on argument number/type*


.. code-block:: cpp
	:caption: function: create overloaded set
	:linenos:

	template <typename... Args>
	struct overloaded_set : std::tuple<Args...> { /* ... */ };

	template <typename... Args>
	overloaded_set<Args...> overload( Args&&... args );

The actual class produced by ``sol::overload`` is essentially a type-wrapper around ``std::tuple`` that signals to the library that an overload is being created. The function helps users make overloaded functions that can be called from Lua using 1 name but multiple arguments. It is meant to replace the spaghetti of code where users mock this up by doing strange if statements and switches on what version of a function to call based on `luaL_check{number/udata/string}`_.

.. note::

	Please note that default parameters in a function (e.g., ``int func(int a = 20)``) do not exist beyond C++'s compile-time fun. When that function gets bound or serialized into Lua's framework, it is bound as a function taking 1 argument, not 2 functions taking either 0 or 1 argument. If you want to achieve the same effect, then you need to use overloading and explicitly call the version of the function you want. There is no magic in C++ that allows me to retrieve default parameters and set this up automatically.

.. note::

	Overload resolution can be affected by configuration defines in the :doc:`safety pages<../safety>`. For example, it is impossible to differentiate between integers (uint8_t, in32_t, etc.) versus floating-point types (float, double, half) when ``SOL_SAFE_NUMERICS`` is not turned on.

Its use is simple: wherever you can pass a function type to Lua, whether its on a :doc:`usertype<usertype>` or if you are just setting any kind of function with ``set`` or ``set_function`` (for :doc:`table<table>` or :doc:`state(_view)<state>`), simply wrap up the functions you wish to be considered for overload resolution on one function like so:

.. code-block:: cpp
	
	sol::overload( func1, func2, ... funcN );


The functions can be any kind of function / function object (lambda). Given these functions and struct:

.. literalinclude:: ../../../examples/overloading_with_members.cpp
	:linenos:
	:lines: 1-27

You then use it just like you would for any other part of the api:

.. literalinclude:: ../../../examples/overloading_with_members.cpp
	:linenos:
	:lines: 29-45

Doing the following in Lua will call the specific overloads chosen, and their associated functions:

.. literalinclude:: ../../../examples/overloading_with_members.cpp
	:linenos:
	:lines: 47-

.. note::

	Overloading is done on a first-come, first-serve system. This means if two overloads are compatible, workable overloads, it will choose the first one in the list.

Note that because of this system, you can use :doc:`sol::variadic_args<variadic_args>` to make a function that serves as a "fallback". Be sure that it is the last specified function in the listed functions for ``sol::overload( ... )``. `This example shows how`_.

.. note::

	Please keep in mind that doing this bears a runtime cost to find the proper overload. The cost scales directly not exactly with the number of overloads, but the number of functions that have the same argument count as each other (Sol will early-eliminate any functions that do not match the argument count).

.. _luaL_check{number/udata/string}: http://www.Lua.org/manual/5.3/manual.html#luaL_checkinteger
.. _This example shows how: https://github.com/ThePhD/sol2/blob/develop/examples/overloading_with_fallback.cpp
