containers
==========
*working with containers in sol2*

Containers are objects that are meant to be inspected and iterated and whose job is to typically provide storage to a collection of items. The standard library has several containers of varying types, and all of them have ``begin()`` and ``end()`` methods which return iterators. C-style arrays are also containers, and sol2 will detect all of them for use and bestow upon them special properties and functions.

* Containers from C++ are stored as ``userdata`` with special ``usertype`` metatables with :ref:`special operations<container-operations>`
	- In Lua 5.1, this means containers pushed without wrappers like :doc:`as_table<api/as_table>` and :doc:`nested<api/nested>` will not work with ``pairs`` or other built-in iteration functions from Lua
		+ Lua 5.2+ will behave just fine (does not include LuaJIT 2.0.x)
	- You must push containers into C++ by returning them directly and getting/setting them directly, and they will have a type of ``sol::type::userdata`` and treated like a usertype
* Containers can be manipulated from both C++ and Lua, and, like userdata, will `reflect changes if you use a reference`_ to the data.
* This means containers **do not automatically serialize as Lua tables**
	- If you need tables, consider using ``sol::as_table`` and ``sol::nested``
	- See `this table serialization example`_ for more details
* Lua 5.1 has different semantics for ``pairs`` and ``ipairs``: be wary. See :ref:`examples down below<containers-pairs-example>` for more details
* You can override container behavior by overriding :ref:`the detection trait<container-detection>` and :ref:`specializing the container_traits template<container-traits>`
* You can bind typical C-style arrays, but must follow :ref:`the rules<container-c-array>`

.. _container-c-array:

.. note::
	
	Please note that c-style arrays must be added to Lua using ``lua["my_arr"] = &my_c_array;`` or ``lua["my_arr"] = std::ref(my_c_array);`` to be bestowed these properties. No, a plain ``T*`` pointer is **not** considered an array. This is important because ``lua["my_string"] = "some string";`` is also typed as an array (``const char[n]``) and thusly we can only use ``std::reference_wrapper``\s or pointers to the actual array types to work for this purpose.

.. _container-detection:

container detection
-------------------

containers are detected by the type trait ``sol::is_container<T>``. If that turns out to be true, sol2 will attempt to push a userdata into Lua for the specified type ``T``, and bestow it with some of the functions and properties listed below. These functions and properties are provided by a template struct ``sol::container_traits<T>``, which has a number of static Lua C functions bound to a safety metatable. If you want to override the behavior for a specific container, you must first specialize ``sol::is_container<T>`` to drive from ``std::true_type``, then override the functions you want to change. Any function you do not override will call the default implementation or equivalent. The default implementation for unrecognized containers is simply errors.

You can also specialize ``sol::is_container<T>`` to turn off container detection, if you find it too eager for a type that just happens to have ``begin`` and ``end`` functions, like so:

.. code-block:: cpp
	:caption: not_container.hpp

	struct not_container {
		void begin() {

		}

		void end() {

		}
	};

	namespace sol {
		template <>
		struct is_container<not_container> : std::false_type {};
	}

This will let the type be pushed as a regular userdata.

.. note::

	Pushing a new :doc:`usertype<api/usertype>` will prevent a qualifying C++ container type from being treated like a container. To force a type that you've registered/bound as a usertype using ``new_usertype`` or ``new_simple_usertype`` to be treated like a container, use :doc:`sol::as_container<api/as_container>`. 


.. _container-traits:

container overriding
--------------------

If you **want** it to participate as a table, use ``std::true_type`` instead of ``std::false_type`` from the :ref:`containter detection<container-detection>` example. and provide the appropriate ``iterator`` and ``value_type`` definitions on the type. Failure to do so will result in a container whose operations fail by default (or compilation will fail).

If you need a type whose declaration and definition you do not have control over to be a container, then you must override the default behavior by specializing container traits, like so:

.. code-block:: cpp
	:caption: specializing.hpp

	struct not_my_type { ... };

	namespace sol {
		template <>
		struct is_container<not_my_type> : std::true_type {};

		template <>
		struct container_traits<not_my_type> {

			...
			// see below for implemetation details	
		};
	}


The various operations provided by ``container_traits<T>`` are expected to be like so, below. Ability to override them requires familiarity with the Lua stack and how it operates, as well as knowledge of Lua's :ref:`raw C functions<raw-function-note>`. You can read up on raw C functions by looking at the "Programming in Lua" book. The `online version's information`_ about the stack and how to return information is still relevant, and you can combine that by also using sol's low-level :doc:`stack API<api/stack>` to achieve whatever behavior you need.

.. warning::

	Exception handling **WILL** be provided around these particular raw C functions, so you do not need to worry about exceptions or errors bubbling through and handling that part. It is specifically handled for you in this specific instance, and **ONLY** in this specific instance. The raw note still applies to every other raw C function you make manually.

.. _container-operations:

container operations
-------------------------

Below are the many container operations and their override points for ``container_traits<T>``. Please use these to understand how to use any part of the implementation.

+-----------+-------------------------------------------+--------------------------------------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| operation | lua syntax                                | container_traits<T>                              | stack argument order | notes/caveats                                                                                                                                                                                |
|           |                                           | extension point                                  |                      |                                                                                                                                                                                              |
+-----------+-------------------------------------------+--------------------------------------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| set       | ``c:set(key, value)``                     | ``static int set(lua_State*);``                  | 1 self               | - if ``value`` is nil, it performs an erase in default implementation                                                                                                                        |
|           |                                           |                                                  | 2 key                | - if this is a sequence container and it support insertion and ``key``,is an index equal to the size of the container,+ 1, it will insert at,the end of the container (this is a Lua idiom)  |
|           |                                           |                                                  | 3 value              |                                                                                                                                                                                              |
+-----------+-------------------------------------------+--------------------------------------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| index_set | ``c[key] = value``                        | ``static int index_set(lua_State*);``            | 1 self               | - default implementation calls "set"                                                                                                                                                         |
|           |                                           |                                                  | 2 key                | - if this is a sequence container and it support insertion and ``key`` is an index equal to the size of the container  + 1, it will insert at the end of the container (this is a Lua idiom) |
|           |                                           |                                                  | 3 value              |                                                                                                                                                                                              |
+-----------+-------------------------------------------+--------------------------------------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| at        | ``v = c:at(key)``                         | ``static int at(lua_State*);``                   | 1 self               | - can return multiple values                                                                                                                                                                 |
|           |                                           |                                                  | 2 index              | - default implementation increments iterators linearly for non-random-access                                                                                                                 |
|           |                                           |                                                  |                      | - if the type does not have random-access iterators, **do not use this in a for loop** !                                                                                                     |
+-----------+-------------------------------------------+--------------------------------------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| get       | ``v = c:get(key)``                        | ``static int get(lua_State*);``                  | 1 self               | - can return multiple values                                                                                                                                                                 |
|           |                                           |                                                  | 2 key                | - default implementation increments iterators linearly for non-random-access                                                                                                                 |
+-----------+-------------------------------------------+--------------------------------------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| index_get | ``v = c[key]``                            | ``static int index_get(lua_State*);``            | 1 self               | - can only return 1 value                                                                                                                                                                    |
|           |                                           |                                                  | 2 key                | - default implementation just calls "get"                                                                                                                                                    |
|           |                                           |                                                  |                      | - if ``key`` is a string and ``key`` is one of the other member functions, it will return that member function rather than perform a lookup / index get                                      |
+-----------+-------------------------------------------+--------------------------------------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| find      | ``c:find(target)``                        | ``static int find(lua_State*);``                 | 1 self               | - ``target`` is a value for non-lookup containers (fixed containers, sequence containers, non-associative and non-ordered containers)                                                        |
|           |                                           |                                                  | 2 target             |                                                                                                                                                                                              |
+-----------+-------------------------------------------+--------------------------------------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| erase     | ``c:erase(target)``                       | ``static int erase(lua_State*);``                | 1 self               | - for sequence containers, ``target`` is an index to erase                                                                                                                                   |
|           |                                           |                                                  | 2 target             | - for lookup containers, ``target`` is the key type                                                                                                                                          |
|           |                                           |                                                  |                      | - uses linear incrementation to spot for sequence containers that do not have random access iterators (``std::list``, ``std::forward_list``, and similar)                                    |
|           |                                           |                                                  |                      | - invalidates iteration                                                                                                                                                                      |
+-----------+-------------------------------------------+--------------------------------------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| insert    | ``c:insert(target, value)``               |                                                  | 1 self               | - for sequence containers, ``target`` is an index, otherwise it is the key type                                                                                                              |
|           |                                           |                                                  | 2 target             | - inserts into a container if possible at the specified location                                                                                                                             |
|           |                                           |                                                  | 3 key                |                                                                                                                                                                                              |
+-----------+-------------------------------------------+--------------------------------------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| add       | ``c:add(key, value)`` or ``c:add(value)`` | ``static int add(lua_State*);``                  | 1 self               | - 2nd argument (3rd on stack) is provided for associative containers to add                                                                                                                  |
|           |                                           |                                                  | 2 key/value          | - ordered containers will insert into the appropriate spot, not necessarily at the end                                                                                                       |
|           |                                           |                                                  | 3 value              |                                                                                                                                                                                              |
+-----------+-------------------------------------------+--------------------------------------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| size      | ``#c``                                    | ``static int size(lua_State*);``                 | 1 self               | - default implementation calls ``.size()`` if present                                                                                                                                        |
|           |                                           |                                                  |                      | - otherwise, default implementation uses ``std::distance(begin(L, self), end(L, self))``                                                                                                     |
+-----------+-------------------------------------------+--------------------------------------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| clear     | ``c:clear()``                             | ``static int clear(lua_State*);``                | 1 self               | - default implementation provides no fallback if there's no ``clear`` operation                                                                                                              |
+-----------+-------------------------------------------+--------------------------------------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| offset    | n/a                                       | ``static int index_adjustment(lua_State*, T&);`` | n/a                  | - returns an index that adds to the passed-in numeric index for array acces (default implementation is ``return -1`` to simulate 1-based indexing from Lua)                                  |
+-----------+-------------------------------------------+--------------------------------------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| begin     | n/a                                       | ``static iterator begin(lua_State*, T&);``       | n/a                  | - called by default implementation                                                                                                                                                           |
|           |                                           |                                                  |                      | - is not the regular raw function: must return an iterator from second "T&" argument, which is self                                                                                          |
+-----------+-------------------------------------------+--------------------------------------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| end       | n/a                                       | ``static iterator end(lua_State*, T&);``         | n/a                  | - called by default implementation                                                                                                                                                           |
|           |                                           |                                                  |                      | - is not the regular raw function: must return an iterator from second "T&" argument, which is self                                                                                          |
+-----------+-------------------------------------------+--------------------------------------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| next      |                                           | ``static int next(lua_State*);``                 | 1 self               | - implement if advanced user only that understands caveats                                                                                                                                   |
|           |                                           |                                                  |                      | - is used as 'iteration function' dispatched with pairs() call                                                                                                                               |
|           |                                           |                                                  |                      |                                                                                                                                                                                              |
|           |                                           |                                                  |                      |                                                                                                                                                                                              |
+-----------+-------------------------------------------+--------------------------------------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| pairs     |                                           | ``static int pairs(lua_State*);``                | 1 self               | - implement if advanced user only that understands caveats                                                                                                                                   |
|           |                                           |                                                  |                      | - override begin and end instead and leave this to default implementation if you do not know what ``__pairs`` is for or how to implement it and the ``next`` function                        |
|           |                                           |                                                  |                      | - works only in Lua 5.2+                                                                                                                                                                     |
|           |                                           |                                                  |                      | - calling ``pairs( c )`` in Lua 5.1 / LuaJIT will crash with assertion failure (Lua expects ``c`` to be a table); can be used as regular function ``c:pairs()`` to get around limitation     |
+-----------+-------------------------------------------+--------------------------------------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ipairs    |                                           | ``static int ipairs(lua_State*);``               | 1 self               | - implement if advanced user only that understands caveats                                                                                                                                   |
|           |                                           |                                                  |                      | - override begin and end instead and leave this to default implementation if you do not know what ``__ipairs`` is for or how to implement it and the ``next`` function                       |
|           |                                           |                                                  |                      | - works only in Lua 5.2, deprecated in Lua 5.3 (but might still be called in compatibiltiy modes)                                                                                            |
|           |                                           |                                                  |                      | - calling ``ipairs( c )`` in Lua 5.1 / LuaJIT will crash with assertion failure (Lua expects ``c`` to be a table)                                                                            |
+-----------+-------------------------------------------+--------------------------------------------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+

.. note::

	If your type does not adequately support ``begin()`` and ``end()`` and you cannot override it, use the ``sol::is_container`` trait override along with a custom implementation of ``pairs`` on your usertype to get it to work as you want it to. Note that a type not having proper ``begin()`` and ``end()`` will not work if you try to forcefully serialize it as a table (this means avoid using :doc:`sol::as_table<api/as_table>` and :doc:`sol::nested<api/nested>`, otherwise you will have compiler errors). Just set it or get it directly, as shown in the examples, to work with the C++ containers.

.. note::

	Overriding the detection traits and operation traits listed above and then trying to use ``sol::as_table`` or similar can result in compilation failures if you do not have a proper ``begin()`` or ``end()`` function on the type. If you want things to behave with special usertype considerations, please do not wrap the container in one of the special table-converting/forcing abstractions.


.. _container-classifications: 

container classifications
-------------------------

When you serialize a container into sol2, the default container handler deals with the containers by inspecting various properties, functions, and typedefs on them. Here are the broad implications of containers sol2's defaults will recognize, and which already-known containers fall into their categories:

+------------------------+----------------------------------------+-------------------------+-----------------------------------------------------------------------------------------------+
| container type         | requirements                           | known containers        | notes/caveats                                                                                 |
+------------------------+----------------------------------------+-------------------------+-----------------------------------------------------------------------------------------------+
| sequence               | ``erase(iterator)``                    | std::vector             | - ``find`` operation is linear in size of list (searches all elements)                        |
|                        | ``push_back``/``insert(value_type)``   | std::deque              | - std::forward_list has forward-only iterators: set/find is a linear operation                |
|                        |                                        | std::list               | - std::forward_list uses "insert_after" idiom, requires special handling internally           |
|                        |                                        | std::forward_list       |                                                                                               |
+------------------------+----------------------------------------+-------------------------+-----------------------------------------------------------------------------------------------+
| fixed                  | lacking ``push_back``/``insert``       | std::array<T, n>        | - regular c-style arrays must be set with ``std::ref( arr )`` or ``&arr`` to be usable        |
|                        | lacking ``erase``                      | T[n] (fixed arrays)     |                                                                                               |
|                        |                                        |                         |                                                                                               |
+------------------------+----------------------------------------+-------------------------+-----------------------------------------------------------------------------------------------+
| ordered                | ``key_type`` typedef                   | std::set                | - ``container[key] = stuff`` operation erases when ``stuff`` is nil, inserts/sets when not    |
|                        | ``erase(key)``                         | std::multi_set          | - ``container.get(key)`` returns the key itself                                               |
|                        | ``find(key)``                          |                         |                                                                                               |
|                        | ``insert(key)``                        |                         |                                                                                               |
+------------------------+----------------------------------------+-------------------------+-----------------------------------------------------------------------------------------------+
| associative, ordered   | ``key_type``, ``mapped_type`` typedefs | std::map                |                                                                                               |
|                        | ``erase(key)``                         | std::multi_map          |                                                                                               |
|                        | ``find(key)``                          |                         |                                                                                               |
|                        | ``insert({ key, value })``             |                         |                                                                                               |
+------------------------+----------------------------------------+-------------------------+-----------------------------------------------------------------------------------------------+
| unordered              | same as ordered                        | std::unordered_set      | - ``container[key] = stuff`` operation erases when ``stuff`` is nil, inserts/sets when not    |
|                        |                                        | std::unordered_multiset | - ``container.get(key)`` returns the key itself                                               |
|                        |                                        |                         | - iteration not guaranteed to be in order of insertion, just like in C++ container            |
|                        |                                        |                         |                                                                                               |
+------------------------+----------------------------------------+-------------------------+-----------------------------------------------------------------------------------------------+
| unordered, associative | same as ordered, associative           | std::unordered_map      | - iteration not guaranteed to be in order of insertion, just like in C++ container            |
|                        |                                        | std::unordered_multimap |                                                                                               |
+------------------------+----------------------------------------+-------------------------+-----------------------------------------------------------------------------------------------+


a complete example
------------------

Here's a complete working example of it working for Lua 5.3 and Lua 5.2, and how you can retrieve out the container in all versions:

.. literalinclude:: ../../examples/containers.cpp
	:name: containers-example
	:linenos:


Note that this will not work well in Lua 5.1, as it has explicit table checks and does not check metamethods, even when ``pairs`` or ``ipairs`` is passed a table. In that case, you will need to use a more manual iteration scheme or you will have to convert it to a table. In C++, you can use :doc:`sol::as_table<api/as_table>` when passing something to the library to get a table out of it: ``lua["arr"] = as_table( std::vector<int>{ ... });``. For manual iteration in Lua code without using ``as_table`` for something with indices, try:

.. code-block:: lua
	:caption: iteration.lua
	:linenos:

	for i = 1, #vec do
		print(i, vec[i]) 
	end

There are also other ways to iterate over key/values, but they can be difficult AND cost your performance due to not having proper support in Lua 5.1. We recommend that you upgrade to Lua 5.2 or 5.3 if this is integral to your infrastructure.

If you can't upgrade, use the "member" function ``my_container:pairs()`` in Lua to perform iteration:

.. literalinclude:: ../../examples/container_with_pairs.cpp
	:name: containers-pairs-example
	:linenos:

.. _online version's information: https://www.lua.org/pil/26.html
.. _reflect changes if you use a reference: https://github.com/ThePhD/sol2/blob/develop/examples/containers.cpp
.. _this table serialization example: https://github.com/ThePhD/sol2/blob/develop/examples/containers_as_table.cpp
