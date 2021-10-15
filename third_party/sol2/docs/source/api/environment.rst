environment
===========
*encapsulation table for script sandboxing*


.. code-block:: cpp
	:caption: environment

	class environment : public table;

	template <typename T>
	void set_environment( const environment& env, const T& target );
	template <typename E = reference, typename T>
	basic_environment<E> get_environment( const T& target );


This type is passed to :ref:`sol::state(_view)::script/do_x<state-script-function>` to provide an environment where local variables that are set and get retrieve. It is just a plain table, and all the same operations :doc:`from table still apply<table>`. This is important because it allows you to do things like set the table's metatable (using :doc:`sol::metatable_key<metatable_key>` for instance) and having its ``__index`` entry point to the global table, meaning you can get -- but not set -- variables from a Global environment, for example.

There are many more uses, including storing state or special dependent variables in an environment that you pre-create using regular table opertions, and then changing at-will:

.. literalinclude:: ../../../examples/docs/preparing_environments.cpp
	:linenos:

Also note that ``sol::environment`` derives from ``sol::table``, which also derives from ``sol::reference``: in other words, copying one ``sol::environment`` value to another ``sol::environment`` value **does not** deep-copy the table, just creates a new reference pointing to the same lua object.

``sol::environment`` objects can be used with `script calls`_, and it can also be `set on functions`_. It can even be applied to :doc:`threads<thread>`.

You can set the environment using ``sol::set_environment( my_env, some_reference );`` or ``my_env.set_on( some_reference );``.

free functions
--------------

.. code-block:: cpp
	:caption: function: set_environment

	template <typename T>
	void set_environment( const environment& env, const T& target );

See :ref:`environment::set_on<environment-set-on>`.


.. code-block:: cpp
	:caption: function: get_environment

	template <typename E = reference, typename T>
	basic_environment<E> get_environment( const T& target );

This function retrieves the environment from the target object. If it does not have a valid environment, then the environment's valid function will return false after creation. Every function (regular Lua function, executable script, and similar) has an environment, as well as userdata in certain versions of the Lua runtime.


members
-------

.. code-block:: cpp
	:caption: constructor: environment

	environment(lua_State* L, sol::new_table nt);
	environment(lua_State* L, sol::new_table nt, const sol::reference& fallback);
	environment(sol::env_t, const sol::reference& object_that_has_environment);
	environment(sol::env_t, const sol::stack_reference& object_that_has_environment);

The ones from table are used here (of particular note is the ability to use ``sol::environment(my_lua_state, sol::create);`` to make a fresh, unnamed environment), plus the three unique constructors shown above.

The first constructor is generally used as ``sol::environment my_env(my_lua_state, sol::create, my_fallback_table);``. The fallback table serves as the backup to lookup attempts on the environment table being created. It is achieved by simply creating a metatable for the ``sol::environment`` being created, and then doing ``env_metatable["__index"] = fallback;``. You can achieve fancier effects by changing the metatable of the environment to your liking, by creating it in some fashion and then setting the metatable explicitly and populating it with data, particularly with :doc:`sol::metatable_key<metatable_key>`.

The second and third unique constructors take a special empty type that serves as a key to trigger this constructor and serves no other purpose, ``sol::env_t``. The shortcut value so you don't have to create one is called ``sol::env_key``. It is used like ``sol::environment my_env(sol::env_key, some_object);``. It will extract the environment out of whatever the second argument is that may or may not have an environment. If it does not have an environment, the constructor will complete but the object will have ``env.valid() == false``, since it will reference Lua's ``nil``.


.. code-block:: cpp
	:caption: function: set_on
	:name: environment-set-on
	
	template <typename T>
	void set_on(const T& target);

This function applies the environment to the desired target. Not that lua 5.1 only tolerates the application of environments to userdata, threads and functions, while 5.2+ has different (more relaxed) rules. It is called by the free function ``sol::set_environment( env, target );``.

.. _script calls: https://github.com/ThePhD/sol2/blob/develop/examples/environments.cpp
.. _set on functions: https://github.com/ThePhD/sol2/blob/develop/examples/environments_on_functions.cpp
