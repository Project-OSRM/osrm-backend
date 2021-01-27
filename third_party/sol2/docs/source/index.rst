.. sol documentation master file, created by
   sphinx-quickstart on Mon Feb 29 21:49:51 2016.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

.. image:: media/sol.png
	:target: https://github.com/ThePhD/sol2
	:alt: sol repository
	:align: center

sol |version|
=============
*a fast, simple C++ and Lua Binding*


When you need to hit the ground running with Lua and C++, `sol`_ is the go-to framework for high-performance binding with an easy to use API.

get going:
----------

.. toctree::
	:maxdepth: 1
	:name: mastertoc
	
	tutorial/all-the-things
	tutorial/tutorial-top
	errors
	compilation
	features
	functions
	usertypes
	containers
	threading
	traits
	api/api-top
	mentions
	benchmarks
	performance
	safety
	exceptions
	rtti
	codecvt
	build
	licenses
	origin


"I need feature X, maybe you have it?"
--------------------------------------
Take a look at the :doc:`Features<features>` page: it links to much of the API. You can also just straight up browse the :doc:`api<api/api-top>` or ease in with the :doc:`tutorials<tutorial/tutorial-top>`. To know more about the implementation for usertypes, see :doc:`here<usertypes>` To know how function arguments are handled, see :ref:`this note<function-argument-handling>`. Don't see a feature you want? Send inquiries for support for a particular abstraction to the `issues`_ tracker.


the basics:
-----------

.. note::
	The code below *and* more examples can be found in the `examples directory`_


.. literalinclude:: ../../examples/docs/simple_functions.cpp
	:name: simple-functions-example
	:linenos:

.. literalinclude:: ../../examples/docs/simple_structs.cpp
	:name: simple-structs-example
	:linenos:	


helping out
-----------

You can support sol2 development by `donating here`_. This is a time-consuming effort, so individuals who donate get to:

- steer the direction and time spent on sol
- get a role on the Discord server
- get their name put up in the CONTRIBUTORS list
- put something of their choice on sol2's README or the documentation's front page

You can also help out the library by submitting pull requests to fix anything or add anything you think would be helpful! This includes making small, useful examples of something you haven't seen, or fixing typos and bad code in the documentation.

Finally, `come join in Discord`_!

Indices and tables
==================

* :ref:`genindex`
* :ref:`search`

.. _Sol: https://github.com/ThePhD/sol2
.. _issues: https://github.com/ThePhD/sol2/issues
.. _examples directory: https://github.com/ThePhD/sol2/tree/develop/examples
.. _donating here: https://www.paypal.me/LMeneide
.. _come join in Discord: https://discord.gg/buxkYNT
