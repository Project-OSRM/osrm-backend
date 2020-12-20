property
========
*wrapper to specify read and write variable functionality using functions*

.. code-block:: cpp
	
	template <typename Read, typename Write>
	decltype(auto) property ( Read&& read_function, Write&& write_function );
	template <typename Read>
	decltype(auto) property ( Read&& read_function );
	template <typename Write>
	decltype(auto) property ( Write&& write_function );

These set of functions create a type which allows a setter and getter pair (or a single getter, or a single setter) to be used to create a variable that is either read-write, read-only, or write-only. When used during :doc:`usertype<usertype>` construction, it will create a variable that uses the setter/getter member function specified.

.. literalinclude:: ../../../examples/property.cpp
	:linenos:

