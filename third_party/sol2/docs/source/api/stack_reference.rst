stack_reference
===============
*zero-overhead object on the stack*


When you work with a :doc:`sol::reference<reference>`, the object gotten from the stack has a reference to it made in the registry, keeping it alive. If you want to work with the Lua stack directly without having any additional references made, ``sol::stack_reference`` is for you. Its API is identical to ``sol::reference`` in every way, except it contains a ``int stack_index()`` member function that allows you to retrieve the stack index.

Note that this will not pin the object since a copy is not made in the registry, meaning things can be pulled out from under it, the stack can shrink under it, things can be added onto the stack before this object's position, and what ``sol::stack_reference`` will point to will change. Please know what the Lua stack is and have discipline while managing your Lua stack when working at this level.

All of the base types have ``stack`` versions of themselves, and the APIs are identical to their non-stack forms. This includes :doc:`sol::stack_table<table>`, :doc:`sol::stack_function<function>`, :doc:`sol::stack_protected_function<protected_function>`, :doc:`sol::stack_(light\_)userdata<userdata>` and :doc:`sol::stack_object<object>`. There is a special case for ``sol::stack_function``, which has an extra type called ``sol::stack_aligned_function`` (and similar ``sol::stack_aligned_protected_function``).


stack_aligned_function
----------------------

This type is particular to working with the stack. It does not push the function object on the stack before pushing the arguments, assuming that the function present is already on the stack before going ahead and invoking the function it is targeted at. It is identical to :doc:`sol::function<function>` and has a protected counterpart as well. If you are working with the stack and know there is a callable object in the right place (i.e., at the top of the Lua stack), use this abstraction to have it call your stack-based function while still having the easy-to-use Lua abstractions.

Furthermore, if you know you have a function in the right place alongside proper arguments on top of it, you can use the ``sol::stack_count`` structure and give its constructor the number of arguments off the top that you want to call your pre-prepared function with:

.. literalinclude:: ../../../examples/stack_aligned_function.cpp
	:caption: stack_aligned_function.cpp
	:linenos:
	:name: stack-aligned-function-example

Finally, there is a special abstraction that provides further stack optimizations for ``sol::protected_function`` variants that are aligned, and it is called ``sol::stack_aligned_stack_handler_protected_function``. This typedef expects you to pass a ``stack_reference`` handler to its constructor, meaning that you have already placed an appropriate error-handling function somewhere on the stack before the aligned function. You can use ``sol::stack_count`` with this type as well.

.. warning::

	Do not use ``sol::stack_count`` with a ``sol::stack_aligned_protected_function``. The default behavior checks if the ``error_handler`` member variable is valid, and attempts to push the handler onto the stack in preparation for calling the function. This inevitably changes the stack. Only use ``sol::stack_aligned_protected_function`` with ``sol::stack_count`` if you know that the handler is not valid (it is ``nil`` or its ``error_handler.valid()`` function returns ``false``), or if you use ``sol::stack_aligned_stack_handler_protected_function``, which references an existing stack index that can be before the precise placement of the function and its arguments.
