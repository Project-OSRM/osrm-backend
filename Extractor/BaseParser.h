/*
    open source routing machine
    Copyright (C) Dennis Luxen, others 2010

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU AFFERO General Public License as published by
the Free Software Foundation; either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
or see http://www.gnu.org/licenses/agpl.txt.
 */

#ifndef BASEPARSER_H_
#define BASEPARSER_H_

#include <luabind/luabind.hpp>

template<typename NodeT, typename RestrictionT, typename WayT>
class BaseParser {
public:
    virtual ~BaseParser() {}
    virtual bool Init() = 0;
    virtual bool RegisterCallbacks(bool (*nodeCallbackPointer)(NodeT), bool (*restrictionCallbackPointer)(RestrictionT), bool (*wayCallbackPointer)(WayT)) = 0;
    virtual void RegisterLUAState(lua_State *myLuaState) = 0;
    virtual bool Parse() = 0;

    void report_errors(lua_State *L, int status) {
        if ( status!=0 ) {
            std::cerr << "-- " << lua_tostring(L, -1) << std::endl;
            lua_pop(L, 1); // remove error message
        }
    }

};

#endif /* BASEPARSER_H_ */
