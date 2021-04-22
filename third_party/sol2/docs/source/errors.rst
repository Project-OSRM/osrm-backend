errors
======
how to handle exceptions or other errors 
----------------------------------------

Here is some advice and some tricks for common errors about iteration, compile time / linker errors, and other pitfalls, especially when dealing with thrown exceptions, error conditions and the like in Sol.

Running Scripts
---------------

Scripts can have syntax errors, can load from the file system wrong, or have runtime issues. Knowing which one can be troublesome. There are various small building blocks to load and run code, but to check errors you can use the overloaded :ref:`script/script_file functions on sol::state/sol::state_view<state-script-function>`, specifically the ``safe_script`` variants. These also take an error callback that is called only when something goes wrong, and Sol comes with some default error handlers in the form of ``sol::script_default_on_error`` and ``sol::script_pass_on_error``.

.. _compilation_errors_warnings:

Compiler Errors / Warnings
--------------------------

A myriad of compiler errors can occur when something goes wrong. Here is some basic advice about working with these types:

* If there are a myriad of errors relating to ``std::index_sequence``, type traits, and other ``std::`` members, it is likely you have not turned on your C++14 switch for your compiler. Visual Studio 2015 turns these on by default, but g++ and clang++ do not have them as defaults and you should pass the flag ``--std=c++1y`` or ``--std=c++14``, or similar for your compiler.
* If you are pushing a non-primitive type into Lua, you may get strange errors about initializer lists or being unable to initializer a ``luaL_Reg``. This may be due to :ref:`automatic function and operator registration<automagical-registration>`. :ref:`Disabling it<automagical>` may help.
* Sometimes, a generated usertype can be very long if you are binding a lot of member functions. You may end up with a myriad of warnings about debug symbols being cut off or about ``__LINE_VAR`` exceeding maximum length. You can silence these warnings safely for some compilers.
* Template depth errors may also be a problem on earlier versions of clang++ and g++. Use ``-ftemplate-depth`` compiler flag and specify really high number (something like 2048 or even double that amount) to let the compiler work freely.
* When using usertype templates extensively, MSVC may invoke `compiler error C1128 <https://msdn.microsoft.com/en-us/library/8578y171.aspx>`_ , which is solved by using the `/bigobj compilation flag <https://msdn.microsoft.com/en-us/library/ms173499.aspx>`_.
* If you have a move-only type, that type may need to be made ``readonly`` if it is bound as a member variable on a usertype or bound using ``state_view::set_function``. See :doc:`sol::readonly<api/readonly>` for more details.
* Assigning a ``std::string`` or a ``std::pair<T1, T2>`` using ``operator=`` after it's been constructed can result in compiler errors when working with ``sol::function`` and its results. See `this issue for fixes to this behavior`_.
* Sometimes, using ``__stdcall`` in a 32-bit (x86) environment on VC++ can cause problems binding functions because of a compiler bug. We have a prelimanry fix in, but if it doesn't work and there are still problems: put the function in a ``std::function`` to make the compiler errors and other problems go away. Also see `this __stdcall issue report`_ for more details.
* If you are using ``/std:c++latest`` on VC++ and get a number of errors for ``noexcept`` specifiers on functions, you may need to file an issue or wait for the next release of the VC++ compiler.

Mac OSX Crashes
---------------

On LuaJIT, your code may crash at seemingly random points when using Mac OSX. Make sure that your build has these flags, as advised by the LuaJIT website::

	-pagezero_size 10000 -image_base 100000000

These will allow your code to run properly, without crashing arbitrarily. Please read the LuaJIT documentation on compiling and running with LuaJIT for more information.


"compiler out of heap space"
----------------------------

Typical of Visual Studio, the compiler will complain that it is out of heap space because Visual Studio defaults to using the x86 (32-bit) version of itself (it will still compile x86 or x64 or ARM binaries, just the compiler **itself** is a 32-bit executable). In order to get around heap space requirements, add the following statement in your ``.vcoxproj`` files under the ``<Import .../>`` statement, as instructed by `OrfeasZ in this issue`_::

	<PropertyGroup>
		<PreferredToolArchitecture>x64</PreferredToolArchitecture>
	</PropertyGroup>


This should use the 64-bit tools by default, and increase your maximum heap space to whatever a 64-bit windows machine can handle. If you do not have more than 4 GB of RAM, or you still encounter issues, you should look into using ``create_simple_usertype`` and adding functions 1 by 1 using ``.set( ... )``, as shown in `the simple usertype example here`_.


Linker Errors
-------------

There are lots of reasons for compiler linker errors. A common one is not knowing that you've compiled the Lua library as C++: when building with C++, it is important to note that every typical (static or dynamic) library expects the C calling convention to be used and that Sol includes the code using ``extern 'C'`` where applicable.

However, when the target Lua library is compiled with C++, one must change the calling convention and name mangling scheme by getting rid of the ``extern 'C'`` block. This can be achieved by adding ``#define SOL_USING_CXX_LUA`` before including sol2, or by adding it to your compilation's command line. If you build LuaJIT in C++ mode (how you would even, is beyond me), then you need to ``#define SOL_USING_CXX_LUAJIT`` as well. Typically, there is never a need to use this last one.

Note that you should not be defining these with standard builds of either Lua or LuaJIT. See the :ref:`config page<config-linker>` for more details.

"caught (...) exception" errors
-------------------------------

Sometimes, you expect properly written errors and instead receive an error about catching a ``...`` exception instead. This might mean that you either built Lua as C++ or are using a framework like LuaJIT that has full interopability support for exceptions on certain system types (x64 for LuaJIT 2.0.5, x86 and x64 on LuaJIT 2.1.x-beta and later).

Please make sure to use the ``SOL_EXCEPTIONS_SAFE_PROPAGATION`` define before including sol2 to make this work out. You can read more :ref:`at the exception page here<exception-interop>`.

Catch and CRASH!
----------------

By default, Sol will add a ``default_at_panic`` handler to states opened by Sol (see :ref:`sol::state automatic handlers<state-automatic-handlers>` for more details). If exceptions are not turned off, this handler will throw to allow the user a chance to recover. However, in almost all cases, when Lua calls ``lua_atpanic`` and hits this function, it means that something *irreversibly wrong* occured in your code or the Lua code and the VM is in an unpredictable or dead state. Catching an error thrown from the default handler and then proceeding as if things are cleaned up or okay is NOT the best idea. Unexpected bugs in optimized and release mode builds can result, among other serious issues.

It is preferred if you catch an error that you log what happened, terminate the Lua VM as soon as possible, and then crash if your application cannot handle spinning up a new Lua state. Catching can be done, but you should understand the risks of what you're doing when you do it. For more information about catching exceptions, the potentials, not turning off exceptions and other tricks and caveats, read about :doc:`exceptions in Sol here<exceptions>`.

Lua is a C API first and foremost: exceptions bubbling out of it is essentially last-ditch, terminal behavior that the VM does not expect. You can see an example of handling a panic on the exceptions page :ref:`here<typical-panic-function>`. This means that setting up a ``try { ... } catch (...) {}`` around an unprotected sol2 function or script call is **NOT** enough to keep the VM in a clean state. Lua does not understand exceptions and throwing them results in undefined behavior if they bubble through the C API once and then the state is used again. Please catch, and crash.

Furthermore, it would be a great idea for you to use the safety features talked about :doc:`safety section<safety>`, especially for those related to functions.


Destructors and Safety
----------------------

Another issue is that Lua is a C API. It uses ``setjmp`` and ``longjmp`` to jump out of code when an error occurs. This means it will ignore destructors in your code if you use the library or the underlying Lua VM improperly. To solve this issue, build Lua as C++. When a Lua VM error occurs and ``lua_error`` is triggered, it raises it as an exception which will provoke proper unwinding semantics.

Building Lua as C++ gets around this issue, and allows lua-thrown errors to properly stack unwind.


Protected Functions and Access
------------------------------

By default, :doc:`sol::function<api/function>` assumes the code ran just fine and there are no problems. :ref:`sol::state(_view)::script(_file)<state-script-function>` also assumes that code ran just fine. Use :doc:`sol::protected_function<api/protected_function>` to have function access where you can check if things worked out. Use :doc:`sol::optional<api/optional>` to get a value safely from Lua. Use :ref:`sol::state(_view)::do_string/do_file/load/load_file<state-do-code>` to safely load and get results from a script. The defaults are provided to be simple and fast with thrown exceptions to violently crash the VM in case things go wrong.

Protected Functions Are Not Catch All
-------------------------------------

Sometimes, some scripts load poorly. Even if you protect the function call, the actual file loading or file execution will be bad, in which case :doc:`sol::protected_function<api/protected_function>` will not save you. Make sure you register your own panic handler so you can catch errors, or follow the advice of the catch + crash behavior above. Remember that you can also bind your own functions and forego sol2's built-in protections for you own by binding a :ref:`raw lua_CFunction function<raw-function-note>`

Iteration
---------

Tables may have other junk on them that makes iterating through their numeric part difficult when using a bland ``for-each`` loop, or when calling sol's ``for_each`` function. Use a numeric look to iterate through a table. Iteration does not iterate in any defined order also: see :ref:`this note in the table documentation for more explanation<iteration_note>`.

.. _OrfeasZ in this issue: https://github.com/ThePhD/sol2/issues/329#issuecomment-276824983
.. _this issue for fixes to this behavior: https://github.com/ThePhD/sol2/issues/414#issuecomment-306839439
.. _this __stdcall issue report: https://github.com/ThePhD/sol2/issues/463
.. _the simple usertype example here: https://github.com/ThePhD/sol2/blob/develop/examples/usertype_simple.cpp#L45
