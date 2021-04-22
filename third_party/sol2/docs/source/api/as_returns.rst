as_returns
==========
*turn an iterable argument into a multiple-return type*


.. code-block:: cpp
	
	template <typename T>
	struct as_returns_t { ... };

	template <typename T>
	as_returns_t<T> as_returns( T&& );


This allows you to wrap up a source that has ``begin`` and ``end`` iterator-returning functions on it and return it as multiple results into Lua. To have more control over the returns, use :doc:`sol::variadic_results<variadic_results>`.


.. literalinclude:: ../../../examples/as_returns.cpp
	:linenos:
