#define SOL_CHECK_ARGUMENTS 1
#include <sol.hpp>

#include <iostream>

struct object {
    void my_func() {
        std::cout << "hello\n";
    }
};

int deny(lua_State* L) {
    return luaL_error(L, "HAH! Deniiiiied!");
}

int main() {
    sol::state lua;
    lua.open_libraries(sol::lib::base);

    object my_obj;

    sol::table obj_table = lua.create_named_table("object");

    sol::table obj_metatable = lua.create_table_with();
    obj_metatable.set_function("my_func", &object::my_func, &my_obj);
    // Set whatever else you need to
    // on the obj_metatable, 
    // not on the obj_table itself!

    // Properly self-index metatable to block things
    obj_metatable[sol::meta_function::new_index] = deny;
    obj_metatable[sol::meta_function::index] = obj_metatable;

    // Set it on the actual table
    obj_table[sol::metatable_key] = obj_metatable;

    try {
        lua.script(R"(
print(object.my_func)
object["my_func"] = 24
print(object.my_func)
        )");
    }
    catch (const std::exception& e) {
        std::cout << "an expected error occurred: " << e.what() << std::endl;
    }
    return 0;
}