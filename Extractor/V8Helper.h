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

#ifndef V8HELPER_H_
#define V8HELPER_H_

//Most of the stuff below is from http://blog.owned.co.za provided with an MIT License

template<class T>
v8::Handle<v8::Object> WrapClass(T* y) {
    // Handle scope for temporary handles,
    v8::HandleScope handle_scope;
    v8::Persistent<v8::ObjectTemplate>  class_template_;

    v8::Handle<v8::ObjectTemplate> raw_template = v8::ObjectTemplate::New();

    //The raw template is the ObjectTemplate (that can be exposed to script too)
    //but is maintained internally.
    raw_template->SetInternalFieldCount(1);

    //Create the actual template object,
    class_template_ = v8::Persistent<v8::ObjectTemplate>::New(raw_template);

    //Create the new handle to return, and set its template type
    v8::Handle<v8::Object> result = class_template_->NewInstance();
    v8::Handle<v8::External> class_ptr = v8::External::New(static_cast<T*>(y));

    //Point the 0 index Field to the c++ pointer for unwrapping later
    result->SetInternalField(0, class_ptr);

    //Return the value, releasing the other handles on its way.
    return handle_scope.Close(result);
}


template<class T>
v8::Handle<v8::Object> ExposeClass(v8::Persistent<v8::Context> context, T* y, v8::Handle<v8::Value> exposedName, v8::PropertyAttribute props) {
    v8::HandleScope handle_scope;

    v8::Handle<v8::Object> obj = WrapClass<T>(y);
    context->Global()->Set(exposedName, obj, props);

    return handle_scope.Close(obj);
}

template<class T>
T* UnwrapClass(v8::Handle<v8::Object> obj) {
    v8::Handle<v8::External> field = v8::Handle<v8::External>::Cast(obj->GetInternalField(0));
    void* ptr = field->Value();
    return static_cast<T*>(ptr);
}

void Expose(v8::Handle<v8::Object> intoObject, v8::Handle<v8::Value> namev8String, v8::InvocationCallback funcID) {
    v8::HandleScope handle_scope;

    v8::Handle<v8::FunctionTemplate> thisFunc = v8::FunctionTemplate::New(funcID);
    intoObject->Set(namev8String, thisFunc->GetFunction());
}

template<typename WrappedType, typename PropertyType, PropertyType WrappedType::*MemVar>
v8::Handle<v8::Value> GetMember(v8::Local<v8::String> property, const v8::AccessorInfo &info) {
    v8::Local<v8::Object> self = info.Holder();
    v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
    void* ptr = wrap->Value();
    int value = static_cast<WrappedType*>(ptr)->*MemVar;
    return PropertyType::New(value);
}


template <typename WrappedType, typename CPPPropertyType, CPPPropertyType WrappedType::*MemVar>
void SetIntMember(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo& info) {
    v8::Local<v8::Object> self = info.Holder();
    v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
    void * ptr = wrap->Value();
    static_cast<WrappedType*>(ptr)->*MemVar = value->Int32Value();
}

template<typename WrappedType, typename JSPropertyType, typename CPPProperyType, CPPProperyType WrappedType::*MemVar>
v8::Handle<v8::Value> GetInt(v8::Local<v8::String> property, const v8::AccessorInfo &info) {
    v8::Local<v8::Object> self = info.Holder();
    v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
    void* ptr = wrap->Value();
    CPPProperyType value = static_cast<WrappedType*>(ptr)->member1;
    return JSPropertyType::New(value);
}


#endif /* V8HELPER_H_ */
