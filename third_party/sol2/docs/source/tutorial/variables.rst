variables
=========

Working with variables is easy with sol, and behaves pretty much like any associative array / map structure you might have dealt with previously.

reading
-------

Given this lua file that gets loaded into sol:

.. literalinclude:: ../../../examples/tutorials/variables_demo.cpp
	:linenos:
	:lines: 15-18

You can interact with the Lua Virtual Machine like so:


.. literalinclude:: ../../../examples/tutorials/variables_demo.cpp
	:linenos:
	:lines: 1-10, 12-12, 20-24, 70-

From this example, you can see that there's many ways to pull out the varaibles you want. For example, to determine if a nested variable exists or not, you can use ``auto`` to capture the value of a ``table[key]`` lookup, and then use the ``.valid()`` method:


.. literalinclude:: ../../../examples/tutorials/variables_demo.cpp
	:linenos:
	:lines: 1-10, 12-12, 34-43, 70-


This comes in handy when you want to check if a nested variable exists. You can also check if a toplevel variable is present or not by using ``sol::optional``, which also checks if A) the keys you're going into exist and B) the type you're trying to get is of a specific type:

.. literalinclude:: ../../../examples/tutorials/variables_demo.cpp
	:linenos:
	:caption: optional lookup
	:lines: 1-10, 12-12, 43-58, 70-


This can come in handy when, even in optimized or release modes, you still want the safety of checking.  You can also use the `get_or` methods to, if a certain value may be present but you just want to default the value to something else:

.. literalinclude:: ../../../examples/tutorials/variables_demo.cpp
	:linenos:
	:caption: optional lookup
	:lines: 1-10, 12-12, 60-


That's all it takes to read variables!


writing
-------

Writing gets a lot simpler. Even without scripting a file or a string, you can read and write variables into lua as you please:

.. literalinclude:: ../../../examples/tutorials/write_variables_demo.cpp
	:linenos:
	:name: writing-variables-demo

This example pretty much sums up what can be done. Note that the syntax ``lua["non_existing_key_1"] = 1`` will make that variable, but if you tunnel too deep without first creating a table, the Lua API will panic (e.g., ``lua["does_not_exist"]["b"] = 20`` will trigger a panic). You can also be lazy with reading / writing values:

.. literalinclude:: ../../../examples/tutorials/lazy_demo.cpp
	:linenos:


Finally, it's possible to erase a reference/variable by setting it to ``nil``, using the constant ``sol::nil`` in C++:

.. literalinclude:: ../../../examples/tutorials/erase_demo.cpp
	:linenos:


It's easy to see that there's a lot of options to do what you want here. But, these are just traditional numbers and strings. What if we want more power, more capabilities than what these limited types can offer us? Let's throw some :doc:`functions in there<functions>` :doc:`C++ classes into the mix<cxx-in-lua>`!
