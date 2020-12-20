new_table
=========
*a table creation hint to environment/table*


.. code-block:: cpp
	
	struct new_table;

	constexpr const new_table create = new_table{};

``sol::new_table`` serves the purpose of letting you create tables using the constructor of :doc:`sol::table<table>` and :doc:`sol::environment<environment>`. It also disambiguates the other kinds of constructors, so is **necessary** to be specified. Leaving it off will result in the wrong constructor to be called, for either ``sol::table`` or ``sol::environment``.

members
-------

.. code-block:: cpp
	:caption: constructor: new_table

	new_table(int sequence_hint = 0, int map_hint = 0);
	
The constructor's sole purpose is to either let you default-constructor the type, in which case it uses the values of "0" for its two hints, or letting you specify either ``sequence_hint`` or both the ``sequence_hint`` and ``map_hint``. Each hint is a heuristic helper for Lua to allocate an appropriately sized and structured table for what you intend to do. In 99% of cases, you will most likely not care about it and thusly will just use the constant ``sol::create`` as the second argument to object-creators like ``sol::table``'s constructor.