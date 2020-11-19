readonly
========
*routine to mark a member variable as read-only*

.. code-block:: cpp
	
	template <typename T>
	auto readonly( T&& value );

The goal of read-only is to protect a variable set on a usertype or a function. Simply wrap it around a member variable, e.g. ``sol::readonly( &my_class::my_member_variable )`` in the appropriate place to use it. If someone tries to set it, it will error their code.

``sol::readonly`` is especially important when you're working with types that do not have a copy constructor. Lua does not understand move semantics, and therefore setters to user-defined-types require a C++ copy constructor. Containers as member variables that contain types that are not copyable but movable -- e.g. ``std::vector<my_move_only_type>`` amongst others -- also can erroneously state they are copyable but fail with compiler errors. If your type does not fit a container's definition of being copyable or is just not copyable in general and it is a member variable, please use ``sol::readonly``.

If you are looking to make a read-only table, you need to go through a bit of a complicated song and dance by overriding the ``__index`` metamethod. Here's a complete example on the way to do that using ``sol``:


.. literalinclude:: ../../../examples/read_only.cpp
	:caption: read_only.cpp
	:linenos:

It is a verbose example, but it explains everything. Because the process is a bit involved and can have unexpected consequences for users that make their own tables, making read-only tables is something that we ask the users to do themselves with the above code, as getting the semantics right for the dozens of use cases would be tremendously difficult.
