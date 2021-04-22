this_state
==========
*transparent state argument for the current state*


.. code-block:: cpp
	
	struct this_state;

This class is a transparent type that is meant to be gotten in functions to get the current lua state a bound function or usertype method is being called from. It does not actually retrieve anything from lua nor does it increment the argument count, making it "invisible" to function calls in lua and calls through ``std::function<...>`` and :doc:`sol::function<function>` on this type. It can be put in any position in the argument list of a function:

.. literalinclude:: ../../../examples/this_state.cpp
	:linenos:
