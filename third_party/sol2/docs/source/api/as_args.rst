as_args
=======
*turn an iterable argument into multiple arguments*


.. code-block:: cpp
	
	template <typename T>
	struct as_args_t { ... };

	template <typename T>
	as_args_t<T> as_args( T&& );


``sol::as_args`` is a function that that takes an iterable and turns it into multiple arguments to a function call. It forwards its arguments, and is meant to be used as shown below:


.. literalinclude:: ../../../examples/args_from_container.cpp
	:caption: args_from_container.cpp
	:linenos:

It is basically implemented as a `one-way customization point`_. For more information about customization points, see the :doc:`tutorial on how to customize Sol to work with your types<../tutorial/customization>`.

.. _one-way customization point: https://github.com/ThePhD/sol2/blob/develop/sol/as_args.hpp
