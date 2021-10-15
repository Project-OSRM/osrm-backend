tutorial: quick 'n' dirty 
=========================

These are all the things. Use your browser's search to find things you want.

.. note::

	After you learn the basics of sol, it is usually advised that if you think something can work, you should TRY IT. It will probably work!	

.. note::
	
	All of the code below is available at the `sol2 tutorial examples`_.

asserts / prerequisites
-----------------------

You'll need to ``#include <sol.hpp>``/``#include "sol.hpp"`` somewhere in your code. Sol is header-only, so you don't need to compile anything. However, **Lua must be compiled and available**. See the :doc:`getting started tutorial<getting-started>` for more details.

The implementation for ``assert.hpp`` with ``c_assert`` looks like so:

.. literalinclude:: ../../../examples/assert.hpp
	:linenos:
	:lines: 1-3, 19-

This is the assert used in the quick code below.

opening a state
---------------

.. literalinclude:: ../../../examples/tutorials/quick_n_dirty/opening_a_state.cpp
	:linenos:


.. _sol-state-on-lua-state:

using sol2 on a lua_State\*
---------------------------

For your system/game that already has Lua or uses an in-house or pre-rolled Lua system (LuaBridge, kaguya, Luwra, etc.), but you'd still like sol2 and nice things:


.. literalinclude:: ../../../examples/tutorials/quick_n_dirty/opening_state_on_raw_lua.cpp
	:linenos:

.. _running-lua-code:

running lua code
----------------

.. literalinclude:: ../../../examples/tutorials/quick_n_dirty/running_lua_code.cpp
	:linenos:
	:lines: 1-10, 16-26

To run Lua code but have an error handler in case things go wrong:

.. literalinclude:: ../../../examples/tutorials/quick_n_dirty/running_lua_code.cpp
	:linenos:
	:lines: 28-39,47-


running lua code (low-level)
----------------------------

You can use the individual load and function call operator to load, check, and then subsequently run and check code.

.. warning::

	This is ONLY if you need some sort of fine-grained control: for 99% of cases, :ref:`running lua code<running-lua-code>` is preferred and avoids pitfalls in not understanding the difference between script/load and needing to run a chunk after loading it.


.. literalinclude:: ../../../examples/tutorials/quick_n_dirty/running_lua_code_low_level.cpp
	:linenos:
	:lines: 1-10, 16-40, 47-49

set and get variables
---------------------

You can set/get everything using table-like syntax.
	
.. literalinclude:: ../../../examples/tutorials/quick_n_dirty/set_and_get_variables.cpp
	:linenos:
	:lines: 1-19

Equivalent to loading lua values like so:

.. literalinclude:: ../../../examples/tutorials/quick_n_dirty/set_and_get_variables.cpp
	:linenos:
	:lines: 22-34

You can show they are equivalent:

.. literalinclude:: ../../../examples/tutorials/quick_n_dirty/set_and_get_variables.cpp
	:linenos:
	:lines: 36-44

Retrieve these variables using this syntax:

.. literalinclude:: ../../../examples/tutorials/quick_n_dirty/set_and_get_variables.cpp
	:linenos:
	:lines: 45-64

Retrieve Lua types using ``object`` and other ``sol::`` types.

.. literalinclude:: ../../../examples/tutorials/quick_n_dirty/set_and_get_variables.cpp
	:linenos:
	:lines: 66-

You can erase things by setting it to ``nullptr`` or ``sol::lua_nil``.

.. literalinclude:: ../../../examples/tutorials/quick_n_dirty/set_and_get_variables_exists.cpp
	:linenos:

Note that if its a :doc:`userdata/usertype<../api/usertype>` for a C++ type, the destructor will run only when the garbage collector deems it appropriate to destroy the memory. If you are relying on the destructor being run when its set to ``sol::lua_nil``, you're probably committing a mistake.

tables
------

Tables can be manipulated using accessor-syntax. Note that :doc:`sol::state<../api/state>` is a table and all the methods shown here work with ``sol::state``, too.

.. literalinclude:: ../../../examples/tutorials/quick_n_dirty/tables_and_nesting.cpp
	:linenos:
	:lines: 1-34

If you're going deep, be safe: 

.. literalinclude:: ../../../examples/tutorials/quick_n_dirty/tables_and_nesting.cpp
	:linenos:
	:lines: 35-

make tables
-----------

There are many ways to make a table. Here's an easy way for simple ones:

.. literalinclude:: ../../../examples/tutorials/quick_n_dirty/make_tables.cpp
	:linenos:
	:lines: 1-21

Equivalent Lua code, and check that they're equivalent:

.. literalinclude:: ../../../examples/tutorials/quick_n_dirty/make_tables.cpp
	:linenos:
	:lines: 22-

You can put anything you want in tables as values or keys, including strings, numbers, functions, other tables.

Note that this idea that things can be nested is important and will help later when you get into :ref:`namespacing<namespacing>`.


functions
---------

They're easy to use, from Lua and from C++:

.. literalinclude:: ../../../examples/tutorials/quick_n_dirty/functions_easy.cpp
	:linenos:
	:lines: 1-

If you need to protect against errors and parser problems and you're not ready to deal with Lua's ``longjmp`` problems (if you compiled with C), use :doc:`sol::protected_function<../api/protected_function>`.

You can bind member variables as functions too, as well as all KINDS of function-like things:

.. literalinclude:: ../../../examples/tutorials/quick_n_dirty/functions_all.cpp
	:linenos:
	:lines: 1-50

The lua code to call these things is:

.. literalinclude:: ../../../examples/tutorials/quick_n_dirty/functions_all.cpp
	:linenos:
	:lines: 51-

You can use ``sol::readonly( &some_class::variable )`` to make a variable readonly and error if someone tries to write to it.


self call
---------

You can pass the ``self`` argument through C++ to emulate 'member function' calls in Lua.

.. literalinclude:: ../../../examples/tutorials/quick_n_dirty/self_call.cpp
	:linenos:
	:lines: 1-


multiple returns from lua
-------------------------

.. literalinclude:: ../../../examples/tutorials/quick_n_dirty/multiple_returns_from_lua.cpp
	:linenos:
	:lines: 1-


multiple returns to lua
-----------------------

.. literalinclude:: ../../../examples/tutorials/quick_n_dirty/multiple_returns_to_lua.cpp
	:linenos:
	:lines: 1-


C++ classes from C++
--------------------

Everything that is not a:

	* primitive type: ``bool``, ``char/short/int/long/long long``, ``float/double``
	* string type: ``std::string``, ``const char*``
	* function type: function pointers, ``lua_CFunction``, ``std::function``, :doc:`sol::function/sol::protected_function<../api/function>`, :doc:`sol::coroutine<../api/coroutine>`, member variable, member function
	* designated sol type: :doc:`sol::table<../api/table>`, :doc:`sol::thread<../api/thread>`, :doc:`sol::error<../api/error>`, :doc:`sol::object<../api/object>`
	* transparent argument type: :doc:`sol::variadic_arg<../api/variadic_args>`, :doc:`sol::this_state<../api/this_state>`, :doc:`sol::this_environment<../api/this_environment>`
	* usertype<T> class: :doc:`sol::usertype<../api/usertype>`

Is set as a :doc:`userdata + usertype<../api/usertype>`.


.. literalinclude:: ../../../examples/tutorials/quick_n_dirty/userdata.cpp
	:linenos:
	:lines: 1-57,97-

``std::unique_ptr``/``std::shared_ptr``'s reference counts / deleters will :doc:`be respected<../api/unique_usertype_traits>`.

If you want it to refer to something, whose memory you know won't die in C++ while it is used/exists in Lua, do the following:

.. literalinclude:: ../../../examples/tutorials/quick_n_dirty/userdata_memory_reference.cpp
	:linenos:
	:lines: 1-45

You can retrieve the userdata in the same way as everything else. Importantly, note that you can change the data of usertype variables and it will affect things in lua if you get a pointer or a reference:

.. literalinclude:: ../../../examples/tutorials/quick_n_dirty/userdata_memory_reference.cpp
	:linenos:
	:lines: 46-


C++ classes put into Lua
------------------------

See this :doc:`section here<cxx-in-lua>`. Also check out a `basic example`_, `special functions example`_ and  `initializers example`_! There are many more examples that show off the usage of classes in C++, so please peruse them all carefully as it can be as simple or as complex as your needs are. 


.. _namespacing:

namespacing
-----------

You can emulate namespacing by having a table and giving it the namespace names you want before registering enums or usertypes:

.. literalinclude:: ../../../examples/tutorials/quick_n_dirty/namespacing.cpp
	:linenos:
	:lines: 1-


This technique can be used to register namespace-like functions and classes. It can be as deep as you want. Just make a table and name it appropriately, in either Lua script or using the equivalent Sol code. As long as the table FIRST exists (e.g., make it using a script or with one of Sol's methods or whatever you like), you can put anything you want specifically into that table using :doc:`sol::table's<../api/table>` abstractions.

there is a LOT more
-------------------

Some more things you can do/read about:
	* :doc:`the usertypes page<../usertypes>` lists the huge amount of features for functions
		- :doc:`unique usertype traits<../api/unique_usertype_traits>` allows you to specialize handle/RAII types from other libraries frameworks, like boost and Unreal, to work with Sol. Allows custom smart pointers, custom handles and others
	* :doc:`the containers page<../containers>` gives full information about handling everything about container-like usertypes
	* :doc:`the functions page<../functions>` lists a myriad of features for functions
		- :doc:`variadic arguments<../api/variadic_args>` in functions with ``sol::variadic_args``.
		- also comes with :doc:`variadic_results<../api/variadic_results>` for returning multiple differently-typed arguments
		- :doc:`this_state<../api/this_state>` to get the current ``lua_State*``, alongside other transparent argument types
	* :doc:`metatable manipulations<../api/metatable_key>` allow a user to change how indexing, function calls, and other things work on a single type.
	* :doc:`ownership semantics<ownership>` are described for how Lua deals with its own internal references and (raw) pointers.
	* :doc:`stack manipulation<../api/stack>` to safely play with the stack. You can also define customization points for ``stack::get``/``stack::check``/``stack::push`` for your type.
	* :doc:`make_reference/make_object convenience function<../api/make_reference>` to get the same benefits and conveniences as the low-level stack API but put into objects you can specify.
	* :doc:`stack references<../api/stack_reference>` to have zero-overhead Sol abstractions while not copying to the Lua registry.
	* :doc:`resolve<../api/resolve>` overloads in case you have overloaded functions; a cleaner casting utility. You must use this to emulate default parameters.

.. _basic example: https://github.com/ThePhD/sol2/blob/develop/examples/usertype.cpp
.. _special functions example: https://github.com/ThePhD/sol2/blob/develop/examples/usertype_special_functions.cpp
.. _initializers example: https://github.com/ThePhD/sol2/blob/develop/examples/usertype_initializers.cpp
.. _sol2 tutorial examples: https://github.com/ThePhD/sol2/tree/develop/examples/tutorials/quick_n_dirty
