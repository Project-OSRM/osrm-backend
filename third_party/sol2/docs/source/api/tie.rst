tie
===
*improved version of std::tie*


`std::tie()`_ does not work well with :doc:`sol::function<function>`'s ``sol::function_result`` returns. Use ``sol::tie`` instead. Because they're both named `tie`, you'll need to be explicit when you use Sol's by naming it with the namespace (``sol::tie``), even with a ``using namespace sol;``. Here's an example:

.. literalinclude:: ../../../examples/tie.cpp
	:linenos:

.. _std::tie(): http://en.cppreference.com/w/cpp/utility/tuple/tie
