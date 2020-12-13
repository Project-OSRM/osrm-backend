as_container
============
*force a type to be viewed as a container-type when serialized to Lua*

.. code-block:: cpp
	
	template <typename T>
	struct as_returns_t { ... };

	template <typename T>
	as_returns_t<T> as_returns( T&& );


Sometimes, you have a type whose metatable you claim with a usertype metatable via :doc:`usertype semantics<usertype>`. But, it still has parts of it that make it behave like a container in C++: A ``value_type`` typedef, an ``iterator`` typedef, a ``begin``, an ``end``, and other things that satisfy the `Container requirements`_ or the `Sequence Container requirements`_ or behaves like a `forward_list`_.

Whatever the case is, you need it to be returned to Lua and have many of the traits and functionality described in the :doc:`containers documentation<../containers>`. Wrap a return type or a setter in ``sol::as_container( value );`` to allow for a type to be treated like a container, regardless of whether ``sol::is_container`` triggers or not.

See `this container example`_ to see how it works.

.. _this container example: https://github.com/ThePhD/sol2/blob/develop/examples/container_usertype_as_container.cpp
.. _Container requirements: http://en.cppreference.com/w/cpp/concept/Container
.. _Sequence Container requirements: http://en.cppreference.com/w/cpp/concept/SequenceContainer
.. _forward_list: http://en.cppreference.com/w/cpp/container/forward_list
