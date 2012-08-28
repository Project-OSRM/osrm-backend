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

#include <iostream>

#include <v8.h>

namespace V8Helper {
//Provides an output mechanism to the V8 Engine
static v8::Handle<v8::Value> PrintToConsole (const v8::Arguments& args){
    std::cout << "[JS] : " << *v8::String::AsciiValue(args[0]) << std::endl;
    return v8::Undefined();
}

//Returns the version of the V8 Engine
v8::Handle<v8::Value> Version(const v8::Arguments& args) {
    return v8::String::New(v8::V8::GetVersion());
}

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

// Extracts a C string from a V8 Utf8Value.
const char* ToCString(const v8::String::Utf8Value& value) {
    return *value ? *value : "<string conversion failed>";
}

void ReportException(v8::TryCatch* try_catch) {
    v8::HandleScope handle_scope;
    v8::String::Utf8Value exception(try_catch->Exception());
    const char* exception_string = ToCString(exception);
    v8::Handle<v8::Message> message = try_catch->Message();
    if (message.IsEmpty()) {
        // V8 didn't provide any extra information about this error; just
        // print the exception.
        std::cout << exception_string << std::endl;
    } else {
        // Print (filename):(line number): (message).
        v8::String::Utf8Value filename(message->GetScriptResourceName());
        int linenum = message->GetLineNumber();
        std::cout << ToCString(filename) << ":" << linenum << ": " << exception_string << std::endl;
        // Print line of source code.
        v8::String::Utf8Value sourceline(message->GetSourceLine());
        std::cout << ToCString(sourceline) << std::endl;

        // Print wavy underline (GetUnderline is deprecated).
        int start = message->GetStartColumn();
        for (int i = 0; i < start; ++i) {
            std::cout << " ";
        }

        for (int i = start, end = message->GetEndColumn(); i < end; ++i) {
            std::cout << "^";
        }
        std::cout << std::endl;
        v8::String::Utf8Value stack_trace(try_catch->StackTrace());
        if (stack_trace.length()) {
            std::cout << ToCString(stack_trace) << std::endl;
        }
    }
}
}

#endif /* V8HELPER_H_ */
