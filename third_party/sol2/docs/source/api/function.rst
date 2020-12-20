function
========
*calling functions bound to Lua*


.. note::

	This abstraction assumes the function runs safely. If you expect your code to have errors (e.g., you don't always have explicit control over it or are trying to debug errors), please use :doc:`sol::protected_function<protected_function>` explicitly. You can also make ``sol::function`` default to ``sol::protected_function`` by turning on :ref:`the safety features<config>`.

.. code-block:: cpp
	
	class unsafe_function : public reference;
	typedef unsafe_function function;

Function is a correct-assuming version of :doc:`protected_function<protected_function>`, omitting the need for typechecks and error handling (thus marginally increasing speed in some cases). It is the default function type of Sol. Grab a function directly off the stack using the constructor:

.. code-block:: cpp
	:caption: constructor: unsafe_function

	unsafe_function(lua_State* L, int index = -1);


Calls the constructor and creates this type, straight from the stack. For example:

.. literalinclude:: ../../../examples/tie.cpp
	:caption: funcs.lua
	:lines: 9-13
	:linenos:

The following C++ code will call this function from this file and retrieve the return value:

.. literalinclude:: ../../../examples/tie.cpp
	:lines: 1-7,16-22
	:linenos:

The call ``woof(20)`` generates a :ref:`unsafe_function_result<unsafe-function-result>`, which is then implicitly converted to an ``double`` after being called. The intermediate temporary ``function_result`` is then destructed, popping the Lua function call results off the Lua stack. 

You can also return multiple values by using ``std::tuple``, or if you need to bind them to pre-existing variables use ``sol::tie``:

.. literalinclude:: ../../../examples/tie.cpp
	:lines: 24-
	:linenos:

This makes it much easier to work with multiple return values. Using ``std::tie`` from the C++ standard will result in dangling references or bad behavior because of the very poor way in which C++ tuples/``std::tie`` were specified and implemented: please use ``sol::tie( ... )`` instead to satisfy any multi-return needs.

.. _function-result-warning:

.. warning::

	Do NOT save the return type of a :ref:`unsafe_function_result<unsafe-function-result>` (``function_result`` when :ref:`safety configurations are not turned on<config>`) with ``auto``, as in ``auto numwoof = woof(20);``, and do NOT store it anywhere. Unlike its counterpart :ref:`protected_function_result<protected-function-result>`, ``function_result`` is NOT safe to store as it assumes that its return types are still at the top of the stack and when its destructor is called will pop the number of results the function was supposed to return off the top of the stack. If you mess with the Lua stack between saving ``function_result`` and it being destructed, you will be subject to an incredible number of surprising and hard-to-track bugs. Don't do it.

.. code-block:: cpp
	:caption: function: call operator / function call

	template<typename... Args>
	unsafe_function_result operator()( Args&&... args );

	template<typename... Ret, typename... Args>
	decltype(auto) call( Args&&... args );

	template<typename... Ret, typename... Args>
	decltype(auto) operator()( types<Ret...>, Args&&... args );

Calls the function. The second ``operator()`` lets you specify the templated return types using the ``my_func(sol::types<int, std::string>, ...)`` syntax. Function assumes there are no runtime errors, and thusly will call the ``atpanic`` function if a detectable error does occur, and otherwise can return garbage / bogus values if the user is not careful.


To know more about how function arguments are handled, see :ref:`this note<function-argument-handling>`
