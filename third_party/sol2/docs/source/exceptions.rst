exceptions
==========
since somebody is going to ask about it...
------------------------------------------

Yes, you can turn off exceptions in Sol with ``#define SOL_NO_EXCEPTIONS`` before including or by passing the command line argument that defines ``SOL_NO_EXCEPTIONS``. We don't recommend it unless you're playing with a Lua distro that also doesn't play nice with exceptions (like non-x64 versions of :ref:`LuaJIT<LuaJIT and exceptions>` ).

If you turn this off, the default `at_panic`_ function :doc:`state<api/state>` set for you will not throw  (see :ref:`sol::state's automatic handlers<state-automatic-handlers>` for more details). Instead, the default Lua behavior of aborting will take place (and give you no chance of escape unless you implement your own at_panic function and decide to try ``longjmp`` out).

To make this not be the case, you can set a panic function directly with ``lua_atpanic( lua, my_panic_function );`` or when you create the ``sol::state`` with ``sol::state lua(my_panic_function);``. Here's an example ``my_panic_function`` you can have that prints out its errors:

.. literalinclude:: ../../examples/docs/my_panic.cpp
	:caption: typical panic function
	:name: typical-panic-function
	:linenos:


Note that ``SOL_NO_EXCEPTIONS`` will also disable :doc:`sol::protected_function<api/protected_function>`'s ability to catch C++ errors you throw from C++ functions bound to Lua that you are calling through that API. So, only turn off exceptions in Sol if you're sure you're never going to use exceptions ever. Of course, if you are ALREADY not using Exceptions, you don't have to particularly worry about this and now you can use Sol!

If there is a place where a throw statement is called or a try/catch is used and it is not hidden behind a ``#ifndef SOL_NO_EXCEPTIONS`` block, please file an issue at `issue`_ or submit your very own pull request so everyone can benefit!

.. _lua-handlers:

various sol and lua handlers
----------------------------

Lua comes with two kind of built-in handlers that sol provides easy opt-ins for. One is the ``panic`` function, as :ref:`demonstrated above<typical-panic-function>`. Another is the ``pcall`` error handler, used with :doc:`sol::protected_function<api/protected_function>`. It is any function that takes a single argument. The single argument is the error type being passed around: in Lua, this is a single string message:

.. literalinclude:: ../../examples/protected_functions.cpp
	:caption: regular error handling
	:name: regular-error-handling
	:linenos:


The other handler is specific to sol2. If you open a ``sol::state``, or open the default state handlers for your ``lua_State*`` (see :ref:`sol::state's automatic handlers<state-automatic-handlers>` for more details), there is a ``sol::exception_handler_function`` type. It allows you to register a function in the event that an exception happens that bubbles out of your functions. The function requires that you push 1 item onto the stack that will be used with a call to `lua_error`_

.. literalinclude:: ../../examples/exception_handler.cpp
	:caption: exception handling
	:name: exception-handling
	:linenos:


.. _LuaJIT and exceptions:

LuaJIT and exceptions
---------------------

It is important to note that a popular 5.1 distribution of Lua, LuaJIT, has some serious `caveats regarding exceptions`_. LuaJIT's exception promises are flaky at best on x64 (64-bit) platforms, and entirely terrible on non-x64 (32-bit, ARM, etc.) platforms. The trampolines we have in place for all functions bound through conventional means in Sol will catch exceptions and turn them into Lua errors so that LuaJIT remainds unperturbed, but if you link up a C function directly yourself and throw, chances are you might have screwed the pooch.

Testing in `this closed issue`_ that it doesn't play nice on 64-bit Linux in many cases either, especially when it hits an error internal to the interpreter (and does not go through Sol). We do have tests, however, that compile for our continuous integration check-ins that check this functionality across several compilers and platforms to keep you protected and given hard, strong guarantees for what happens if you throw in a function bound by Sol. If you stray outside the realm of Sol's protection, however... Good luck.


.. _exception-interop:

Lua and LuaJIT C++ Exception Full Interoperability
--------------------------------------------------

You can ``#define SOL_EXCEPTIONS_SAFE_PROPAGATION`` before including Sol or define ``SOL_EXCEPTIONS_SAFE_PROPAGATION`` on the command line if you know your implmentation of Lua has proper unwinding semantics that can be thrown through the version of the Lua API you have built / are using.

This will prevent sol from catching ``(...)`` errors in platforms and compilers that have full C++ exception interoperability. This means that Lua errors can be caught with ``catch (...)`` in the C++ end of your code after it goes through Lua, and exceptions can pass through the Lua API and Stack safely.

Currently, the only known platform to do this is the listed "Full" `platforms for LuaJIT`_ and Lua compiled as C++. This define is turned on automatically for compiling Lua as C++ and ``SOL_USING_CXX_LUA`` (or ``SOL_USING_CXX_LUA_JIT``) is defined.

.. warning::

	``SOL_EXCEPTIONS_SAFE_PROPAGATION`` is not defined automatically when Sol detects LuaJIT. *It is your job to define it if you know that your platform supports it*!


.. _issue: https://github.com/ThePhD/sol2/issues/
.. _at_panic: http://www.Lua.org/manual/5.3/manual.html#4.6
.. _lua_error: https://www.lua.org/manual/5.3/manual.html#lua_error
.. _caveats regarding exceptions: http://luajit.org/extensions.html#exceptions
.. _platforms for LuaJIT: http://luajit.org/extensions.html#exceptions
.. _this closed issue: https://github.com/ThePhD/sol2/issues/28
