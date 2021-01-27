customization traits
====================

These are customization points within the library to help you make sol2 work for the types in your framework and types. 

To learn more about various customizable traits, visit:

* :doc:`containers customization traits<containers>`
	- This is how to work with containers in their entirety and what operations you're afforded on them
	- when you have an compiler error when serializing a type that has ``begin`` and ``end`` functions but isn't exactly a container
* :doc:`unique usertype (custom pointer) traits<api/unique_usertype_traits>`
	- This is how to deal with unique usertypes, e.g. ``boost::shared_ptr``, reference-counted pointers, etc
	- Useful for custom pointers from all sorts of frameworks or handle types that employ very specific kinds of destruction semantics and access
* :doc:`customization points<tutorial/customization>`
	- This is how to customize a type to work with sol2
	- Can be used for specializations to push strings and other class types that are not natively ``std::string`` or ``const char*``, like `a wxString, for example`_
	  
.. _a wxString, for example: https://github.com/ThePhD/sol2/issues/140#issuecomment-237934947
