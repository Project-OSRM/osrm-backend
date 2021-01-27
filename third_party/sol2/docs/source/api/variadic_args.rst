variadic_args
=============
*transparent argument to deal with multiple parameters to a function*


.. code-block:: cpp

	struct variadic_args;

This class is meant to represent every single argument at its current index and beyond in a function list. It does not increment the argument count and is thus transparent. You can place it anywhere in the argument list, and it will represent all of the objects in a function call that come after it, whether they are listed explicitly or not.

``variadic_args`` also has ``begin()`` and ``end()`` functions that return (almost) random-acess iterators. These return a proxy type that can be implicitly converted to a type you want, much like the :doc:`table proxy type<proxy>`.

.. literalinclude:: ../../../examples/variadic_args.cpp
	:linenos:

You can also "save" arguments and the like later, by stuffing them into a ``std::vector<sol::object>`` or something similar that serializes them into the registry. Below is an example of saving all of the arguments provided by ``sol::variadic_args`` in a lambda capture variable called ``args``.

.. literalinclude:: ../../../examples/variadic_args_storage.cpp
	:linenos:

Finally, note that you can use ``sol::variadic_args`` constructor to "offset"/"shift over" the arguments being viewed:

.. literalinclude:: ../../../examples/variadic_args_shifted.cpp
	:linenos:
