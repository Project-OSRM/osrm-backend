table
=====
*a representation of a Lua (meta)table*


.. code-block:: cpp
	
	template <bool global>
	class table_core;

	typedef table_core<false> table;
	typedef table_core<true> global_table;

``sol::table`` is an extremely efficient manipulator of state that brings most of the magic of the Sol abstraction. Capable of doing multiple sets at once, multiple gets into a ``std::tuple``, being indexed into using ``[key]`` syntax and setting keys with a similar syntax (see: :doc:`here<proxy>`), ``sol::table`` is the corner of the interaction between Lua and C++.

There are two kinds of tables: the global table and non-global tables: however, both have the exact same interface and all ``sol::global_table`` s are convertible to regular ``sol::table`` s.

Tables are the core of Lua, and they are very much the core of Sol.


members
-------

.. code-block:: cpp
	:caption: constructor: table

	table(lua_State* L, int index = -1);
	table(lua_State* L, sol::new_table nt)

The first takes a table from the Lua stack at the specified index and allows a person to use all of the abstractions therein. The second creates a new table using the capacity hints specified in ``sol::new_table``'s structure (``sequence_hint`` and ``map_hint``). If you don't care exactly about the capacity, create a table using ``sol::table my_table(my_lua_state, sol::create);``. Otherwise, specify the table creation size hints by initializing it manually through :doc:`sol::new_table's simple constructor<new_table>`.

.. code-block:: cpp
	:caption: function: get / traversing get
	:name: get-value

	template<typename... Args, typename... Keys>
	decltype(auto) get(Keys&&... keys) const;

	template<typename T, typename... Keys>
	decltype(auto) traverse_get(Keys&&... keys) const;

	template<typename T, typename Key>
	decltype(auto) get_or(Key&& key, T&& otherwise) const;

	template<typename T, typename Key, typename D>
	decltype(auto) get_or(Key&& key, D&& otherwise) const;


These functions retrieve items from the table. The first one (``get``) can pull out *multiple* values, 1 for each key value passed into the function. In the case of multiple return values, it is returned in a ``std::tuple<Args...>``. It is similar to doing ``return table["a"], table["b"], table["c"]``. Because it returns a ``std::tuple``, you can use ``std::tie``/``std::make_tuple`` on a multi-get to retrieve all of the necessary variables. The second one (``traverse_get``) pulls out a *single* value,	using each successive key provided to do another lookup into the last. It is similar to doing ``x = table["a"]["b"]["c"][...]``.

If the keys within nested queries try to traverse into a table that doesn't exist, it will first pull out a ``nil`` value. If there are further lookups past a key that do not exist, the additional lookups into the nil-returned variable will cause a panic to be fired by the lua C API. If you need to check for keys, check with ``auto x = table.get<sol::optional<int>>( std::tie("a", "b", "c" ) );``, and then use the :doc:`optional<optional>` interface to check for errors. As a short-hand, easy method for returning a default if a value doesn't exist, you can use ``get_or`` instead.

This function does not create tables where they do not exist.

.. code-block:: cpp
	:caption: function: raw get / traversing raw get
	:name: raw-get-value

	template<typename... Args, typename... Keys>
	decltype(auto) raw_get(Keys&&... keys) const;

	template<typename T, typename... Keys>
	decltype(auto) traverse_raw_get(Keys&&... keys) const;

	template<typename T, typename Key>
	decltype(auto) raw_get_or(Key&& key, T&& otherwise) const;

	template<typename T, typename Key, typename D>
	decltype(auto) raw_get_or(Key&& key, D&& otherwise) const;


Similar to :ref:`get<get-value>`, but it does so "raw" (ignoring metamethods on the table's metatable).

.. code-block:: cpp
	:caption: function: set / traversing set
	:name: set-value

	template<typename... Args>
	table& set(Args&&... args);

	template<typename... Args>
	table& traverse_set(Args&&... args);

These functions set items into the table. The first one (``set``) can set  *multiple* values, in the form ``key_a, value_a, key_b, value_b, ...``. It is similar to ``table[key_a] = value_a; table[key_b] = value_b, ...``. The second one (``traverse_set``) sets a *single* value, using all but the last argument as keys to do another lookup into the value retrieved prior to it. It is equivalent to ``table[key_a][key_b][...] = value;``.

If the keys within nested queries try to traverse into a table that doesn't exist, it will first pull out a ``nil`` value. If there are further lookups past a key that do not exist, the additional lookups into the nil-returned variable will cause a panic to be fired by the lua C API.

Please note how callables and lambdas are serialized, as there may be issues on GCC-based implementations. See this :ref:`note here<lambda-registry>`.

This function does not create tables where they do not exist.

.. code-block:: cpp
	:caption: function: raw set / traversing raw set
	:name: raw-set-value

	template<typename... Args>
	table& raw_set(Args&&... args);

	template<typename... Args>
	table& traverse_raw_set(Args&&... args);

Similar to :ref:`set<set-value>`, but it does so "raw" (ignoring metamethods on the table's metatable).

Please note how callables and lambdas are serialized, as there may be issues on GCC-based implementations. See this :ref:`note here<lambda-registry>`.

.. note::

	Value semantics are applied to all set operations. If you do not ``std::ref( obj )`` or specifically make a pointer with ``std::addressof( obj )`` or ``&obj``, it will copy / move. This is different from how :doc:`sol::function<function>` behaves with its call operator. Also note that this does not detect callables by default: see the :ref:`note here<binding-callable-objects>`.

.. code-block:: cpp
	:caption: function: set a function with the specified key into lua
	:name: set-function

	template<typename Key, typename Fx>
	state_view& set_function(Key&& key, Fx&& fx, [...]);

Sets the desired function to the specified key value. Note that it also allows for passing a member function plus a member object or just a single member function: however, using a lambda is almost always better when you want to bind a member function + class instance to a single function call in Lua. Also note that this will allow Lua to understand that a callable object (such as a lambda) should be serialized as a function and not as a userdata: see the :ref:`note here<binding-callable-objects>` for more details.

.. code-block:: cpp
	:caption: function: add

	template<typename... Args>
	table& add(Args&&... args);

This function appends a value to a table. The definition of appends here is only well-defined for a table which has a perfectly sequential (and integral) ordering of numeric keys with associated non-null values (the same requirement for the :ref:`size<size-function>` function). Otherwise, this falls to the implementation-defined behavior of your Lua VM, whereupon is may add keys into empty 'holes' in the array (e.g., the first empty non-sequential integer key it gets to from ``size``) or perhaps at the very "end" of the "array". Do yourself the favor of making sure your keys are sequential.

Each argument is appended to the list one at a time.

.. code-block:: cpp
	:caption: function: size
	:name: size-function

	std::size_t size() const;

This function returns the size of a table. It is only well-defined in the case of a Lua table which has a perfectly sequential (and integral) ordering of numeric keys with associated non-null values.
	
.. code-block:: cpp
	:caption: function: setting a usertype
	:name: new-usertype

	template<typename Class, typename... Args>
	table& new_usertype(const std::string& name, Args&&... args);
	template<typename Class, typename CTor0, typename... CTor, typename... Args>
	table& new_usertype(const std::string& name, Args&&... args);
	template<typename Class, typename... CArgs, typename... Args>
	table& new_usertype(const std::string& name, constructors<CArgs...> ctor, Args&&... args);

This class of functions creates a new :doc:`usertype<usertype>` with the specified arguments, providing a few extra details for constructors. After creating a usertype with the specified argument, it passes it to :ref:`set_usertype<set_usertype>`.
	
.. code-block:: cpp
	:caption: function: creating an enum
	:name: new-enum

	template<bool read_only = true, typename... Args>
	basic_table_core& new_enum(const std::string& name, Args&&... args);
	template<typename T, bool read_only = true>
	basic_table_core& new_enum(const std::string& name, std::initializer_list<std::pair<string_view, T>> items);
	
Use this function to create an enumeration type in Lua. By default, the enum will be made read-only, which creates a tiny performance hit to make the values stored in this table behave exactly like a read-only enumeration in C++. If you plan on changing the enum values in Lua, set the ``read_only`` template parameter in your ``new_enum`` call to false. The arguments are expected to come in ``key, value, key, value, ...`` list.

If you use the second overload, you will create a (runtime) ``std::initializer_list``. This will avoid compiler overhead for excessively large enumerations. For this overload, hoever, you must pass the enumeration name as a template parameter first, and then the ``read_only`` parameter, like ``lua.new_enum<my_enum>( "my_enum", { {"a", my_enum:: a}, { "b", my_enum::b } } );``.

.. _set_usertype:

.. code-block:: cpp
	:caption: function: setting a pre-created usertype
	:name: set-usertype

	template<typename T>
	table& set_usertype(usertype<T>& user);
	template<typename Key, typename T>
	table& set_usertype(Key&& key, usertype<T>& user);

Sets a previously created usertype with the specified ``key`` into the table. Note that if you do not specify a key, the implementation falls back to setting the usertype with a ``key`` of ``usertype_traits<T>::name``, which is an implementation-defined name that tends to be of the form ``{namespace_name 1}_[{namespace_name 2 ...}_{class name}``.

.. code-block:: cpp
	:caption: function: begin / end for iteration
	:name: table-iterators

	table_iterator begin () const;
	table_iterator end() const;
	table_iterator cbegin() const;
	table_iterator cend() const;

Provides (what can barely be called) `input iterators`_ for a table. This allows tables to work with single-pass, input-only algorithms (like ``std::for_each``). Note that manually getting an iterator from ``.begin()`` without a ``.end()`` or using postfix incrementation (``++mytable.begin()``) will lead to poor results. The Lua stack is manipulated by an iterator and thusly not performing the full iteration once you start is liable to ruin either the next iteration or break other things subtly. Use a C++11 ranged for loop, ``std::for_each``, or other algorithims which pass over the entire collection at least once and let the iterators fall out of scope.

.. _iteration_note:
.. warning::

	The iterators you use to walk through a ``sol::table`` are NOT guaranteed to iterate in numeric order, or ANY kind of order. They may iterate backwards, forwards, in the style of cuckoo-hashing, by accumulating a visited list while calling ``rand()`` to find the next target, or some other crazy scheme. Now, no implementation would be crazy, but it is behavior specifically left undefined because there are many ways that your Lua package can implement the table type.

	Iteration order is NOT something you should rely on. If you want to figure out the length of a table, call the length operation (``int count = mytable.size();`` using the sol API) and then iterate from ``1`` to ``count`` (inclusive of the value of count, because Lua expects iteration to work in the range of ``[1, count]``). This will save you some headaches in the future when the implementation decides not to iterate in numeric order.


.. code-block:: cpp
	:caption: function: iteration with a function
	:name: table-for-each

	template <typename Fx>
	void for_each(Fx&& fx);

A functional ``for_each`` loop that calls the desired function. The passed in function must take either ``sol::object key, sol::object value`` or take a ``std::pair<sol::object, sol::object> key_value_pair``. This version can be a bit safer as allows the implementation to definitively pop the key/value off the Lua stack after each call of the function.

.. code-block:: cpp
	:caption: function: operator[] access

	template<typename T>
	proxy<table&, T> operator[](T&& key);
	template<typename T>
	proxy<const table&, T> operator[](T&& key) const;

Generates a :doc:`proxy<proxy>` that is templated on the table type and the key type. Enables lookup of items and their implicit conversion to a desired type. Lookup is done lazily.

Please note how callables and lambdas are serialized, as there may be issues on GCC-based implementations. See this :ref:`note here<lambda-registry>`.

.. code-block:: cpp
	:caption: function: create a table with defaults
	:name: table-create

	table create(int narr = 0, int nrec = 0);
	template <typename Key, typename Value, typename... Args>
	table create(int narr, int nrec, Key&& key, Value&& value, Args&&... args);
	
	static table create(lua_State* L, int narr = 0, int nrec = 0);
	template <typename Key, typename Value, typename... Args>
	static table create(lua_State* L, int narr, int nrec, Key&& key, Value&& value, Args&&... args);

Creates a table, optionally with the specified values pre-set into the table. If ``narr`` or ``nrec`` are 0, then compile-time shenanigans are used to guess the amount of array entries (e.g., integer keys) and the amount of hashable entries (e.g., all other entries).

.. code-block:: cpp
	:caption: function: create a table with compile-time defaults assumed
	:name: table-create-with

	template <typename... Args>
	table create_with(Args&&... args);
	template <typename... Args>
	static table create_with(lua_State* L, Args&&... args);
	

Creates a table, optionally with the specified values pre-set into the table. It checks every 2nd argument (the keys) and generates hints for how many array or map-style entries will be placed into the table. Applies the same rules as :ref:`table.set<set-value>` when putting the argument values into the table, including how it handles callable objects.

.. code-block:: cpp
	:caption: function: create a named table with compile-time defaults assumed
	:name: table-create-named

	template <typename Name, typename... Args>
	table create_named(Name&& name, Args&&... args);
	

Creates a table, optionally with the specified values pre-set into the table, and sets it as the key ``name`` in the table. Applies the same rules as :ref:`table.set<set-value>` when putting the argument values into the table, including how it handles callable objects.

.. _input iterators: http://en.cppreference.com/w/cpp/concept/InputIterator
