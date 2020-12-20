as_function
===========
*make sure an object is pushed as a function*


.. code-block:: cpp
	
	template <typename Sig = sol::function_sig<>, typename... Args>
	function_argumants<Sig, Args...> as_function ( Args&& ... );

This function serves the purpose of ensuring that a callable struct (like a lambda) can be passed to the ``set( key, value )`` calls on :ref:`sol::table<set-value>` and be treated like a function binding instead of a userdata. It is recommended that one uses the :ref:`sol::table::set_function<set-function>` call instead, but if for some reason one must use ``set``, then ``as_function`` can help ensure a callable struct is handled like a lambda / callable, and not as just a userdata structure.

This class can also make it so usertypes bind variable types as functions to for usertype bindings.

.. literalinclude:: ../../../examples/docs/as_function.cpp
	:linenos:


Note that if you actually want a userdata, but you want it to be callable, you simply need to create a :ref:`sol::table::new_usertype<new-usertype>` and then bind the ``"__call"`` metamethod (or just use ``sol::meta_function::call`` :ref:`enumeration<meta_function_enum>`). This may or may not be done automatically for you, depending on whether or not the call operator is overloaded and such.

Here's an example of binding a variable as a function to a usertype:

.. literalinclude:: ../../../examples/docs/as_function_usertype_member_variable.cpp
	:linenos:
