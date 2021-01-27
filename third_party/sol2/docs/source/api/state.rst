state
=====
*owning and non-owning state holders for registry and globals*


.. code-block:: cpp

	class state_view;
	
	class state : state_view, std::unique_ptr<lua_State*, deleter>;

The most important class here is ``state_view``. This structure takes a ``lua_State*`` that was already created and gives you simple, easy access to Lua's interfaces without taking ownership. ``state`` derives from ``state_view``, inheriting all of this functionality, but has the additional purpose of creating a fresh ``lua_State*`` and managing its lifetime for you in its constructors.

The majority of the members between ``state_view`` and :doc:`sol::table<table>` are identical, with a few added for this higher-level type. Therefore, all of the examples and notes in :doc:`sol::table<table>` apply here as well.

``state_view`` is cheap to construct and creates 2 references to things in the ``lua_State*`` while it is alive: the global Lua table, and the Lua C Registry.

.. _state-automatic-handlers:

``sol::state`` automatic handlers
---------------------------------

One last thing you should understand: constructing a ``sol::state`` does a few things behind-the-scenes for you, mostly to ensure compatibility and good error handler/error handling. The function it uses to do this is ``set_default_state``. They are as follows:

* set a default panic handler with ``state_view::set_panic``/``lua_atpnic``
* set a default ``sol::protected_function`` handler with ``sol::protected_function::set_default_handler``, using a ``sol::reference`` to ``&sol::detail::default_traceback_error_handler`` as the default handler function
* set a default exception handler to ``&sol::detail::default_exception_handler``
* register the state as the main thread (only does something for Lua 5.1, which does not have a way to get the main thread) using ``sol::stack::register_main_thread(L)``
* register the LuaJIT C function exception handler with ``stack::luajit_exception_handler(L)``

You can read up on the various panic and exception handlers on the :ref:`exceptions page<lua-handlers>`.

sol::state_view does none of these things for you. If you want to make sure your self-created or self-managed state has the same properties, please apply this function once to the state. Please note that it will override your panic handler and, if using LuaJIT, your LuaJIT C function handler.

.. warning::

	It is your responsibility to make sure ``sol::state_view`` goes out of scope before you call ``lua_close`` on a pre-existing state, or before ``sol::state`` goes out of scope and its destructor gets called. Failure to do so can result in intermittent crashes because the ``sol::state_view`` has outstanding references to an already-dead ``lua_State*``, and thusly will try to decrement the reference counts for the Lua Registry and the Global Table on a dead state. Please use ``{`` and ``}`` to create a new scope, or other lifetime techniques, when you know you are going to call ``lua_close`` so that you have a chance to specifically control the lifetime of a ``sol::state_view`` object.

enumerations
------------

.. code-block:: cpp
	:caption: in-lua libraries
	:name: lib-enum

	enum class lib : char {
	    base,
	    package,
	    coroutine,
	    string,
	    os,
	    math,
	    table,
	    debug,
	    bit32,
	    io,
	    ffi,
	    jit,
	    count // do not use
	};

This enumeration details the various base libraries that come with Lua. See the `standard lua libraries`_ for details about the various standard libraries.


members
-------

.. code-block:: cpp
	:caption: function: open standard libraries/modules
	:name: open-libraries

	template<typename... Args>
	void open_libraries(Args&&... args);

This function takes a number of :ref:`sol::lib<lib-enum>` as arguments and opens up the associated Lua core libraries.

.. code-block:: cpp
	:caption: function: script / safe_script / script_file / safe_script_file / unsafe_script / unsafe_script_file
	:name: state-script-function

	function_result script(const string_view& code, const std::string& chunk_name = "[string]", load_mode mode = load_mode::any);
	protected_function_result script(const string_view& code, const environment& env, const std::string& chunk_name = "[string]", load_mode mode = load_mode::any);
	template <typename ErrorFunc>
	protected_function_result script(const string_view& code, ErrorFunc&& on_error, const std::string& chunk_name = "[string]", load_mode mode = load_mode::any);
	template <typename ErrorFunc>
	protected_function_result script(const string_view& code, const environment& env, ErrorFunc&& on_error, const std::string& chunk_name = "[string]", load_mode mode = load_mode::any);

	function_result script_file(const std::string& filename, load_mode mode = load_mode::any);
	protected_function_result script_file(const std::string& filename, const environment& env, load_mode mode = load_mode::any);
	template <typename ErrorFunc>
	protected_function_result script_file(const std::string& filename, ErrorFunc&& on_error, load_mode mode = load_mode::any);
	template <typename ErrorFunc>
	protected_function_result script_file(const std::string& filename, const environment& env, ErrorFunc&& on_error, load_mode mode = load_mode::any);

If you need safety, please use the version of these functions with ``safe`` (such as ``safe_script(_file)``) appended in front of them. They will always check for errors and always return a ``sol::protected_function_result``. If you explicitly do not want to check for errors and want to simply invoke ``lua_error`` in the case of errors (which will call ``panic``), use ``unsafe_script(_file)`` versions.

These functions run the desired blob of either code that is in a string, or code that comes from a filename, on the ``lua_State*``. It will not run isolated: any scripts or code run will affect code in the ``lua_State*`` the object uses as well (unless ``local`` is applied to a variable declaration, as specified by the Lua language). Code ran in this fashion is not isolated. If you need isolation, consider creating a new state or traditional Lua sandboxing techniques.

If your script returns a value, you can capture it from the returned :ref:`sol::unsafe_function_result<unsafe-function-result>`/:ref:`sol::protected_function_result<protected-function-result>`. Note that the plain versions that do not take an environment or a callback function assume that the contents internally not only loaded properly but ran to completion without errors, for the sake of simplicity and performance.

To handle errors when using the second overload, provide a callable function/object that takes a ``lua_State*`` as its first argument and a ``sol::protected_function_result`` as its second argument. ``sol::script_default_on_error`` and ``sol::script_pass_on_error`` are 2 functions provided by sol that will either generate a traceback error to return / throw (if throwing is allowed); or, pass the error on through and return it to the user (respectively). An example of having your:

.. literalinclude:: ../../../examples/docs/state_script_safe.cpp
	:linenos:
	:name: state-script-safe

You can also pass a :doc:`sol::environment<environment>` to ``script``/``script_file`` to have the script have sandboxed / contained in a way inside of a state. This is useful for runnig multiple different "perspectives" or "views" on the same state, and even has fallback support. See the :doc:`sol::environment<environment>` documentation for more details. 

.. code-block:: cpp
	:caption: function: require / require_file
	:name: state-require-function

	sol::object require(const std::string& key, lua_CFunction open_function, bool create_global = true);
	sol::object require_script(const std::string& key, const std::string& code, bool create_global = true);
	sol::object require_file(const std::string& key, const std::string& file, bool create_global = true);

These functions play a role similar to `luaL_requiref`_ except that they make this functionality available for loading a one-time script or a single file. The code here checks if a module has already been loaded, and if it has not, will either load / execute the file or execute the string of code passed in. If ``create_global`` is set to true, it will also link the name ``key`` to the result returned from the open function, the code or the file. Regardless or whether a fresh load happens or not, the returned module is given as a single :doc:`sol::object<object>` for you to use as you see fit.

Thanks to `Eric (EToreo) for the suggestion on this one`_!

.. code-block:: cpp
	:caption: function: load / load_file
	:name: state-load-code

	sol::load_result load(lua_Reader reader, void* data, const std::string& chunk_name = "[string]", load_mode mode = load_mode::any);
	sol::load_result load(const string_view& code, const std::string& chunk_name = "[string]", load_mode mode = load_mode::any);
	sol::load_result load_buffer(const char* buff, std::size_t buffsize, const std::string& chunk_name = "[string]", load_mode mode = load_mode::any);
	sol::load_result load_file(const std::string& filename, load_mode mode = load_mode::any);

These functions *load* the desired blob of either code that is in a string, or code that comes from a filename, on the ``lua_State*``. That blob will be turned into a Lua Function. It will not be run: it returns a ``load_result`` proxy that can be called to actually run the code, when you are ready. It can also be turned into a ``sol::function``, a ``sol::protected_function``, or some other abstraction that can serve to call the function. If it is called, it will run on the object's current ``lua_State*``: it is not isolated. If you need isolation, consider using :doc:`sol::environment<environment>`, creating a new state, or other Lua sandboxing techniques.

Finally, if you have a custom source of data, you can use the ``lua_Reader`` overloaded function alongside passing in a ``void*`` pointing to a single type that has everything you need to run it. Use that callback to provide data to the underlying Lua implementation to read data, as explained `in the Lua manual`_.

This is a low-level function and if you do not understand the difference between loading a piece of code versus running that code, you should be using :ref:`state_view::script<state-script-function>`.

.. code-block:: cpp
	:caption: function: do_string / do_file
	:name: state-do-code

	sol::protected_function_result do_string(const string_view& code);
	sol::protected_function_result do_file(const std::string& filename);
	sol::protected_function_result do_string(const string_view& code, sol::environment env);
	sol::protected_function_result do_file(const std::string& filename, sol::environment env);

These functions *loads and performs* the desired blob of either code that is in a string, or code that comes from a filename, on the ``lua_State*``. It *will* run, and then return a ``protected_function_result`` proxy that can be examined for either an error or the return value. This function does not provide a callback like :ref:`state_view::script<state-script-function>` does. It is a lower-level function that performs less checking and directly calls ``load(_file)`` before running the result, with the optional environment.

It is advised that, unless you have specific needs or the callback function is not to your liking, that you work directly with :ref:`state_view::script<state-script-function>`.

.. code-block:: cpp
	:caption: function: global table / registry table

	sol::global_table globals() const;
	sol::table registry() const;

Get either the global table or the Lua registry as a :doc:`sol::table<table>`, which allows you to modify either of them directly. Note that getting the global table from a ``state``/``state_view`` is usually unnecessary as it has all the exact same functions as a :doc:`sol::table<table>` anyhow.


.. code-block:: cpp
	:caption: function: set_panic
	:name: set-panic

	void set_panic(lua_CFunction panic);

Overrides the panic function Lua calls when something unrecoverable or unexpected happens in the Lua VM. Must be a function of the that matches the ``int(lua_State*)`` function signature.


.. code-block:: cpp
	:caption: function: memory_used
	:name: memory-used

	std::size_t memory_used() const;

Returns the amount of memory used *in bytes* by the Lua State.


.. code-block:: cpp
	:caption: function: collect_garbage
	:name: collect-garbage

	void collect_garbage();

Attempts to run the garbage collector. Note that this is subject to the same rules as the Lua API's collect_garbage function: memory may or may not be freed, depending on dangling references or other things, so make sure you don't have tables or other stack-referencing items currently alive or referenced that you want to be collected.


.. code-block:: cpp
	:caption: function: make a table

	sol::table create_table(int narr = 0, int nrec = 0);
	template <typename Key, typename Value, typename... Args>
	sol::table create_table(int narr, int nrec, Key&& key, Value&& value, Args&&... args);


	template <typename... Args>
	sol::table create_table_with(Args&&... args);
	
	static sol::table create_table(lua_State* L, int narr = 0, int nrec = 0);
	template <typename Key, typename Value, typename... Args>
	static sol::table create_table(lua_State* L, int narr, int nrec, Key&& key, Value&& value, Args&&... args);

Creates a table. Forwards its arguments to :ref:`table::create<table-create>`. Applies the same rules as :ref:`table.set<set-value>` when putting the argument values into the table, including how it handles callable objects.

.. _standard lua libraries: http://www.lua.org/manual/5.3/manual.html#6 
.. _luaL_requiref: https://www.lua.org/manual/5.3/manual.html#luaL_requiref
.. _Eric (EToreo) for the suggestion on this one: https://github.com/ThePhD/sol2/issues/90
.. _in the Lua manual: https://www.lua.org/manual/5.3/manual.html#lua_Reader
