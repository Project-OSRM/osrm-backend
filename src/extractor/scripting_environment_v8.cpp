#include "extractor/scripting_environment_v8.hpp"

#include "extractor/external_memory_node.hpp"
#include "extractor/extraction_helper_functions.hpp"
#include "extractor/extraction_node.hpp"
#include "extractor/extraction_way.hpp"
#include "extractor/internal_extractor_edge.hpp"
#include "extractor/profile_properties.hpp"
#include "extractor/raster_source.hpp"
#include "extractor/restriction_parser.hpp"
#include "util/exception.hpp"
#include "util/make_unique.hpp"
#include "util/simple_logger.hpp"
#include "util/typedefs.hpp"
#include "util/v8_util.hpp"

#include <osmium/osm.hpp>

#include <tbb/parallel_for.h>

#include <pthread.h>

#include <sstream>

namespace osrm
{
namespace extractor
{

static util::V8Allocator allocator;

struct V8ScriptingContext final
{
    V8ScriptingContext(const std::string &file_name);
    ~V8ScriptingContext();

    struct IsolateDeleter
    {
        void operator()(v8::Isolate *isolate) const
        {
            // TODO: This crashes
            // isolate->Dispose();
            (void)isolate;
        }
    };

    std::vector<std::string> GetExceptions();
    std::vector<std::string> GetNameSuffixList();
    void ProcessElements(
        const tbb::blocked_range<std::size_t> &range,
        const std::vector<osmium::memory::Buffer::const_iterator> &osm_elements,
        const RestrictionParser &restriction_parser,
        tbb::concurrent_vector<std::pair<std::size_t, ExtractionNode>> &resulting_nodes,
        tbb::concurrent_vector<std::pair<std::size_t, ExtractionWay>> &resulting_ways,
        tbb::concurrent_vector<boost::optional<InputRestrictionContainer>> &resulting_restrictions);
    int32_t GetTurnPenalty(const double angle);

    static void DurationIsValid(const v8::FunctionCallbackInfo<v8::Value>& info);
    static void ParseDuration(const v8::FunctionCallbackInfo<v8::Value>& info);
    static void TrimLaneString(const v8::FunctionCallbackInfo<v8::Value>& info);

    ProfileProperties properties;
    SourceContainer sources;

    std::unique_ptr<v8::Isolate, IsolateDeleter> isolate;
    v8::Isolate::Scope isolate_scope;
    v8::HandleScope handle_scope;
    v8::Local<v8::Context> context;
    v8::Context::Scope context_scope;
    util::V8Env env;
    v8::Local<v8::Script> script;

    v8::Persistent<v8::Function> get_exceptions;
    v8::Persistent<v8::Function> get_name_suffix_list;
    v8::Persistent<v8::Function> process_node;
    v8::Persistent<v8::Function> process_way;
    v8::Persistent<v8::Function> get_turn_penalty;

    v8::Persistent<v8::Function> osmium_node;
    v8::Persistent<v8::Function> result_node;
    v8::Persistent<v8::Function> osmium_way;
    v8::Persistent<v8::Function> result_way;

    v8::Local<v8::Function> WrapProfileProperties();
    v8::Local<v8::Function> WrapConsole();
    v8::Local<v8::Function> WrapOsmiumNode();
    v8::Local<v8::Function> WrapResultNode();
    v8::Local<v8::Function> WrapOsmiumWay();
    v8::Local<v8::Function> WrapResultWay();

    bool has_turn_penalty_function;
    bool has_node_function;
    bool has_way_function;
    bool has_segment_function;
};

V8ScriptingContext::V8ScriptingContext(const std::string &file_name)
    : isolate([] {
          v8::Isolate::CreateParams create_params;
          create_params.array_buffer_allocator = &allocator;
          return v8::Isolate::New(create_params);
      }()),
      isolate_scope(isolate.get()), handle_scope(isolate.get()),
      context(v8::Context::New(isolate.get())), context_scope(context), env(), script([&] {
          std::ifstream stream(file_name.c_str());
          if (!stream)
          {
              throw std::runtime_error("could not locate file " + file_name);
          }

          std::istreambuf_iterator<char> begin(stream), end;
          const std::string value(begin, end);

          return v8::Script::Compile(context, env.new_string(value.data(), value.length()))
              .ToLocalChecked();
      }())
{
    v8::HandleScope scope(isolate.get());

    env.set_property(context->Global(),
                     "properties",
                     env.new_object(WrapProfileProperties(), properties),
                     v8::PropertyAttribute::DontDelete);

    env.set_property(context->Global(),
                     "console",
                     WrapConsole()->NewInstance(),
                     v8::PropertyAttribute::DontDelete);

    env.set_property(context->Global(),
                     "durationIsValid",
                     env.new_function_template(DurationIsValid)->GetFunction());
    env.set_property(context->Global(),
                     "parseDuration",
                     env.new_function_template(ParseDuration)->GetFunction());
    env.set_property(context->Global(),
                     "trimLaneString",
                     env.new_function_template(TrimLaneString)->GetFunction());

    auto mode = env.new_object();
    env.set(mode, "inaccessible", env.new_integer(TRAVEL_MODE_INACCESSIBLE));
    env.set(mode, "driving", env.new_integer(TRAVEL_MODE_DRIVING));
    env.set(mode, "cycling", env.new_integer(TRAVEL_MODE_CYCLING));
    env.set(mode, "walking", env.new_integer(TRAVEL_MODE_WALKING));
    env.set(mode, "ferry", env.new_integer(TRAVEL_MODE_FERRY));
    env.set(mode, "train", env.new_integer(TRAVEL_MODE_TRAIN));
    env.set(mode, "pushingBike", env.new_integer(TRAVEL_MODE_PUSHING_BIKE));
    env.set(mode, "stepsUp", env.new_integer(TRAVEL_MODE_STEPS_UP));
    env.set(mode, "stepsDown", env.new_integer(TRAVEL_MODE_STEPS_DOWN));
    env.set(mode, "riverUp", env.new_integer(TRAVEL_MODE_RIVER_UP));
    env.set(mode, "riverDown", env.new_integer(TRAVEL_MODE_RIVER_DOWN));
    env.set(mode, "route", env.new_integer(TRAVEL_MODE_ROUTE));
    env.freeze(mode);
    env.set_property(context->Global(), "mode", mode, v8::PropertyAttribute::DontDelete);

    // TODO: try catch
    auto result = script->Run(context);
    if (!result.IsEmpty())
    {
        auto get_exceptions_local = env.get(context->Global(), "getExceptions");
        if (!get_exceptions_local.IsEmpty() && get_exceptions_local->IsFunction())
        {
            get_exceptions.Reset(isolate.get(), get_exceptions_local.As<v8::Function>());
        }

        auto get_name_suffix_list_local = env.get(context->Global(), "getNameSuffixList");
        if (!get_name_suffix_list_local.IsEmpty() && get_name_suffix_list_local->IsFunction())
        {
            get_name_suffix_list.Reset(isolate.get(),
                                       get_name_suffix_list_local.As<v8::Function>());
        }

        auto process_node_local = env.get(context->Global(), "processNode");
        if (!process_node_local.IsEmpty() && process_node_local->IsFunction())
        {
            process_node.Reset(isolate.get(), process_node_local.As<v8::Function>());
            has_node_function = true;
        }

        auto process_way_local = env.get(context->Global(), "processWay");
        if (!process_way_local.IsEmpty() && process_way_local->IsFunction())
        {
            process_way.Reset(isolate.get(), process_way_local.As<v8::Function>());
            has_way_function = true;
        }

        auto get_turn_penalty_local = env.get(context->Global(), "getTurnPenalty");
        if (!get_turn_penalty_local.IsEmpty() && get_turn_penalty_local->IsFunction())
        {
            get_turn_penalty.Reset(isolate.get(), get_turn_penalty_local.As<v8::Function>());
            has_turn_penalty_function = true;
        }
    }

    osmium_node.Reset(isolate.get(), WrapOsmiumNode());
    result_node.Reset(isolate.get(), WrapResultNode());
    osmium_way.Reset(isolate.get(), WrapOsmiumWay());
    result_way.Reset(isolate.get(), WrapResultWay());
}

template <typename I>
std::string to_string(const I &info, const int index, const std::string &def = {})
{
    if (info.Length() < (index + 1) || info[index]->IsUndefined())
    {
        return def;
    }
    else
    {
        v8::String::Utf8Value utfValue(info[index]->ToString());
        return std::string{*utfValue, static_cast<size_t>(utfValue.length())};
    }
}

template <typename I>
int32_t to_integer(const I &info, const int index, const int32_t def = 0)
{
    if (info.Length() < (index + 1) || info[index]->IsUndefined())
    {
        return def;
    }
    else
    {
        return info[index]->ToInt32()->Value();
    }
}


void V8ScriptingContext::DurationIsValid(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    util::V8Env env;
    v8::HandleScope scope(env.isolate);

    info.GetReturnValue().Set(extractor::durationIsValid(to_string(info, 0)));
}

void V8ScriptingContext::ParseDuration(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    util::V8Env env;
    v8::HandleScope scope(env.isolate);
    info.GetReturnValue().Set(extractor::parseDuration(to_string(info, 0)));
}

void V8ScriptingContext::TrimLaneString(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    util::V8Env env;
    v8::HandleScope scope(env.isolate);
    const auto lane_string =
        extractor::trimLaneString(to_string(info, 0), to_integer(info, 1), to_integer(info, 2));
    info.GetReturnValue().Set(env.new_string(lane_string));
}


v8::Local<v8::Function> V8ScriptingContext::WrapProfileProperties()
{
    util::V8Object obj(env, "Properties");
    using Class = util::V8Class<ProfileProperties>;

    obj.property("trafficSignalPenalty",
                 [](v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value> &info) {
                     info.GetReturnValue().Set(Class::Unwrap(info).GetTrafficSignalPenalty());
                 },
                 [](v8::Local<v8::String>,
                    v8::Local<v8::Value> value,
                    const v8::PropertyCallbackInfo<void> &info) {
                     Class::Unwrap(info).SetTrafficSignalPenalty(value->ToNumber()->Value());
                 });

    obj.property("uTurnPenalty",
                 [](v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value> &info) {
                     info.GetReturnValue().Set(Class::Unwrap(info).GetUturnPenalty());
                 },
                 [](v8::Local<v8::String>,
                    v8::Local<v8::Value> value,
                    const v8::PropertyCallbackInfo<void> &info) {
                     Class::Unwrap(info).SetUturnPenalty(value->ToNumber()->Value());
                 });

    obj.property("continueStraightAtWaypoint",
                 [](v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value> &info) {
                     info.GetReturnValue().Set(Class::Unwrap(info).continue_straight_at_waypoint);
                 },
                 [](v8::Local<v8::String>,
                    v8::Local<v8::Value> value,
                    const v8::PropertyCallbackInfo<void> &info) {
                     Class::Unwrap(info).continue_straight_at_waypoint =
                         value->ToBoolean()->Value();
                 });

    obj.property("useTurnRestrictions",
                 [](v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value> &info) {
                     info.GetReturnValue().Set(Class::Unwrap(info).use_turn_restrictions);
                 },
                 [](v8::Local<v8::String>,
                    v8::Local<v8::Value> value,
                    const v8::PropertyCallbackInfo<void> &info) {
                     Class::Unwrap(info).use_turn_restrictions = value->ToBoolean()->Value();
                 });

    return obj.get();
}

std::mutex cerr_mutex;

v8::Local<v8::Function> V8ScriptingContext::WrapConsole()
{
    util::V8Object obj(env, "Console");

    obj.method("log", [](const v8::FunctionCallbackInfo<v8::Value> &info) {
        util::V8Env env;
        v8::HandleScope(env.isolate);
        for (int i = 0; i < info.Length(); i++)
        {
            v8::String::Utf8Value value(info[i]->ToString());
            if (i > 0)
            {
                std::cout << " ";
            }
            std::cout.write(*value, value.length());
        }
        std::cout << std::endl;
    });

    obj.method("warn", [](const v8::FunctionCallbackInfo<v8::Value> &info) {
        util::V8Env env;
        v8::HandleScope(env.isolate);
        std::lock_guard<std::mutex> lock(cerr_mutex);
        for (int i = 0; i < info.Length(); i++)
        {
            v8::String::Utf8Value value(info[i]->ToString());
            if (i > 0)
            {
                std::cerr << " ";
            }
            std::cerr.write(*value, value.length());
        }
        std::cerr << std::endl;
    });

    return obj.get();
}

v8::Local<v8::Function> V8ScriptingContext::WrapOsmiumNode()
{
    util::V8Object obj(env, "Node");
    using Class = util::V8Class<const osmium::Node>;

    obj.method("tag", [](const v8::FunctionCallbackInfo<v8::Value> &info) {
        util::V8Env env;
        v8::HandleScope(env.isolate);
        if (info.Length() < 1)
        {
            env.isolate->ThrowException(
                v8::Exception::TypeError(env.new_string("first argument must be a tag name")));
            return;
        }

        auto value = Class::Unwrap(info).get_value_by_key(*v8::String::Utf8Value(info[0]));
        if (value)
        {
            info.GetReturnValue().Set(env.new_string(value));
        }
    });

    obj.property("id", [](v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value> &info) {
        info.GetReturnValue().Set(static_cast<double>(Class::Unwrap(info).id()));
    });

    obj.property("lat", [](v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value> &info) {
        info.GetReturnValue().Set(static_cast<double>(Class::Unwrap(info).location().lat()));
    });

    obj.property("lon", [](v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value> &info) {
        info.GetReturnValue().Set(static_cast<double>(Class::Unwrap(info).location().lon()));
    });

    return obj.get();
}

v8::Local<v8::Function> V8ScriptingContext::WrapResultNode()
{
    util::V8Object obj(env, "ResultNode");
    using Class = util::V8Class<extractor::ExtractionNode>;

    obj.property("trafficLights",
                 [](v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value> &info) {
                     info.GetReturnValue().Set(Class::Unwrap(info).traffic_lights);
                 },
                 [](v8::Local<v8::String>,
                    v8::Local<v8::Value> value,
                    const v8::PropertyCallbackInfo<void> &info) {
                     Class::Unwrap(info).traffic_lights = value->ToBoolean()->Value();
                 });

    obj.property("barrier",
                 [](v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value> &info) {
                     info.GetReturnValue().Set(Class::Unwrap(info).barrier);
                 },
                 [](v8::Local<v8::String>,
                    v8::Local<v8::Value> value,
                    const v8::PropertyCallbackInfo<void> &info) {
                     Class::Unwrap(info).barrier = value->ToBoolean()->Value();
                 });

    return obj.get();
}

v8::Local<v8::Function> V8ScriptingContext::WrapOsmiumWay()
{
    util::V8Object obj(env, "Way");
    using Class = util::V8Class<const osmium::Way>;

    obj.method("tag", [](const v8::FunctionCallbackInfo<v8::Value> &info) {
        util::V8Env env;
        v8::HandleScope(env.isolate);
        if (info.Length() < 1)
        {
            env.isolate->ThrowException(
                v8::Exception::TypeError(env.new_string("first argument must be a tag name")));
            return;
        }

        auto value = Class::Unwrap(info).get_value_by_key(*v8::String::Utf8Value(info[0]));
        if (value)
        {
            info.GetReturnValue().Set(env.new_string(value));
        }
    });

    obj.property("id", [](v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value> &info) {
        info.GetReturnValue().Set(static_cast<double>(Class::Unwrap(info).id()));
    });

    // TODO: get_nodes_for_way

    return obj.get();
}

v8::Local<v8::Function> V8ScriptingContext::WrapResultWay()
{
    util::V8Object obj(env, "ResultWay");
    using Class = util::V8Class<extractor::ExtractionWay>;

    obj.property("forwardSpeed",
                 [](v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value> &info) {
                     info.GetReturnValue().Set(Class::Unwrap(info).forward_speed);
                 },
                 [](v8::Local<v8::String>,
                    v8::Local<v8::Value> value,
                    const v8::PropertyCallbackInfo<void> &info) {
                     Class::Unwrap(info).forward_speed = value->ToNumber()->Value();
                 });

    obj.property("backwardSpeed",
                 [](v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value> &info) {
                     info.GetReturnValue().Set(Class::Unwrap(info).backward_speed);
                 },
                 [](v8::Local<v8::String>,
                    v8::Local<v8::Value> value,
                    const v8::PropertyCallbackInfo<void> &info) {
                     Class::Unwrap(info).backward_speed = value->ToNumber()->Value();
                 });

    obj.property("duration",
                 [](v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value> &info) {
                     info.GetReturnValue().Set(Class::Unwrap(info).duration);
                 },
                 [](v8::Local<v8::String>,
                    v8::Local<v8::Value> value,
                    const v8::PropertyCallbackInfo<void> &info) {
                     Class::Unwrap(info).duration = value->ToNumber()->Value();
                 });

    obj.property("name",
                 [](v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value> &info) {
                     util::V8Env env;
                     info.GetReturnValue().Set(env.new_string(Class::Unwrap(info).name));
                 },
                 [](v8::Local<v8::String>,
                    v8::Local<v8::Value> value,
                    const v8::PropertyCallbackInfo<void> &info) {
                     const v8::String::Utf8Value utfValue(value->ToString());
                     Class::Unwrap(info).name = {*utfValue, static_cast<size_t>(utfValue.length())};
                 });

    obj.property(
        "pronunciation",
        [](v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value> &info) {
            util::V8Env env;
            info.GetReturnValue().Set(env.new_string(Class::Unwrap(info).pronunciation));
        },
        [](v8::Local<v8::String>,
           v8::Local<v8::Value> value,
           const v8::PropertyCallbackInfo<void> &info) {
            const v8::String::Utf8Value utfValue(value->ToString());
            Class::Unwrap(info).pronunciation = {*utfValue, static_cast<size_t>(utfValue.length())};
        });

    obj.property(
        "destinations",
        [](v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value> &info) {
            util::V8Env env;
            info.GetReturnValue().Set(env.new_string(Class::Unwrap(info).destinations));
        },
        [](v8::Local<v8::String>,
           v8::Local<v8::Value> value,
           const v8::PropertyCallbackInfo<void> &info) {
            const v8::String::Utf8Value utfValue(value->ToString());
            Class::Unwrap(info).destinations = {*utfValue, static_cast<size_t>(utfValue.length())};
        });

    obj.property(
        "turnLanesForward",
        [](v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value> &info) {
            util::V8Env env;
            info.GetReturnValue().Set(env.new_string(Class::Unwrap(info).turn_lanes_forward));
        },
        [](v8::Local<v8::String>,
           v8::Local<v8::Value> value,
           const v8::PropertyCallbackInfo<void> &info) {
            const v8::String::Utf8Value utfValue(value->ToString());
            Class::Unwrap(info).turn_lanes_forward = {*utfValue,
                                                      static_cast<size_t>(utfValue.length())};
        });

    obj.property(
        "turnLanesBackward",
        [](v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value> &info) {
            util::V8Env env;
            info.GetReturnValue().Set(env.new_string(Class::Unwrap(info).turn_lanes_backward));
        },
        [](v8::Local<v8::String>,
           v8::Local<v8::Value> value,
           const v8::PropertyCallbackInfo<void> &info) {
            const v8::String::Utf8Value utfValue(value->ToString());
            Class::Unwrap(info).turn_lanes_backward = {*utfValue,
                                                       static_cast<size_t>(utfValue.length())};
        });

    obj.property("roundabout",
                 [](v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value> &info) {
                     info.GetReturnValue().Set(Class::Unwrap(info).roundabout);
                 },
                 [](v8::Local<v8::String>,
                    v8::Local<v8::Value> value,
                    const v8::PropertyCallbackInfo<void> &info) {
                     Class::Unwrap(info).roundabout = value->ToBoolean()->Value();
                 });

    obj.property("isAccessRestricted",
                 [](v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value> &info) {
                     info.GetReturnValue().Set(Class::Unwrap(info).is_access_restricted);
                 },
                 [](v8::Local<v8::String>,
                    v8::Local<v8::Value> value,
                    const v8::PropertyCallbackInfo<void> &info) {
                     Class::Unwrap(info).is_access_restricted = value->ToBoolean()->Value();
                 });

    obj.property("isStartpoint",
                 [](v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value> &info) {
                     info.GetReturnValue().Set(Class::Unwrap(info).is_startpoint);
                 },
                 [](v8::Local<v8::String>,
                    v8::Local<v8::Value> value,
                    const v8::PropertyCallbackInfo<void> &info) {
                     Class::Unwrap(info).is_startpoint = value->ToBoolean()->Value();
                 });

    obj.property("forwardMode",
                 [](v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value> &info) {
                     info.GetReturnValue().Set(
                         static_cast<uint32_t>(Class::Unwrap(info).forward_travel_mode));
                 },
                 [](v8::Local<v8::String>,
                    v8::Local<v8::Value> value,
                    const v8::PropertyCallbackInfo<void> &info) {
                     Class::Unwrap(info).forward_travel_mode =
                         static_cast<extractor::TravelMode>(value->ToUint32()->Value());
                 });

    obj.property("backwardMode",
                 [](v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value> &info) {
                     info.GetReturnValue().Set(
                         static_cast<uint32_t>(Class::Unwrap(info).backward_travel_mode));
                 },
                 [](v8::Local<v8::String>,
                    v8::Local<v8::Value> value,
                    const v8::PropertyCallbackInfo<void> &info) {
                     Class::Unwrap(info).backward_travel_mode =
                         static_cast<extractor::TravelMode>(value->ToUint32()->Value());
                 });

    return obj.get();
}

std::vector<std::string> V8ScriptingContext::GetExceptions()
{
    v8::HandleScope scope(isolate.get());
    std::vector<std::string> restriction_exceptions;

    v8::Local<v8::Function> callback = get_exceptions.Get(isolate.get());
    if (!callback.IsEmpty())
    {
        auto retval = callback->Call(context->Global(), 0, {});

        if (retval->IsArray())
        {
            auto array = retval.As<v8::Array>();
            for (uint32_t i = 0; i < array->Length(); i++)
            {
                restriction_exceptions.emplace_back(env.get_string(array, i));
            }
        }
        else
        {
            util::SimpleLogger().Write() << "getExceptions() should return an array of exceptions";
        }
    }

    return restriction_exceptions;
}

std::vector<std::string> V8ScriptingContext::GetNameSuffixList()
{
    v8::HandleScope scope(isolate.get());
    std::vector<std::string> suffixes_vector;

    v8::Local<v8::Function> callback = get_name_suffix_list.Get(isolate.get());
    if (!callback.IsEmpty())
    {
        auto retval = callback->Call(context->Global(), 0, {});

        if (retval->IsArray())
        {
            auto array = retval.As<v8::Array>();
            for (uint32_t i = 0; i < array->Length(); i++)
            {
                suffixes_vector.emplace_back(env.get_string(array, i));
            }
        }
        else
        {
            util::SimpleLogger().Write()
                << "getNameSuffixList() should return an array of exceptions";
        }
    }

    return suffixes_vector;
}

void V8ScriptingContext::ProcessElements(
    const tbb::blocked_range<std::size_t> &range,
    const std::vector<osmium::memory::Buffer::const_iterator> &osm_elements,
    const RestrictionParser &restriction_parser,
    tbb::concurrent_vector<std::pair<std::size_t, ExtractionNode>> &resulting_nodes,
    tbb::concurrent_vector<std::pair<std::size_t, ExtractionWay>> &resulting_ways,
    tbb::concurrent_vector<boost::optional<InputRestrictionContainer>> &resulting_restrictions)
{
    v8::HandleScope scope(isolate.get());

    v8::Local<v8::Function> process_node_fn = process_node.Get(isolate.get());
    v8::Local<v8::Function> osmium_node_fn = osmium_node.Get(isolate.get());
    v8::Local<v8::Object> node = osmium_node_fn->NewInstance();
    v8::Local<v8::Function> result_node_fn = result_node.Get(isolate.get());
    v8::Local<v8::Object> result_node = result_node_fn->NewInstance();
    ExtractionNode extraction_node;
    result_node->SetInternalField(0, v8::External::New(isolate.get(), &extraction_node));
    constexpr const int node_argc = 2;
    v8::Local<v8::Value> node_argv[node_argc] = {node, result_node};

    v8::Local<v8::Function> process_way_fn = process_way.Get(isolate.get());
    v8::Local<v8::Function> osmium_way_fn = osmium_way.Get(isolate.get());
    v8::Local<v8::Object> way = osmium_way_fn->NewInstance();
    v8::Local<v8::Function> result_way_fn = result_way.Get(isolate.get());
    v8::Local<v8::Object> result_way = result_way_fn->NewInstance();
    ExtractionWay extraction_way;
    result_way->SetInternalField(0, v8::External::New(isolate.get(), &extraction_way));
    constexpr const int way_argc = 2;
    v8::Local<v8::Value> way_argv[way_argc] = {way, result_way};

    for (auto x = range.begin(), end = range.end(); x != end; ++x)
    {
        const auto entity = osm_elements[x];

        switch (entity->type())
        {
        case osmium::item_type::node:
            extraction_node.clear();
            if (has_node_function)
            {
                node->SetInternalField(
                    0, v8::External::New(isolate.get(), &const_cast<osmium::OSMEntity &>(*entity)));
                process_node_fn->Call(context->Global(), node_argc, node_argv);
            }
            resulting_nodes.push_back(std::make_pair(x, std::move(extraction_node)));
            break;
        case osmium::item_type::way:
            extraction_way.clear();
            if (has_way_function)
            {
                way->SetInternalField(
                    0, v8::External::New(isolate.get(), &const_cast<osmium::OSMEntity &>(*entity)));
                process_way_fn->Call(context->Global(), way_argc, way_argv);
            }
            resulting_ways.push_back(std::make_pair(x, std::move(extraction_way)));
            break;
        case osmium::item_type::relation:
            resulting_restrictions.push_back(
                restriction_parser.TryParse(static_cast<const osmium::Relation &>(*entity)));
            break;
        default:
            break;
        }
    }
}

int32_t V8ScriptingContext::GetTurnPenalty(const double angle)
{
    if (has_turn_penalty_function) {
        v8::HandleScope scope(env.isolate);

        v8::Local<v8::Function> callback = get_turn_penalty.Get(env.isolate);
        if (!callback.IsEmpty())
        {
            constexpr const int argc = 1;
            v8::Local<v8::Value> argv[argc] = {env.new_number(angle)};
            return callback->Call(context->Global(), argc, argv)->ToInt32()->Value();
        }
    }

    return 0;
}

V8ScriptingContext::~V8ScriptingContext() {}

V8ScriptingEnvironment::V8ScriptingEnvironment(const char *argv0, const std::string &file_name)
    : file_name(file_name)
{
    // Initialize V8.
    v8::V8::InitializeICU();
    v8::V8::InitializeExternalStartupData(argv0);
    platform = v8::platform::CreateDefaultPlatform();
    v8::V8::InitializePlatform(platform);
    v8::V8::Initialize();

    util::SimpleLogger().Write() << "Using script " << file_name;
}

V8ScriptingEnvironment::~V8ScriptingEnvironment()
{
    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
    delete platform;
}

const ProfileProperties &V8ScriptingEnvironment::GetProfileProperties()
{
    return GetV8Context().properties;
}

static pthread_key_t key;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;

V8ScriptingContext &V8ScriptingEnvironment::GetV8Context()
{
    pthread_once(&key_once, [] {
        pthread_key_create(&key,
                           [](void *ctx) { delete reinterpret_cast<V8ScriptingContext *>(ctx); });
    });
    auto *ctx = reinterpret_cast<V8ScriptingContext *>(pthread_getspecific(key));
    if (!ctx)
    {
        ctx = new V8ScriptingContext(file_name);
        pthread_setspecific(key, ctx);
    }

    return *ctx;
}

void V8ScriptingEnvironment::ProcessElements(
    const std::vector<osmium::memory::Buffer::const_iterator> &osm_elements,
    const RestrictionParser &restriction_parser,
    tbb::concurrent_vector<std::pair<std::size_t, ExtractionNode>> &resulting_nodes,
    tbb::concurrent_vector<std::pair<std::size_t, ExtractionWay>> &resulting_ways,
    tbb::concurrent_vector<boost::optional<InputRestrictionContainer>> &resulting_restrictions)
{
    // parse OSM entities in parallel, store in resulting vectors
    tbb::parallel_for(tbb::blocked_range<std::size_t>(0, osm_elements.size()),
                      [&](const tbb::blocked_range<std::size_t> &range) {
                          GetV8Context().ProcessElements(range,
                                                         osm_elements,
                                                         restriction_parser,
                                                         resulting_nodes,
                                                         resulting_ways,
                                                         resulting_restrictions);
                      });
}

std::vector<std::string> V8ScriptingEnvironment::GetNameSuffixList()
{
    return GetV8Context().GetNameSuffixList();
}

std::vector<std::string> V8ScriptingEnvironment::GetExceptions()
{
    return GetV8Context().GetExceptions();
}

void V8ScriptingEnvironment::SetupSources() { auto &context = GetV8Context(); }

int32_t V8ScriptingEnvironment::GetTurnPenalty(const double angle)
{
    return GetV8Context().GetTurnPenalty(angle);
}

void V8ScriptingEnvironment::ProcessSegment(const osrm::util::Coordinate &source,
                                            const osrm::util::Coordinate &target,
                                            double distance,
                                            InternalExtractorEdge::WeightData &weight)
{
    auto &context = GetV8Context();
    if (context.has_segment_function)
    {
    }
}
}
}
