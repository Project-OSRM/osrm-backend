proxy, (protected\unsafe)_function_result - proxy_base derivatives
==================================================================
*``table[x]`` and ``function(...)`` conversion struct*


.. code-block:: cpp

	template <typename Recurring>
	struct proxy_base;

	template <typename Table, typename Key>
	struct proxy : proxy_base<...>;

	struct stack_proxy: proxy_base<...>;

	struct unsafe_function_result : proxy_base<...>;

	struct protected_function_result: proxy_base<...>;


These classes provide implicit assignment operator ``operator=`` (for ``set``) and an implicit conversion operator ``operator T`` (for ``get``) to support items retrieved from the underlying Lua implementation, specifically :doc:`sol::table<table>` and the results of function calls on :doc:`sol::function<function>` and :doc:`sol::protected_function<protected_function>`.

.. _proxy:

proxy
-----

``proxy`` is returned by lookups into :doc:`sol::table<table>` and table-like entities. Because it is templated on key and table type, it would be hard to spell: you can capture it using the word ``auto`` if you feel like you need to carry it around for some reason before using it. ``proxy`` evaluates its arguments lazily, when you finally call ``get`` or ``set`` on it. Here are some examples given the following lua script:

.. literalinclude:: ../../../examples/table_proxy.cpp
	:linenos:
	:lines: 11-15

After loading that file in or putting it in a string and reading the string directly in lua (see :doc:`state`), you can start kicking around with it in C++ like so:

.. literalinclude:: ../../../examples/table_proxy.cpp
	:linenos:
	:lines: 1-8,18-40

We don't recommend using ``proxy`` lazy evaluation the above to be used across classes or between function: it's more of something you can do to save a reference to a value you like, call a script or run a lua function, and then get it afterwards. You can also set functions (and function objects) this way, and retrieve them as well:

.. literalinclude:: ../../../examples/table_proxy.cpp
	:linenos:
	:lines: 41-

members
-------

.. code-block:: c++
	:caption: functions: [overloaded] implicit conversion get
	:name: implicit-get

	requires( sol::is_primitive_type<T>::value == true )
	template <typename T>
	operator T() const;
	
	requires( sol::is_primitive_type<T>::value == false )
	template <typename T>
	operator T&() const;

Gets the value associated with the keys the proxy was generated and convers it to the type ``T``. Note that this function will always return ``T&``, a non-const reference, to types which are not based on :doc:`sol::reference<reference>` and not a :doc:`primitive lua type<types>`

.. code-block:: c++
	:caption: function: get a value
	:name: regular-get

	template <typename T>
	decltype(auto) get( ) const;

Gets the value associated with the keys and converts it to the type ``T``.

.. code-block:: c++
	:caption: function: optionally get a value
	:name: regular-get-or

	template <typename T, typename Otherwise>
	optional<T> get_or( Otherwise&& otherise ) const;

Gets the value associated with the keys and converts it to the type ``T``. If it is not of the proper type, it will return a ``sol::nullopt`` instead.

.. code-block:: c++
	:caption: function: [overloaded] optionally get or create a value
	:name: regular-get-or-create

	template <typename T>
	decltype(auto) get_or_create();
	template <typename T, typename Otherwise>
	decltype(auto) get_or_create( Otherwise&& other );

Gets the value associated with the keys if it exists. If it does not, it will set it with the value and return the result.

``operator[]`` proxy-only members
---------------------------------

.. code-block:: c++
	:caption: function: valid
	:name: proxy-valid

	bool valid () const;

Returns whether this proxy actually refers to a valid object. It uses :ref:`sol::stack::probe_get_field<stack-probe-get-field>` to determine whether or not its valid.

.. code-block:: c++
	:caption: functions: [overloaded] implicit set
	:name: implicit-set

	requires( sol::detail::Function<Fx> == false )
	template <typename T>
	proxy& operator=( T&& value );
	
	requires( sol::detail::Function<Fx> == true )
	template <typename Fx>
	proxy& operator=( Fx&& function );

Sets the value associated with the keys the proxy was generated with to ``value``. If this is a function, calls ``set_function``. If it is not, just calls ``set``. Does not exist on :ref:`unsage_function_result<unsafe-function-result>` or :ref:`protected_function_result<protected-function-result>`.

.. code-block:: c++
	:caption: function: set a callable
	:name: regular-set-function

	template <typename Fx>
	proxy& set_function( Fx&& fx );

Sets the value associated with the keys the proxy was generated with to a function ``fx``. Does not exist on :ref:`unsafe_function_result<unsafe-function-result>` or :ref:`protected_function_result<protected-function-result>`.


.. code-block:: c++
	:caption: function: set a value
	:name: regular-set

	template <typename T>
	proxy& set( T&& value );

Sets the value associated with the keys the proxy was generated with to ``value``. Does not exist on :ref:`unsafe_function_result<unsafe-function-result>` or :ref:`protected_function_result<protected-function-result>`.

.. _stack-proxy:

stack_proxy
-----------

``sol::stack_proxy`` is what gets returned by :doc:`sol::variadic_args<variadic_args>` and other parts of the framework. It is similar to proxy, but is meant to alias a stack index and not a named variable.

.. _unsafe-function-result:

unsafe_function_result
----------------------

``unsafe_function_result`` is a temporary-only, intermediate-only implicit conversion worker for when :doc:`function<function>` is called. It is *NOT* meant to be stored or captured with ``auto``. It provides fast access to the desired underlying value. It does not implement ``set`` / ``set_function`` / templated ``operator=``, as is present on :ref:`proxy<proxy>`.


This type does, however, allow access to multiple underlying values. Use ``result.get<Type>(index_offset)`` to retrieve an object of ``Type`` at an offset of ``index_offset`` in the results. Offset is 0 based. Not specifying an argument defaults the value to 0.

``unsafe_function_result`` also has ``begin()`` and ``end()`` functions that return (almost) "random-acess" iterators. These return a proxy type that can be implicitly converted to :ref:`stack_proxy<stack-proxy>`.

.. _protected-function-result:

protected_function_result
-------------------------

``protected_function_result`` is a nicer version of ``unsafe_function_result`` that can be used to detect errors. Its gives safe access to the desired underlying value. It does not implement ``set`` / ``set_function`` / templated ``operator=`` as is present on :ref:`proxy<proxy>`.


This type does, however, allow access to multiple underlying values. Use ``result.get<Type>(index_offset)`` to retrieve an object of ``Type`` at an offset of ``index_offset`` in the results. Offset is 0 based. Not specifying an argument defaults the value to 0.

``unsafe_function_result`` also has ``begin()`` and ``end()`` functions that return (almost) "random-acess" iterators. These return a proxy type that can be implicitly converted to :ref:`stack_proxy<stack-proxy>`.

.. _note 1:

on function objects and proxies
-------------------------------

.. note::

	As of recent versions of sol2 (2.18.2 and above), this is no longer an issue, as even bound classes will have any detectable function call operator automatically bound to the object, to allow this to work without having to use ``.set`` or ``.set_function``. The note here is kept for posterity and information for older versions. There are only some small caveats, see: :ref:`this note here<binding-callable-objects>`.

