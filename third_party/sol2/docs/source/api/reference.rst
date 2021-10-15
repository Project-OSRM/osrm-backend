reference
=========
*general purpose reference to Lua object in registry*


..  code-block:: cpp
	:caption: reference
		
	class reference;

This type keeps around a reference to something inside of Lua, whether that object was on the stack or already present as an object in the Lua Runtime. It places the object Lua registry and will keep it alive.

It is the backbone for all things that reference items on the stack that need to be kept around beyond their appearance and lifetime on said Lua stack or need to be kept alive outside of a script beyond garbage collection times. Its progeny include :doc:`sol::coroutine<coroutine>`, :doc:`sol::function<function>`, :doc:`sol::protected_function<protected_function>`, :doc:`sol::object<object>`, :doc:`sol::table<table>`/:doc:`sol::global_table<table>`, :doc:`sol::thread<thread>`, and :doc:`sol::(light_)userdata<userdata>`, which are type-specific versions of ``sol::reference``.

Note that if you need to keep a reference to something inside of Lua, it is better to use ``sol::reference`` or :doc:`sol::object<object>` to keep a reference to it and then use the ``obj.as<T>()`` member function to retrieve what you need than to take a direct dependency on the memory by retrieving a pointer or reference to the userdata itself. This will ensure that if a script or the Lua Runtime is finished with an object, it will not be garbage collected. Do this only if you need long-term storage.

For all of these types, there's also a ``sol::stack_{x}`` version of them, such as ``sol::stack_table``. They are useful for a small performance boost at the cost of not having a strong reference, which has implications for what happens when the item is moved off of the stack. See :doc:`sol::stack_reference<stack_reference>` for more details.


members
-------

.. code-block:: cpp
	:caption: constructor: reference
	:name: reference-constructor

	reference(lua_State* L, int index = -1);
	reference(lua_State* L, lua_nil_t);
	reference(lua_State* L, absolute_index index);
	reference(lua_State* L, raw_index index);
	reference(lua_State* L, ref_index index);
	template <typename Object>
	reference(Object&& o);
	template <typename Object>
	reference(lua_State* L, Object&& o);

The first constructor creates a reference from the Lua stack at the specified index, saving it into the metatable registry. The second attemtps to register something that already exists in the registry. The third attempts to reference a pre-existing object and create a reference to it. These constructors are exposed on all types that derive from ``sol::reference``, meaning that you can grab tables, functions, and coroutines from the registry, stack, or from other objects easily.

.. _lua_xmove-note:

.. note::

	Note that the last constructor has ``lua_xmove`` safety built into it. You can pin an object to a certain thread (or the main thread) by initializing it with ``sol::reference pinned(state, other_reference_object);``. This ensures that ``other_reference_object`` will exist in the state/thread of ``state``. Also note that copy/move assignment operations will also use pinning semantics if it detects that the state of the object on the right is ``lua_xmove`` compatible. (But, the ``reference`` object on the left must have a valid state as well. You can have a nil ``reference`` with a valid state by using the ``sol::reference pinned(state, sol::lua_nil)`` constructor as well.) This applies for any ``sol::reference`` derived type.

	You can un-pin and null the state by doing ``ref = sol::lua_nil;``. This applies to **all derived types**, including ``sol::(protected_)function``, ``sol::thread``, ``sol::object``, ``sol::table``, and similar.

.. code-block:: cpp
	:caption: function: push referred-to element from the stack

	int push() const noexcept;

This function pushes the referred-to data onto the stack and returns how many things were pushed. Typically, it returns 1.

.. code-block:: cpp
	:caption: function: reference value

	int registry_index() const noexcept;

The value of the reference in the registry.

.. code-block:: cpp
	:caption: functions: non-nil, non-null check

	bool valid () const noexcept;
	explicit operator bool () const noexcept;

These functions check if the reference at ``T`` is valid: that is, if it is not :ref:`nil<nil>` and if it is not non-existing (doesn't refer to anything, including nil) reference. The explicit operator bool allows you to use it in the context of an ``if ( my_obj )`` context.

.. code-block:: cpp
	:caption: function: retrieves the type

	type get_type() const noexcept;

Gets the :doc:`sol::type<types>` of the reference; that is, the Lua reference.

.. code-block:: cpp
	:caption: function: lua_State* of the reference

	lua_State* lua_state() const noexcept;

Gets the ``lua_State*`` this reference exists in.


non-members
-----------

.. code-block:: cpp
	:caption: operators: reference comparators
	:name: reference-operators-comparators

	bool operator==(const reference&, const reference&);
	bool operator!=(const reference&, const reference&);

Compares two references using the Lua API's `lua_compare`_ for equality.


.. _lua_compare: https://www.lua.org/manual/5.3/manual.html#lua_compare
