userdata
========
*reference to a userdata*

.. code-block:: cpp
	:caption: (light\_)userdata reference

	class userdata : public table;

	class light_userdata : public table;

These types are meant to hold a reference to a (light) userdata from Lua and make it easy to push an existing userdata onto the stack. It is essentially identical to :doc:`table<table>` in every way, just with a definitive C++ type that ensures the type is some form of userdata (helpful for trapping type errors with :doc:`safety features turned on<../safety>`). You can also use its ``.is<T>()`` and ``.as<T>()`` methods to check if its of a specific type and retrieve that type, respectively.
