#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

// Something that can't be collided with
static const auto& script_key = "GlobalResource.MySpecialIdentifier123";

struct GlobalResource {
	int value = 2;
};

// Customize sol2 to handle this type
namespace sol {
	template <>
	struct lua_type_of<GlobalResource*> : std::integral_constant<sol::type, sol::type::lightuserdata> {};

	namespace stack {
		template <>
		struct checker<GlobalResource*> {
			template <typename Handler>
			static bool check(lua_State* L, int /*index*/, Handler&& handler, record& tracking) {
				tracking.use(0);
				// get the field from global storage
				stack::get_field<true>(L, script_key);
				// verify type
				type t = static_cast<type>(lua_type(L, -1));
				lua_pop(L, 1);
				if (t != type::lightuserdata) {
					handler(L, 0, type::lightuserdata, t, "global resource is not present");
					return false;
				}
				return true;
			}
		};

		template <>
		struct getter<GlobalResource*> {
			static GlobalResource* get(lua_State* L, int /*index*/, record& tracking) {
				// retrieve the (light) userdata for this type
				tracking.use(0); // not actually pulling anything off the stack
				stack::get_field<true>(L, script_key);
				GlobalResource* ls = static_cast<GlobalResource*>(lua_touserdata(L, -1));
				lua_pop(L, 1); // clean up stack value returned by `get_field`
				return ls;
			};
		};
		template <>
		struct pusher<GlobalResource*> {
			static int push(lua_State* L, GlobalResource* ls) {
				// push light userdata
				return stack::push(L, make_light(ls));;
			}
		};
	}
}

int main() {

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	GlobalResource instance;

	// get GlobalResource
	lua.set_function("f", [](GlobalResource* l, int value) {
		return l->value + value;
	});
	lua.set(script_key, &instance);

	// note only 1 argument,
	// despite being 2
	lua.script("assert(f(1) == 3)");

	return 0;
}
