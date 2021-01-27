protect
=======
*routine to mark a function / variable as requiring safety*


.. code-block:: cpp
	
	template <typename T>
	auto protect( T&& value );

``sol::protect( my_func )`` allows you to protect a function call or member variable call when it is being set to Lua. It can be used with usertypes or when just setting a function into Sol. Below is an example that demonstrates that a call that would normally not error without :doc:`Safety features turned on<../safety>` that instead errors and makes the Lua safety-call wrapper ``pcall`` fail:

.. literalinclude:: ../../../examples/protect.cpp
	:linenos:	
