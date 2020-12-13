benchmarks
==========
because somebody is going to ask eventually...
----------------------------------------------


Here are measurements of the *overhead that libraries impose around the Lua C API*: that is, the cost of abstracting / wrapping the plain Lua C API. Measurements are (at the time of writing) done with all libraries compiled against a DLL version of Lua 5.3.3 to make sure each Lua call has the same overhead between libraries (no Link Time Optimizations or other static library optimizations).

These are some informal and formal benchmarks done by both the developers of sol and other library developers / users. We leave you to interpret the data as you see fit.

* `lua_binding_benchmarks`_ by satoren (developer of `kaguya`_) (`sol`_ is the "sol2" entry)
* `lua-bindings-shootout`_ by ThePhD (developer of `sol`_)

As of the writing of this documentation (May 17th, 2018), :doc:`sol<index>` seems to take the cake in most categories for speed! Below are some graphs from `lua-bindings-shootout`_. You can read the benchmarking code there if you think something was done wrong, and submit a pull requests or comment on something to make sure that ThePhD is being honest about his work. All categories are the performance of things described at the top of the :doc:`feature table<features>`.

Note that sol here makes use of its more performant variants (see :doc:`c_call<api/c_call>` and others), and ThePhD also does his best to make use of the most performant variants for other frameworks by disabling type checks where possible as well (Thanks to Liam Devine of OOLua for explaining how to turn off type checks in OOLua).

Bars go up to the average execution time. Lower is better. Reported times are for the desired operation run through `nonius`_. Results are sorted from top to bottom by best to worst. Note that there are error bars to show potential variance in performance: generally, same-sized errors bars plus very close average execution time implies no significant difference in speed, despite the vastly different abstraction techniques used.

.. image:: /media/bench/member_function_call.png
	:target: https://raw.githubusercontent.com/ThePhD/lua-bindings-shootout/master/benchmark_results/member%20function%20call.png
	:alt: bind several member functions to an object and call them in Lua code

.. image:: /media/bench/userdata_variable_access.png
	:target: https://raw.githubusercontent.com/ThePhD/lua-bindings-shootout/master/benchmark_results/userdata%20variable%20access.png
	:alt: bind a variable to an object and call it in Lua code

.. image:: /media/bench/userdata_variable_access_large.png
	:target: https://raw.githubusercontent.com/ThePhD/lua-bindings-shootout/master/benchmark_results/userdata%20variable%20access%20large.png
	:alt: bind MANY variables to an object and access them in Lua code

.. image:: /media/bench/c_function_through_lua_in_c.png
	:target: https://raw.githubusercontent.com/ThePhD/lua-bindings-shootout/master/benchmark_results/c%20function%20through%20lua%20in%20c.png
	:alt: retrieve a C function bound in Lua and call it from C++

.. image:: /media/bench/stateful_function_object.png
	:target: https://raw.githubusercontent.com/ThePhD/lua-bindings-shootout/master/benchmark_results/stateful%20function%20object.png
	:alt: bind a stateful C function (e.g., a mutable lambda), retrieve it, and call it from C++

.. image:: /media/bench/c_function.png
	:target: https://raw.githubusercontent.com/ThePhD/lua-bindings-shootout/master/benchmark_results/c%20function.png
	:alt: call a C function through Lua code

.. image:: /media/bench/lua_function_in_c.png
	:target: https://raw.githubusercontent.com/ThePhD/lua-bindings-shootout/master/benchmark_results/lua%20function%20in%20c.png
	:alt: retrieve a plain Lua function and call it from C++

.. image:: /media/bench/multi_return.png
	:target: https://raw.githubusercontent.com/ThePhD/lua-bindings-shootout/master/benchmark_results/multi%20return.png
	:alt: return mutliple objects from C++ using std::tuple or through a library-defined mechanism

.. image:: /media/bench/multi_return_lua.png
	:target: https://raw.githubusercontent.com/ThePhD/lua-bindings-shootout/master/benchmark_results/multi%20return%20lua.png
	:alt: return mutliple objects from C++ using std::tuple or through a library-defined mechanism and use it in Lua

.. image:: /media/bench/table_global_string_get.png
	:target: https://raw.githubusercontent.com/ThePhD/lua-bindings-shootout/master/benchmark_results/table%20global%20string%20get.png
	:alt: retrieve a value from the global table

.. image:: /media/bench/table_global_string_set.png
	:target: https://raw.githubusercontent.com/ThePhD/lua-bindings-shootout/master/benchmark_results/table%20global%20string%20set.png
	:alt: set a value into the global table

.. image:: /media/bench/table_chained_get.png
	:target: https://raw.githubusercontent.com/ThePhD/lua-bindings-shootout/master/benchmark_results/table%20chained%20get.png
	:alt: measures the cost of doing consecutive lookups into a table that exists from C++; some libraries fail here because they do not do lazy evaluation or chaining properly

.. image:: /media/bench/table_get.png
	:target: https://raw.githubusercontent.com/ThePhD/lua-bindings-shootout/master/benchmark_results/table%20get.png
	:alt: measures the cost of retrieving a value from a table in C++; this nests 1 level so as to not be equivalent to any measured global table get optimzations

.. image:: /media/bench/table_chained_set.png
	:target: https://raw.githubusercontent.com/ThePhD/lua-bindings-shootout/master/benchmark_results/table%20chained%20set.png
	:alt: measures the cost of doing consecutive lookups into a table that exists from C++ and setting the final value; some libraries fail here because they do not do lazy evaluation or chaining properly

.. image:: /media/bench/table_set.png
	:target: https://raw.githubusercontent.com/ThePhD/lua-bindings-shootout/master/benchmark_results/table%20set.png
	:alt: measures the cost of setting a value into a table in C++; this nests 1 level so as to not be equivalent to any measured global table set optimzations

.. image:: /media/bench/return_userdata.png
	:target: https://raw.githubusercontent.com/ThePhD/lua-bindings-shootout/master/benchmark_results/return%20userdata.png
	:alt: bind a C function which returns a custom class by-value and call it through Lua code

.. image:: /media/bench/optional_failure.png
	:target: https://raw.githubusercontent.com/ThePhD/lua-bindings-shootout/master/benchmark_results/optional%20failure.png
	:alt: retrieve an item from a table that does not exist in Lua and check for its existence (testing the cost of the failure case)

.. image:: /media/bench/optional_half_failure.png
	:target: https://raw.githubusercontent.com/ThePhD/lua-bindings-shootout/master/benchmark_results/optional%20half%20failure.png
	:alt: retrieve an item from a table that does not exist in Lua and check for its existence (testing the cost of the first level success, second level failure case)

.. image:: /media/bench/optional_success.png
	:target: https://raw.githubusercontent.com/ThePhD/lua-bindings-shootout/master/benchmark_results/optional%20success.png
	:alt: retrieve an item from a table that does not exist in Lua and check for its existence (testing the cost of the success case)

.. image:: /media/bench/base_derived.png
	:target: https://raw.githubusercontent.com/ThePhD/lua-bindings-shootout/master/benchmark_results/base%20derived.png
	:alt: retrieve base class pointer out of Lua without knowing exact derived at compile-time, and have it be correct for multiple-inheritance

.. _lua-bindings-shootout: https://github.com/ThePhD/lua-bindings-shootout
.. _lua_binding_benchmarks: http://satoren.github.io/lua_binding_benchmark/
.. _kaguya: https://github.com/satoren/kaguya
.. _sol: https://github.com/ThePhD/sol2
.. _nonius: https://github.com/rmartinho/nonius/
