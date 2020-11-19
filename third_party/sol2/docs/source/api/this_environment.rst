this_environment
================
*retrieving the environment of the calling function*


Sometimes in C++ it's useful to know where a Lua call is coming from and what :doc:`environment<environment>` it is from. The former is covered by Lua's Debug API, which is extensive and is not fully wrapped up by sol2. But, sol2 covers the latter in letting you get the environment of the calling script / function, if it has one. ``sol::this_environment`` is a *transparent argument* and does not need to be passed in Lua scripts or provided when using :doc:`sol::function<function>`, similar to :doc:`sol::this_state<this_state>`:

.. code-block:: cpp
	:linenos:

	#define SOL_CHECK_ARGUMENTS
	#include <sol.hpp>

	#include <iostream>


	void env_check(sol::this_state ts, int x, sol::this_environment te) {
		std::cout << "In C++, 'int x' is in the second position, and its value is: " << x << std::endl;
		if (!te) {
			std::cout << "function does not have an environment: exiting function early" << std::endl;
			return;
		}
		sol::environment& env = te;
		sol::state_view lua = ts;
		sol::environment freshenv = lua["freshenv"];
		bool is_same_env = freshenv == env;
		std::cout << "env == freshenv : " << is_same_env << std::endl;
	}

	int main() {
		sol::state lua;
		sol::environment freshenv(lua, sol::create, lua.globals());
		lua["freshenv"] = freshenv;
		
		lua.set_function("f", env_check);
		
		// note that "f" only takes 1 argument and is okay here
		lua.script("f(25)", freshenv);
		
		return 0;
	}


Also see `this example`_ for more details.

.. _this example: https://github.com/ThePhD/sol2/blob/develop/examples/environment_snooping.cpp