usertypes
=========

Perhaps the most powerful feature of sol2, ``usertypes`` are the way sol2 and C++ communicate your classes to the Lua runtime and bind things between both tables and to specific blocks of C++ memory, allowing you to treat Lua userdata and other things like classes.

To learn more about usertypes, visit:

* :doc:`the basic tutorial<tutorial/cxx-in-lua>`
* :doc:`customization point tutorial<tutorial/customization>`
* :doc:`api documentation<api/usertype>`
* :doc:`memory documentation<api/usertype_memory>`

The examples folder also has a number of really great examples for you to see. There are also some notes about guarantees you can find about usertypes, and their associated userdata, below:

* Containers get pushed as special usertypes, but can be disabled if problems arise as detailed :doc:`here<containers>`.
* `Certain operators`_ are detected and bound automatically for usertypes
* You can use bitfields but it requires some finesse on your part. We have an example to help you get started `here, that uses a few tricks`_.
* All usertypes are runtime extensible in both `Lua`_ and `C++`_
	- If you need dynamic callbacks or runtime overridable functions, have a ``std::function`` member variable and get/set it on the usertype object
	- ``std::function`` works as a member variable or in passing as an argument / returning as a value (you can even use it with ``sol::property``)
	- You can also create an entirely dynamic object: see the `dynamic_object example`_ for more details
* You can use :doc:`filters<api/filters>` to control dependencies and streamline return values, as well as apply custom behavior to a functions return
* You can work with special wrapper types such as ``std::unique_ptr<T>``, ``std::shared_ptr<T>``, and others by default
    - Extend them using the :doc:`sol::unique_usertype\<T\> traits<api/unique_usertype_traits>`
    - This allows for custom smart pointers, special pointers, custom handles and others to be given certain handling semantics to ensure proper RAII with Lua's garbage collection
* (Advanced) You can override the iteration function for Lua 5.2 and above (LuaJIT does not have the capability) `as shown in the pairs example`_
* (Advanced) Interop with ``toLua``, ``kaguya``, ``OOLua``, ``LuaBind``, ``luwra``, and all other existing libraries by using the stack API's ``sol::stack::userdata_checker`` and ``sol::stack::userdata_getter`` :ref:`extension points<extension_points>`
    - Must turn on ``SOL_ENABLE_INTEROP``, as defined in the :ref:`configuration and safety documentation<config>`, to use

.. _usertype-special-features:
.. note::

	Note that to use many of sol2's features, such as automatic constructor creation, ``sol::property``, and similar, one must pass these things to the usertype as part of its initial creation and grouping of arguments. Attempting to do so afterwards will result in unexpected and wrong behavior, as the system will be missing information it needs. This is because many of these features rely on ``__index`` and ``__newindex`` Lua metamethods being overridden and handled in a special way!

Here are some other general advice and tips for understanding and dealing with usertypes:

* Please note that the colon is necessary to "automatically" pass the ``this``/``self`` argument to Lua methods
	- ``obj:method_name()`` is how you call "member" methods in Lua
	- It is purely syntactic sugar that passes the object name as the first argument to the ``method_name`` function
	- ``my_obj:foo(bar, baz)`` is the same as ``my_obj.foo(my_obj, bar, baz)``
	- **Please note** that one uses a colon, and the other uses a dot, and forgetting to do this properly will crash your code
	- There are safety defines outlined in the :ref:`safety page here<config>`
* You can push types classified as userdata before you register a usertype.
	- You can register a usertype with the Lua runtime at any time
	- You can retrieve a usertype from the Lua runtime at any time
	- Methods and properties will be added to the type only after you register the usertype with the Lua runtime
	- All methods and properties will appear on all userdata, even if that object was pushed before the usertype (all userdata will be updated)
* Types either copy once or move once into the memory location, if it is a value type. If it is a pointer, we store only the reference
	- This means retrieval of class types (not primitive types like strings or integers) by ``T&`` or ``T*`` allow you to modify the data in Lua directly
	- Retrieve a plain ``T`` to get a copy
	- Return types and passing arguments to ``sol::function``-types use perfect forwarding and reference semantics, which means no copies happen unless you specify a value explicitly. See :ref:`this note for details<function-argument-handling>`
*  You can set ``index`` and ``new_index`` freely on any usertype you like to override the default "if a key is missing, find it / set it here" functionality of a specific object of a usertype
	- ``new_index`` and ``index`` will not be called if you try to manipulate the named usertype table directly. sol2's will be called to add that function to the usertype's function/variable lookup table
	- ``new_index`` and ``index`` will be called if you attempt to call a key that does not exist on an actual userdata object (the C++ object) itself
	- If you made a usertype named ``test``, this means ``t = test()``, with ``t.hi = 54`` will call your function, but ``test.hi = function () print ("hi"); end`` will instead set the key ``hi`` to to lookup that function for all ``test`` types
* The first ``sizeof( void* )`` bytes is always a pointer to the typed C++ memory. What comes after is based on what you've pushed into the system according to :doc:`the memory specification for usertypes<api/usertype_memory>`. This is compatible with a number of systems other than just sol2, making it easy to interop with select other Lua systems.
* Member methods, properties, variables and functions taking ``self&`` arguments modify data directly
	- Work on a copy by taking arguments or returning by value.
	- Do not use r-value references: they do not mean anything in Lua code.
	- Move-only types can only be taken by reference: sol2 cannot know if/when to move a value (except when serializing with perfect forwarding *into* Lua, but not calling a C++ function from Lua)
* The actual metatable associated with the usertype has a long name and is defined to be opaque by the Sol implementation.
* The actual metatable inner workings is opaque and defined by the Sol implementation, and there are no internal docs because optimizations on the operations are applied based on heuristics we discover from performance testing the system.

.. _here, that uses a few tricks: https://github.com/ThePhD/sol2/blob/develop/examples/usertype_bitfields.cpp
.. _Lua: https://github.com/ThePhD/sol2/blob/develop/examples/usertype_advanced.cpp#L81
.. _C++: https://github.com/ThePhD/sol2/blob/develop/examples/usertype_simple.cpp#L51
.. _Certain operators: https://github.com/ThePhD/sol2/blob/develop/examples/usertype_automatic_operators.cpp
.. _dynamic_object example: https://github.com/ThePhD/sol2/blob/develop/examples/dynamic_object.cpp
.. _as shown in the pairs example: https://github.com/ThePhD/sol2/blob/develop/examples/pairs.cpp
