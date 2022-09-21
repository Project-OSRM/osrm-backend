variadic_results
================
*push multiple disparate arguments into lua*

.. code-block:: cpp
	
	struct variadic_results : std::vector<object> { ... };

This type allows someone to prepare multiple returns before returning them into Lua. It derives from ``std::vector``, so it can be used exactly like that, and objects can be added using the various constructors and functions relating to :doc:`sol::object<object>`. You can see it and other return-type helpers in action `here`_.

.. _here: https://github.com/ThePhD/sol2/blob/develop/examples/multi_results.cpp
