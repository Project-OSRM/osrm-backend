stack namespace
===============
*the nitty-gritty core abstraction layer over Lua*


.. code-block:: cpp

	namespace stack

If you find that the higher level abstractions are not meeting your needs, you may want to delve into the ``stack`` namespace to try and get more out of Sol. ``stack.hpp`` and the ``stack`` namespace define several utilities to work with Lua, including pushing / popping utilities, getters, type checkers, Lua call helpers and more. This namespace is not thoroughly documented as the majority of its interface is mercurial and subject to change between releases to either heavily boost performance or improve the Sol :doc:`api<api-top>`.

Working at this level of the stack can be enhanced by understanding how the `Lua stack works in general`_ and then supplementing it with the objects and items here.

There are, however, a few :ref:`template customization points<extension_points>` that you may use for your purposes and a handful of potentially handy functions. These may help if you're trying to slim down the code you have to write, or if you want to make your types behave differently throughout the Sol stack. Note that overriding the defaults **can** throw out many of the safety guarantees Sol provides: therefore, modify the :ref:`extension points<extension_points>` at your own discretion.

structures
----------

.. code-block:: cpp
	:caption: struct: record
	:name: stack-record

	struct record {
		int last;
		int used;

		void use(int count);
	};

This structure is for advanced usage with :ref:`stack::get<stack-get>` and :ref:`stack::check_get<stack-get>`. When overriding the customization points, it is important to call the ``use`` member function on this class with the amount of things you are pulling from the stack. ``used`` contains the total accumulation of items produced. ``last`` is the number of items gotten from the stack with the last operation (not necessarily popped from the stack). In all trivial cases for types, ``last == 1`` and ``used == 1`` after an operation; structures such as ``std::pair`` and ``std::tuple`` may pull more depending on the classes it contains.

When overriding the :doc:`customization points<../tutorial/customization>`, please note that this structure should enable you to push multiple return values and get multiple return values to the stack, and thus be able to seamlessly pack/unpack return values from Lua into a single C++ struct, and vice-versa. This functionality is only recommended for people who need to customize the library further than the basics. It is also a good way to add support for the type and propose it back to the original library so that others may benefit from your work.

Note that customizations can also be put up on a separate page here, if individuals decide to make in-depth custom ones for their framework or other places.

.. code-block:: cpp
	:caption: struct: probe
	:name: stack-probe-struct

	struct probe {
		bool success;
		int levels;

		probe(bool s, int l);
		operator bool() const;
	};

This struct is used for showing whether or not a :ref:`probing get_field<stack-probe-get-field>` was successful or not.


members
-------

.. code-block:: cpp
	:caption: function: call_lua
	:name: stack-call-lua

	template<bool check_args = stack_detail::default_check_arguments, bool clean_stack = true, typename Fx, typename... FxArgs>
	inline int call_lua(lua_State* L, int start, Fx&& fx, FxArgs&&... fxargs);

This function is helpful for when you bind to a raw C function but need sol's abstractions to save you the agony of setting up arguments and know how `calling C functions works`_. The ``start`` parameter tells the function where to start pulling arguments from. The parameter ``fx``  is what's supposed to be called. Extra arguments are passed to the function directly. There are intermediate versions of this (``sol::stack::call_into_lua`` and similar) for more advanced users, but they are not documented as they are subject to change to improve performance or adjust the API accordingly in later iterations of sol2. Use the more advanced versions at your own peril.

.. code-block:: cpp
	:caption: function: get
	:name: stack-get

	template <typename T>
	auto get( lua_State* L, int index = -1 )
	template <typename T>
	auto get( lua_State* L, int index, record& tracking )

Retrieves the value of the object at ``index`` in the stack. The return type varies based on ``T``: with primitive types, it is usually ``T``: for all unrecognized ``T``, it is generally a ``T&`` or whatever the extension point :ref:`stack::getter\<T><getter>` implementation returns. The type ``T`` has top-level ``const`` qualifiers and reference modifiers removed before being forwarded to the extension point :ref:`stack::getter\<T><getter>` struct. ``stack::get`` will default to forwarding all arguments to the :ref:`stack::check_get<stack-check-get>` function with a handler of ``type_panic`` to strongly alert for errors, if you ask for the :doc:`safety<../safety>`.

You may also retrieve an :doc:`sol::optional\<T><optional>` from this as well, to have it attempt to not throw errors when performing the get and the type is not correct.

.. code-block:: cpp
	:caption: function: check
	:name: stack-check

	template <typename T>
	bool check( lua_State* L, int index = -1 )

	template <typename T, typename Handler>
	bool check( lua_State* L, int index, Handler&& handler )

	template <typename T, typename Handler>
	bool check( lua_State* L, int index, Handler&& handler, record& tracking )

Checks if the object at ``index`` is of type ``T``. If it is not, it will call the ``handler`` function with ``lua_State* L``, ``int index``, ``sol::type expected``, and ``sol::type actual`` as arguments (and optionally with a 5th string argument ``sol::string_view message``. If you do not pass your own handler, a ``no_panic`` handler will be passed.

.. code-block:: cpp
	:caption: function: get_usertype
	:name: stack-get-usertype

	template <typename T>
	auto get_usertype( lua_State* L, int index = -1 )
	template <typename T>
	auto get_usertype( lua_State* L, int index, record& tracking )

Directly attempts to rertieve the type ``T`` using sol2's usertype mechanisms. Similar to a regular ``get`` for a user-defined type. Useful when you need to access sol2's usertype getter mechanism while at the same time `providing your own customization`_.

.. code-block:: cpp
	:caption: function: check_usertype
	:name: stack-check-usertype

	template <typename T>
	bool check_usertype( lua_State* L, int index = -1 )

	template <typename T, typename Handler>
	bool check_usertype( lua_State* L, int index, Handler&& handler )

	template <typename T, typename Handler>
	bool check_usertype( lua_State* L, int index, Handler&& handler, record& tracking )

Checks if the object at ``index`` is of type ``T`` and stored as a sol2 usertype. Useful when you need to access sol2's usertype checker mechanism while at the same time `providing your own customization`_.

.. code-block:: cpp
	:caption: function: check_get
	:name: stack-check-get

	template <typename T>
	auto check_get( lua_State* L, int index = -1 )
	template <typename T, typename Handler>
	auto check_get( lua_State* L, int index, Handler&& handler, record& tracking )

Retrieves the value of the object at ``index`` in the stack, but does so safely. It returns an ``optional<U>``, where ``U`` in this case is the return type deduced from ``stack::get<T>``. This allows a person to properly check if the type they're getting is what they actually want, and gracefully handle errors when working with the stack if they so choose to. You can define ``SOL_CHECK_ARGUMENTS`` to turn on additional :doc:`safety<../safety>`, in which ``stack::get`` will default to calling this version of the function with some variant on a handler of ``sol::type_panic_string`` to strongly alert for errors and help you track bugs if you suspect something might be going wrong in your system.

.. code-block:: cpp
	:caption: function: push
	:name: stack-push

	// push T inferred from call site, pass args... through to extension point
	template <typename T, typename... Args>
	int push( lua_State* L, T&& item, Args&&... args )

	// push T that is explicitly specified, pass args... through to extension point
	template <typename T, typename Arg, typename... Args>
	int push( lua_State* L, Arg&& arg, Args&&... args )

	// recursively call the the above "push" with T inferred, one for each argument
	template <typename... Args>
	int multi_push( lua_State* L, Args&&... args )

Based on how it is called, pushes a variable amount of objects onto the stack. in 99% of cases, returns for 1 object pushed onto the stack. For the case of a ``std::tuple<...>``, it recursively pushes each object contained inside the tuple, from left to right, resulting in a variable number of things pushed onto the stack (this enables multi-valued returns when binding a C++ function to a Lua). Can be called with ``sol::stack::push<T>( L, args... )`` to have arguments different from the type that wants to be pushed, or ``sol::stack::push( L, arg, args... )`` where ``T`` will be inferred from ``arg``. The final form of this function is ``sol::stack::multi_push``, which will call one ``sol::stack::push`` for each argument. The ``T`` that describes what to push is first sanitized by removing top-level ``const`` qualifiers and reference qualifiers before being forwarded to the extension point :ref:`stack::pusher\<T><pusher>` struct.

.. code-block:: cpp
	:caption: function: push_reference
	:name: stack-push-reference

	// push T inferred from call site, pass args... through to extension point
	template <typename T, typename... Args>
	int push_reference( lua_State* L, T&& item, Args&&... args )

	// push T that is explicitly specified, pass args... through to extension point
	template <typename T, typename Arg, typename... Args>
	int push_reference( lua_State* L, Arg&& arg, Args&&... args )

	// recursively call the the above "push" with T inferred, one for each argument
	template <typename... Args>
	int multi_push_reference( lua_State* L, Args&&... args )


These functinos behave similarly to the ones above, but they check for specific criteria and instead attempt to push a reference rather than forcing a copy if appropriate. Use cautiously as sol2 uses this mainly as a return from usertype functions and variables to preserve chaining/variable semantics from that a class object. Its internals are updated to fit the needs of sol2 and while it generally does the "right thing" and has not needed to be changed for a while, sol2 reserves the right to change its internal detection mechanisms to suit its users needs at any time, generally without breaking backwards compatibility and expectations but not exactly guaranteed.

.. code-block:: cpp
	:caption: function: pop
	:name: stack-pop

	template <typename... Args>
	auto pop( lua_State* L );

Pops an object off the stack. Will remove a fixed number of objects off the stack, generally determined by the ``sol::lua_size<T>`` traits of the arguments supplied. Generally a simplicity function, used for convenience.

.. code-block:: cpp
	:caption: function: top
	:name: stack-top

	int top( lua_State* L );

Returns the number of values on the stack.

.. code-block:: cpp
	:caption: function: set_field

	template <bool global = false, typename Key, typename Value>
	void set_field( lua_State* L, Key&& k, Value&& v );

	template <bool global = false, typename Key, typename Value>
	void set_field( lua_State* L, Key&& k, Value&& v, int objectindex);

Sets the field referenced by the key ``k`` to the given value ``v``, by pushing the key onto the stack, pushing the value onto the stack, and then doing the equivalent of ``lua_setfield`` for the object at the given ``objectindex``. Performs optimizations and calls faster verions of the function if the type of ``Key`` is considered a c-style string and/or if its also marked by the templated ``global`` argument to be a global.

.. code-block:: cpp
	:caption: function: get_field

	template <bool global = false, typename Key>
	void get_field( lua_State* L, Key&& k [, int objectindex] );

Gets the field referenced by the key ``k``, by pushing the key onto the stack, and then doing the equivalent of ``lua_getfield``. Performs optimizations and calls faster verions of the function if the type of ``Key`` is considered a c-style string and/or if its also marked by the templated ``global`` argument to be a global.

This function leaves the retrieved value on the stack.

.. code-block:: cpp
	:caption: function: probe_get_field
	:name: stack-probe-get-field

	template <bool global = false, typename Key>
	probe probe_get_field( lua_State* L, Key&& k [, int objectindex] );

Gets the field referenced by the key ``k``, by pushing the key onto the stack, and then doing the equivalent of ``lua_getfield``. Performs optimizations and calls faster verions of the function if the type of ``Key`` is considered a c-style string and/or if its also marked by the templated ``global`` argument to be a global. Furthermore, it does this safely by only going in as many levels deep as is possible: if the returned value is not something that can be indexed into, then traversal queries with ``std::tuple``/``std::pair`` will stop early and return probing information with the :ref:`probe struct<stack-probe-struct>`.

This function leaves the retrieved value on the stack.

.. _extension_points:

objects (extension points)
--------------------------

You can customize the way Sol handles different structures and classes by following the information provided in the :doc:`adding your own types<../tutorial/customization>`.

Below is more extensive information for the curious.

The structs below are already overriden for a handful of types. If you try to mess with them for the types ``sol`` has already overriden them for, you're in for a world of thick template error traces and headaches. Overriding them for your own user defined types should be just fine, however.

.. code-block:: cpp
	:caption: struct: getter
	:name: getter

	template <typename T, typename = void>
	struct getter {
		static T get (lua_State* L, int index, record& tracking) {
			// ...
			return // T, or something related to T.
		}
	};

This is an SFINAE-friendly struct that is meant to expose static function ``get`` that returns a ``T``, or something convertible to it. The default implementation assumes ``T`` is a usertype and pulls out a userdata from Lua before attempting to cast it to the desired ``T``. There are implementations for getting numbers (``std::is_floating``, ``std::is_integral``-matching types), getting ``std::string`` and ``const char*``, getting raw userdata with :doc:`userdata_value<types>` and anything as upvalues with :doc:`upvalue_index<types>`, getting raw `lua_CFunction`_ s, and finally pulling out Lua functions into ``std::function<R(Args...)>``. It is also defined for anything that derives from :doc:`sol::reference<reference>`. It also has a special implementation for the 2 standard library smart pointers (see :doc:`usertype memory<usertype_memory>`).

.. code-block:: cpp
	:caption: struct: pusher
	:name: pusher

	template <typename X, typename = void>
	struct pusher {
		template <typename T>
		static int push ( lua_State* L, T&&, ... ) {
			// can optionally take more than just 1 argument
			// ...
			return // number of things pushed onto the stack
		}
	};

This is an SFINAE-friendly struct that is meant to expose static function ``push`` that returns the number of things pushed onto the stack. The default implementation assumes ``T`` is a usertype and pushes a userdata into Lua with a class-specific, state-wide metatable associated with it. There are implementations for pushing numbers (``std::is_floating``, ``std::is_integral``-matching types), getting ``std::string`` and ``const char*``, getting raw userdata with :doc:`userdata<types>` and raw upvalues with :doc:`upvalue<types>`, getting raw `lua_CFunction`_ s, and finally pulling out Lua functions into ``sol::function``. It is also defined for anything that derives from :doc:`sol::reference<reference>`. It also has a special implementation for the 2 standard library smart pointers (see :doc:`usertype memory<usertype_memory>`).

.. code-block:: cpp
	:caption: struct: checker
	:name: checker

	template <typename T, type expected = lua_type_of<T>, typename = void>
	struct checker {
		template <typename Handler>
		static bool check ( lua_State* L, int index, Handler&& handler, record& tracking ) {
			// if the object in the Lua stack at index is a T, return true
			if ( ... ) { 
				tracking.use(1); // or however many you use
				return true;
			}
			// otherwise, call the handler function,
			// with the required 4/5 arguments, then return false
			//
			handler(L, index, expected, indextype, "optional message");
			return false;
		}
	};

This is an SFINAE-friendly struct that is meant to expose static function ``check`` that returns whether or not a type at a given index is what its supposed to be. The default implementation simply checks whether the expected type passed in through the template is equal to the type of the object at the specified index in the Lua stack. The default implementation for types which are considered ``userdata`` go through a myriad of checks to support checking if a type is *actually* of type ``T`` or if its the base class of what it actually stored as a userdata in that index. Down-casting from a base class to a more derived type is, unfortunately, impossible to do.

.. _userdata-interop:

.. code-block:: cpp
	:caption: struct: userdata_checker
	:name: userdata_checker

	template <typename T, typename = void>
	struct userdata_checker {
		template <typename Handler>
		static bool check ( lua_State* L, int index, type indextype, Handler&& handler, record& tracking ) {
			// implement custom checking here for a userdata:
			// if it doesn't match, return "false" and regular 
			// sol userdata checks will kick in
			return false;
			// returning true will skip sol's
			// default checks
		}
	};

This is an SFINAE-friendly struct that is meant to expose a function ``check=`` that returns ``true`` if a type meets some custom userdata specifiction, and ``false`` if it does not. The default implementation just returns ``false`` to let the original sol2 handlers take care of everything. If you want to implement your own usertype checking; e.g., for messing with ``toLua`` or ``OOLua`` or ``kaguya`` or some other libraries. Note that the library must have a with a :doc:`memory compatible layout<usertype_memory>` if you **want to specialize this checker method but not the subsequent getter method**. You can specialize it as shown in the `interop examples`_.

.. note::

	You must turn this feature on with ``SOL_ENABLE_INTEROP``, as described in the :ref:`config and safety section<config>`.


.. code-block:: cpp
	:caption: struct: userdata_getter
	:name: userdata_getter

	template <typename T, typename = void>
	struct userdata_getter {
		static std::pair<bool, T*> get ( lua_State* L, int index, void* unadjusted_pointer, record& tracking ) {
			// implement custom getting here for non-sol2 userdatas:
			// if it doesn't match, return "false" and regular 
			// sol userdata checks will kick in
			return { false, nullptr };
		}
	};

This is an SFINAE-friendly struct that is meant to expose a function ``get`` that returns ``true`` and an adjusted pointer if a type meets some custom userdata specifiction (from, say, another library or an internal framework). The default implementation just returns ``{ false, nullptr }`` to let the original sol2 getter take care of everything. If you want to implement your own usertype getter; e.g., for messing with ``kaguya`` or some other libraries. You can specialize it as shown in the `interop examples`_.

.. note::

	You do NOT need to use this method in particular if the :doc:`memory layout<usertype_memory>` is compatible. (For example, ``toLua`` stores userdata in a sol2-compatible way.)


.. note::

	You must turn it on with ``SOL_ENABLE_INTEROP``, as described in the :ref:`config and safety section<config>`.


.. _lua_CFunction: http://www.Lua.org/manual/5.3/manual.html#lua_CFunction
.. _Lua stack works in general: https://www.lua.org/pil/24.2.html
.. _calling C functions works: https://www.lua.org/pil/26.html
.. _interop examples: https://github.com/ThePhD/sol2/blob/develop/examples/interop
.. _providing your own customization: https://github.com/ThePhD/sol2/blob/develop/examples/customization_convert_on_get.cpp
