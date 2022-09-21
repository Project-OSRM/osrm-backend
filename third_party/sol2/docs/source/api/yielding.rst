yielding
========
*telling a C++ function to yield its results into Lua*

.. code-block:: cpp

	template <typename F>
	yield_wrapper<F> yielding( F&& f )

``sol::yielding`` is useful for calling C++ functions which need to yield into a Lua coroutine. It is a wrapper around a single argument which is expected to be bound as a function. You can pass it anywhere a regular function can be bound, **except for in usertype definitions**.
