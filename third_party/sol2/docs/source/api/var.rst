var
===
*For hooking up static / global variables to Lua usertypes*


The sole purpose of this tagging type is to work with :doc:`usertypes<usertype>` to provide ``my_class.my_static_var`` access, and to also provide reference-based access as well.

.. literalinclude:: ../../../examples/usertype_var.cpp
	:linenos:
