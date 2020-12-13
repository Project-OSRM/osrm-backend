config and safety
=================

Sol was designed to be correct and fast, and in the pursuit of both uses the regular ``lua_to{x}`` functions of Lua rather than the checking versions (``lua_check{X}``) functions. The API defaults to paranoidly-safe alternatives if you have a ``#define SOL_CHECK_ARGUMENTS`` before you include Sol, or if you pass the ``SOL_CHECK_ARGUMENTS`` define on the build command for your build system. By default, it is off and remains off unless you define this, even in debug mode.

.. _config:

config
------

Note that you can obtain safety with regards to functions you bind by using the :doc:`protect<api/protect>` wrapper around function/variable bindings you set into Lua. Additionally, you can have basic boolean checks when using the API by just converting to a :doc:`sol::optional\<T><api/optional>` when necessary for getting things out of Lua and for function arguments.

Also note that you can have your own states use sol2's safety panics and similar to protect your code from crashes. See :ref:`sol::state automatic handlers<state-automatic-handlers>` for more details.

.. _config-safety:

Safety Config
+++++++++++++

``SOL_SAFE_USERTYPE`` triggers the following change:
	* If the userdata to a usertype function is nil, will trigger an error instead of letting things go through and letting the system segfault/crash
	* Turned on by default with clang++, g++ and VC++ if a basic check for building in debug mode is detected (lack of ``_NDEBUG`` or similar compiler-specific checks)

``SOL_SAFE_REFERENCES`` triggers the following changes:
	* Checks the Lua type to ensure it matches what you expect it to be upon using :doc:`sol::reference<api/reference>` derived types, such as ``sol::thread``, ``sol::function``, etc...
	* Turned on by default with clang++, g++ and VC++ if a basic check for building in debug mode is detected (lack of ``_NDEBUG`` or similar compiler-specific checks)

``SOL_SAFE_FUNCTION_CALLS`` triggers the following changes:
	* ``sol::stack::call`` and its variants will, if no templated boolean is specified, check all of the arguments for a function call
	* All calls from Lua will have their arguments checked
	* Turned on by default with clang++, g++ and VC++ if a basic check for building in debug mode is detected (lack of ``_NDEBUG`` or similar compiler-specific checks)

``SOL_SAFE_FUNCTION`` triggers the following change:
	* All uses of ``sol::function`` and ``sol::stack_function`` will default to ``sol::protected_function`` and ``sol::stack_protected_function``, respectively, rather than ``sol::unsafe_function`` and ``sol::stack_unsafe_function``
		- Note this does not apply to ``sol::stack_aligned_function``: this variant must always be unprotected due to stack positioning requirements, especially in use with ``sol::stack_count``
	* Will make any ``sol::state_view::script`` calls default to their safe variants if there is no supplied environment or error handler function
	* **Not** turned on by default under any detectible compiler settings: *this MUST be turned on manually*

``SOL_SAFE_NUMERICS`` triggers the following changes:
	* Numbers will also be checked to see if they fit within a ``lua_Number`` if there is no ``lua_Integer`` type available that can fit your signed or unsigned number
	* You can opt-out of this behavior with ``SOL_NO_CHECK_NUMBER_PRECISION``
	* **This option is required to differentiate between floats/ints in overloads**
	* **Not** turned on by default under any settings: *this MUST be turned on manually*

``SOL_SAFE_GETTER`` triggers the following changes:
	* ``sol::stack::get`` (used everywhere) defaults to using ``sol::stack::check_get`` and dereferencing the argument. It uses ``sol::type_panic`` as the handler if something goes wrong
	* Affects nearly the entire library for safety (with some blind spots covered by the other definitions)
	* **Not** turned on by default under any settings: *this MUST be turned on manually*

``SOL_DEFAULT_PASS_ON_ERROR`` triggers the following changes:
	* The default error handler for ``sol::state_view::script_`` functions is ``sol::script_pass_on_error`` rather than ``sol::script_throw_on_error``
	* Passes errors on through: **very dangerous** as you can ignore or never be warned about errors if you don't catch the return value of specific functions
	* **Not** turned on by default: *this MUST be turned on manually*
	* Don't turn this on unless you have an extremely good reason
	* *DON'T TURN THIS ON UNLESS YOU HAVE AN EXTREMELY GOOD REASON*

``SOL_CHECK_ARGUMENTS`` triggers the following changes:
	* If ``SOL_SAFE_USERTYPE``, ``SOL_SAFE_REFERENCES``, ``SOL_SAFE_FUNCTION``, ``SOL_SAFE_NUMERICS``, ``SOL_SAFE_GETTER``, and ``SOL_SAFE_FUNCTION_CALLS`` are not defined, they get defined and the effects described above kick in
	* **Not** turned on by default under any settings: *this MUST be turned on manually*

``SOL_NO_CHECK_NUMBER_PRECISION`` triggers the following changes:
	* If ``SOL_SAFE_NUMERICS`` is defined, turns off number precision and integer precision fitting when pushing numbers into sol2
	* **Not** turned on by default under any settings: *this MUST be turned on manually*

``SOL_STRINGS_ARE_NUMBERS`` triggers the following changes:
	* Allows automatic to-string conversions for numbers
		- ``lua_tolstring`` conversions are not permitted on numbers through sol2 by default: only actual strings are allowed
		- This is necessary to allow :doc:`sol::overload<api/overload>` to work properly
	* ``sol::stack::get`` and ``sol::stack::check_get`` will allow anything that Lua thinks is number-worthy to be number-worthy
	* This includes: integers, floating-point numbers, and strings
	* This **does not** include: booleans, types with ``__tostring`` enabled, and everything else
	* Overrides safety and always applies if it is turned on
	* **Not** turned on by default under any settings: *this MUST be turned on manually*

.. _config-feature:

Feature Config
++++++++++++++

``SOL_USE_BOOST`` triggers the following change:
	* Attempts to use ``boost::optional`` instead of sol's own ``optional``
	* **Not** turned on by default under any settings: *this MUST be turned on manually*

``SOL_PRINT_ERRORS`` triggers the following change:
	* Includes ``<iostream>`` and prints all exceptions and errors to ``std::cerr``, for you to see
	* **Not** turned on by default under any settings: *this MUST be turned on manually*

``SOL_CONTAINERS_START`` triggers the following change:
	* If defined and **is an integral value**, is used to adjust the container start value
	* Applies to C++ containers **only** (not Lua tables or algorithms)
	* Defaults to 1 (containers in Lua count from 1)

``SOL_ENABLE_INTEROP`` triggers the following change:
	* Allows the use of ``extensible<T>`` to be used with ``userdata_checker`` and ``userdata_getter`` to retrieve non-sol usertypes
		- Particularly enables non-sol usertypes to be used in overloads
		- See the :ref:`stack dcoumentation<userdata-interop>` for details
	* May come with a slight performance penalty: only recommended for those stuck with non-sol libraries that still need to leverage some of sol's power
	* **Not** turned on by default under any settings: *this MUST be turned on manually*
	
.. _config-memory:

Memory Config
+++++++++++++

``SOL_NO_MEMORY_ALIGNMENT`` triggers the following changes:
	* Memory is no longer aligned and is instead directly sized and allocated
	* If you need to access underlying userdata memory from sol, please see the :doc:`usertype memory documentation<api/usertype_memory>`
	* **Not** turned on by default under any settings: *this MUST be turned on manually*


.. _config-linker:

Linker Config
+++++++++++++

``SOL_USING_CXX_LUA`` triggers the following changes:
	* Lua includes are no longer wrapped in ``extern "C" {}`` blocks
	* Turns on ``SOL_EXCEPTIONS_SAFE_PROPAGATION`` automatically for you
	* Only use this if you know you've built your LuaJIT with the C++-specific invocations of your compiler (Lua by default builds as C code and is not distributed as a C++ library, but a C one with C symbols)

``SOL_USING_CXX_LUA_JIT`` triggers the following changes:
	* LuaJIT includes are no longer wrapped in ``extern "C" {}`` blocks
	* Turns on ``SOL_EXCEPTIONS_SAFE_PROPAGATION`` automatically for you
	* Only use this if you know you've built your LuaJIT with the C++-specific invocations of your compiler
	* LuaJIT by default builds as C code, but includes hook to handle C++ code unwinding: this should almost never be necessary for regular builds

``SOL_EXCEPTIONS_ALWAYS_UNSAFE`` triggers the following changes:
	* If any of the ``SOL_USING_CXX_*`` defines are in play, it **does NOT** automatically turn on ``SOL_EXCEPTIONS_SAFE_PROPAGATION`` automatically
	* This standardizes some behavior, since throwing exceptions through the C API's interface can still lead to undefined behavior that Lua cannot handle properly

``SOL_EXCEPTIONS_SAFE_PROPAGATION`` triggers the following changes:
	* try/catch will not be used around C-function trampolines when going from Lua to C++
	* try/catch will not be used in ``safe_``/``protected_function`` internals
	* Should only be used in accordance with compiling vanilla PUC-RIO Lua as C++, using :ref:`LuaJIT under the proper conditions<exception-interop>`, or in accordance with your Lua distribution's documentation

Tests are compiled with this on to ensure everything is going as expected. Remember that if you want these features, you must explicitly turn them on all of them to be sure you are getting them.

memory
------

Memory safety can be tricky. Lua is handled by a garbage-collected runtime, meaning object deletion is not cleary defined or deterministic. If you need to keep an object from the Lua Runtime alive, use :doc:`sol::reference<api/reference>` or one of its derived types, such as :doc:`sol::table<api/table>`, :doc:`sol::object<api/object>`, or similar. These will pin a reference down to an object controlled in C++, and Lua will not delete an object that you still have a reference to through one of these types. You can then retrieve whatever you need from that Lua slot using object's ``obj.as<T>()`` member function or other things, and work on the memory from there.

The usertype memory layout for all Lua-instantiated userdata and for all objects pushed/set into the Lua Runtime is also described :doc:`here<api/usertype_memory>`. Things before or after that specified memory slot is implementation-defined and no assumptions are to be made about it.

Please be wary of alignment issues. sol2 **aligns memory** by default. If you need to access underlying userdata memory from sol, please see the :doc:`usertype memory documentation<api/usertype_memory>`

functions
---------

The *vast majority* of all users are going to want to work with :doc:`sol::safe_function/sol::protected_function<api/protected_function>`. This version allows for error checking, prunes results, and responds to the defines listed above by throwing errors if you try to use the result of a function without checking. :doc:`sol::function/sol::unsafe_function<api/function>` is unsafe. It assumes that its contents run correctly and throw no errors, which can result in crashes that are hard to debug while offering a very tiny performance boost for not checking error codes or catching exceptions.

If you find yourself crashing inside of ``sol::function``, try changing it to a ``sol::protected_function`` and seeing if the error codes and such help you find out what's going on. You can read more about the API on :doc:`the page itself<api/protected_function>`. You can also define ``SOL_SAFE_FUNCTION`` as described above, but be warned that the ``protected_function`` API is a superset of the regular default ``function`` API: trying to revert back after defining ``SOL_SAFE_FUNCTION`` may result in some compiler errors if you use things beyond the basic, shared interface of the two types.

As a side note, binding functions with default parameters does not magically bind multiple versions of the function to be called with the default parameters. You must instead use :doc:`sol::overload<api/overload>`.

.. warning::

	Do **NOT** save the return type of a :ref:`unsafe_function_result<unsafe-function-result>` with ``auto``, as in ``auto numwoof = woof(20);``, and do NOT store it anywhere unless you are exactly aware of the consequences of messing with the stack. See :ref:`here<function-result-warning>` for more information.
