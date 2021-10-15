threading
=========

Lua has no thread safety. sol does not force thread safety bottlenecks anywhere. Treat access and object handling like you were dealing with a raw ``int`` reference (``int&``) (no safety or order guarantees whatsoever).

Assume any access or any call on Lua affects the whole ``sol::state``/``lua_State*`` (because it does, in a fair bit of cases). Therefore, every call to a state should be blocked off in C++ with some kind of access control (when you're working with multiple C++ threads). When you start hitting the same state from multiple threads, race conditions (data or instruction) can happen.

Individual Lua coroutines might be able to run on separate C++-created threads without tanking the state utterly, since each Lua coroutine has the capability to run on an independent Lua execution stack (Lua confusingly calls it a ``thread`` in the C API, but it really just means a separate execution stack) as well as some other associated bits and pieces that won't quite interfere with the global state.

To handle multithreaded environments, it is encouraged to either spawn a Lua state (``sol::state``) for each thread you are working with and keep inter-state communication to synchronized serialization points. This means that 3 C++ threads should each have their own Lua state, and access between them should be controlled using some kind of synchronized C++ mechanism (actual transfer between states must be done by serializing the value into C++ and then re-pushing it into the other state).

Using coroutines and Lua's threads might also buy you some concurrency and parallelism (**unconfirmed and likely untrue, do not gamble on this**), but remember that Lua's 'threading' technique is ultimately cooperative and requires explicit yielding and resuming (simplified as function calls for :doc:`sol::coroutine<api/coroutine>`).


getting the main thread
-----------------------

Lua 5.1 does not keep a reference to the main thread, therefore the user has to store it themselves. If you create a ``sol::state`` or follow the :ref:`steps for opening up compatibility and default handlers here<state-automatic-handlers>`, you can work with ``sol::main_thread`` to retrieve you the main thread, given a ``lua_State*`` that is either a full state or a thread: ``lua_state* Lmain = sol::main_thread( Lcoroutine )``; This function will always work in Lua 5.2 and above: in Lua 5.1, if you do not follow the ``sol::state`` instructions and do not pass a fallback ``lua_State*`` to the function, this function may not work properly and return ``nullptr``.

working with multiple Lua threads
---------------------------------

You can mitigate some of the pressure of using coroutines and threading by using the ``lua_xmove`` constructors that sol implements. Simply keep a reference to your ``sol::state_view`` or ``sol::state`` or the target ``lua_State*`` pointer, and pass it into the constructor along with the object you want to copy. Note that there is also some implicit ``lua_xmove`` checks that are done for copy and move assignment operators as well, as noted :ref:`at the reference constructor explanations<lua_xmove-note>`.

.. note::

	Advanced used: Furthermore, for every single ``sol::reference`` derived type, there exists a version prefixed with the word ``main_``, such as ``sol::main_table``, ``sol::main_function``, ``sol::main_object`` and similar. These classes, on construction, assignment and other operations, forcibly obtain the ``lua_State*`` associated with the main thread, if possible. Using these classes will allow your code to be immune when a wrapped coroutine or a lua thread is set to ``nil`` and then garbage-collected.


.. note::

	This does **not** provide immunity from typical multithreading issues in C++, such as synchronized access and the like. Lua's coroutines are cooperative in nature and concurrent execution with things like ``std::thread`` and similar still need to follow good C++ practices for multi threading.


Here's an example of explicit state transferring below:

.. literalinclude:: ../../examples/docs/state_transfer.cpp
	:name: state-transfer
	:linenos:
