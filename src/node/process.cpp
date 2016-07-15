#include <nan.h>

#include "extractor/extraction_helper_functions.hpp"
#include "extractor/extraction_node.hpp"
#include "extractor/extraction_way.hpp"
#include "extractor/extractor.hpp"
#include "extractor/extractor_config.hpp"
#include "extractor/restriction_parser.hpp"
#include "extractor/scripting_environment_lua.hpp"
#include "extractor/travel_mode.hpp"

#include "util/simple_logger.hpp"

#include <osmium/osm.hpp>

#include <variant/variant.hpp>

#include <tbb/task_scheduler_init.h>

#include <future>

namespace
{

std::string toString(Nan::MaybeLocal<v8::Value> value, const std::string &def = {})
{
    if (value.IsEmpty() || value.ToLocalChecked()->IsUndefined())
    {
        return def;
    }
    else
    {
        v8::String::Utf8Value utfValue(value.ToLocalChecked()->ToString());
        return {*utfValue, static_cast<size_t>(utfValue.length())};
    }
}

std::string
toString(Nan::NAN_METHOD_ARGS_TYPE info, const size_t index, const std::string &def = {})
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

double toNumber(Nan::MaybeLocal<v8::Value> value, double def = 0)
{
    if (value.IsEmpty() || value.ToLocalChecked()->IsUndefined())
    {
        return def;
    }
    else
    {
        return value.ToLocalChecked()->ToNumber()->Value();
    }
}

int32_t toInteger(Nan::NAN_METHOD_ARGS_TYPE info, const size_t index, const int32_t def = 0)
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

bool toBoolean(Nan::MaybeLocal<v8::Value> value, bool def = 0)
{
    if (value.IsEmpty() || value.ToLocalChecked()->IsUndefined())
    {
        return def;
    }
    else
    {
        return value.ToLocalChecked()->ToBoolean()->Value();
    }
}
}

namespace osrm
{
namespace node
{

Nan::Persistent<v8::Function> osmiumNode;
Nan::Persistent<v8::Function> osmiumWay;
Nan::Persistent<v8::Function> extractionNode;
Nan::Persistent<v8::Function> extractionWay;

class ExtractWorker final : public Nan::AsyncWorker, public extractor::ScriptingEnvironment
{
  public:
    ExtractWorker(Nan::Callback *callback, v8::Local<v8::Object> profile)
        : Nan::AsyncWorker(callback)
    {
        async = new uv_async_t();
        uv_async_init(uv_default_loop(), async, Get);
        async->data = this;

        // ProfileProperties
        properties.SetTrafficSignalPenalty(
            toNumber(Nan::Get(profile, Nan::New("trafficSignalPenalty").ToLocalChecked()),
                     properties.GetTrafficSignalPenalty()));
        properties.SetUturnPenalty(
            toNumber(Nan::Get(profile, Nan::New("uTurnPenalty").ToLocalChecked()),
                     properties.GetUturnPenalty()));
        properties.continue_straight_at_waypoint =
            toBoolean(Nan::Get(profile, Nan::New("continueStraightAtWaypoint").ToLocalChecked()),
                      properties.continue_straight_at_waypoint);
        properties.use_turn_restrictions =
            toBoolean(Nan::Get(profile, Nan::New("useTurnRestrictions").ToLocalChecked()),
                      properties.use_turn_restrictions);

        // Retain the profile.
        persistentHandle.Reset(profile);
    }
    ~ExtractWorker() override
    {
        uv_close(reinterpret_cast<uv_handle_t *>(async),
                 [](uv_handle_t *handle) { delete reinterpret_cast<uv_async_t *>(handle); });
    }

    // Executed inside the worker-thread.
    void Execute() override
    {
        util::LogPolicy::GetInstance().Unmute();
        if (config.profile_path.empty())
        {
            util::SimpleLogger().Write() << "Using JavaScript profile";
            extractor::Extractor(std::move(config)).run(*this);
        }
        else
        {
            extractor::LuaScriptingEnvironment scripting_environment(
                config.profile_path.string().c_str());
            extractor::Extractor(std::move(config)).run(scripting_environment);
        }
    }

    // Executed when the async work is complete
    void HandleOKCallback() override
    {
        Nan::HandleScope scope;

        if (!callback->IsEmpty())
        {
            constexpr const int argc = 1;
            v8::Local<v8::Value> argv[argc] = {Nan::Null()};
            callback->Call(argc, argv);
        }
    }

    extractor::ProfileProperties properties;
    extractor::ExtractorConfig config;

  private:
    const extractor::ProfileProperties &GetProfileProperties() override;

    struct SetupSourcesParameters
    {
    };
    void SetupSources(SetupSourcesParameters &);
    void SetupSources() override;

    struct GetNameSuffixListParameters
    {
        std::unordered_set<std::string> result;
    };
    void GetNameSuffixList(GetNameSuffixListParameters &);
    std::unordered_set<std::string> GetNameSuffixList() override;

    struct GetExceptionsParameters
    {
        std::vector<std::string> result;
    };
    void GetExceptions(GetExceptionsParameters &);
    std::vector<std::string> GetExceptions() override;

    struct ProcessElementsParameters
    {
        const std::vector<osmium::memory::Buffer::const_iterator> &osm_elements;
        const extractor::RestrictionParser &restriction_parser;
        tbb::concurrent_vector<std::pair<std::size_t, extractor::ExtractionNode>> &resulting_nodes;
        tbb::concurrent_vector<std::pair<std::size_t, extractor::ExtractionWay>> &resulting_ways;
        tbb::concurrent_vector<boost::optional<extractor::InputRestrictionContainer>>
            &resulting_restrictions;
    };
    void ProcessElements(ProcessElementsParameters &);
    void ProcessElements(
        const std::vector<osmium::memory::Buffer::const_iterator> &osm_elements,
        const extractor::RestrictionParser &restriction_parser,
        tbb::concurrent_vector<std::pair<std::size_t, extractor::ExtractionNode>> &resulting_nodes,
        tbb::concurrent_vector<std::pair<std::size_t, extractor::ExtractionWay>> &resulting_ways,
        tbb::concurrent_vector<boost::optional<extractor::InputRestrictionContainer>>
            &resulting_restrictions) override;

    struct ProcessTurnPenaltiesParameters
    {
        std::vector<float> &angles;
    };
    void ProcessTurnPenalties(ProcessTurnPenaltiesParameters &);
    void ProcessTurnPenalties(std::vector<float> &angles) override;

    using ParameterType = mapbox::util::variant<SetupSourcesParameters,
                                                GetNameSuffixListParameters,
                                                GetExceptionsParameters,
                                                ProcessElementsParameters,
                                                ProcessTurnPenaltiesParameters>;
    ParameterType parameters;
    std::promise<void> result;
    uv_async_t *async = nullptr;
    static NAUV_WORK_CB(Get);

    void ProcessSegment(const osrm::util::Coordinate &source,
                        const osrm::util::Coordinate &target,
                        double distance,
                        extractor::InternalExtractorEdge::WeightData &weight) override;
};

const extractor::ProfileProperties &ExtractWorker::GetProfileProperties() { return properties; }

NAUV_WORK_CB(ExtractWorker::Get)
{
    Nan::HandleScope scope;
    auto worker = static_cast<ExtractWorker *>(async->data);

    if (worker->parameters.is<ProcessElementsParameters>())
    {
        worker->ProcessElements(worker->parameters.get<ProcessElementsParameters>());
    }
    else if (worker->parameters.is<ProcessTurnPenaltiesParameters>())
    {
        worker->ProcessTurnPenalties(worker->parameters.get<ProcessTurnPenaltiesParameters>());
    }
    else if (worker->parameters.is<SetupSourcesParameters>())
    {
        worker->SetupSources(worker->parameters.get<SetupSourcesParameters>());
    }
    else if (worker->parameters.is<GetNameSuffixListParameters>())
    {
        worker->GetNameSuffixList(worker->parameters.get<GetNameSuffixListParameters>());
    }
    else if (worker->parameters.is<GetExceptionsParameters>())
    {
        worker->GetExceptions(worker->parameters.get<GetExceptionsParameters>());
    }
}

void ExtractWorker::SetupSources()
{
    parameters = SetupSourcesParameters{};
    result = {};
    uv_async_send(async);
    result.get_future().get();
}

void ExtractWorker::SetupSources(SetupSourcesParameters &)
{
    v8::Local<v8::Value> callback = GetFromPersistent("setupSources");
    if (callback->IsFunction())
    {
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), callback.As<v8::Function>(), 0, {});
    }

    result.set_value();
}

std::unordered_set<std::string> ExtractWorker::GetNameSuffixList()
{
    parameters = GetNameSuffixListParameters{};
    result = {};
    uv_async_send(async);
    result.get_future().get();
    return parameters.get<GetNameSuffixListParameters>().result;
}

void ExtractWorker::GetNameSuffixList(GetNameSuffixListParameters &params)
{
    v8::Local<v8::Value> callback = GetFromPersistent("getNameSuffixList");
    if (callback->IsFunction())
    {
        auto retval = Nan::MakeCallback(
            Nan::GetCurrentContext()->Global(), callback.As<v8::Function>(), 0, {});

        if (retval->IsArray())
        {
            auto array = retval.As<v8::Array>();
            for (uint32_t i = 0; i < array->Length(); i++)
            {
                auto value = Nan::Get(array, i);
                if (!value.IsEmpty() && !value.ToLocalChecked()->IsUndefined())
                {
                    v8::String::Utf8Value utfValue(value.ToLocalChecked()->ToString());
                    std::string suffix{*utfValue, static_cast<size_t>(utfValue.length())};
                    boost::algorithm::to_lower(suffix);
                    params.result.emplace(std::move(suffix));
                }
            }
        }
    }

    result.set_value();
}

std::vector<std::string> ExtractWorker::GetExceptions()
{
    parameters = GetExceptionsParameters{};
    result = {};
    uv_async_send(async);
    result.get_future().get();
    return parameters.get<GetExceptionsParameters>().result;
}

void ExtractWorker::GetExceptions(GetExceptionsParameters &params)
{
    v8::Local<v8::Value> callback = GetFromPersistent("getExceptions");
    if (callback->IsFunction())
    {
        auto retval = Nan::MakeCallback(
            Nan::GetCurrentContext()->Global(), callback.As<v8::Function>(), 0, {});

        if (retval->IsArray())
        {
            auto array = retval.As<v8::Array>();
            for (uint32_t i = 0; i < array->Length(); i++)
            {
                auto value = Nan::Get(array, i);
                if (!value.IsEmpty() && !value.ToLocalChecked()->IsUndefined())
                {
                    v8::String::Utf8Value utfValue(value.ToLocalChecked()->ToString());
                    params.result.emplace_back(*utfValue, static_cast<size_t>(utfValue.length()));
                }
            }
        }
    }

    result.set_value();
}

void ExtractWorker::ProcessElements(
    const std::vector<osmium::memory::Buffer::const_iterator> &osm_elements,
    const extractor::RestrictionParser &restriction_parser,
    tbb::concurrent_vector<std::pair<std::size_t, extractor::ExtractionNode>> &resulting_nodes,
    tbb::concurrent_vector<std::pair<std::size_t, extractor::ExtractionWay>> &resulting_ways,
    tbb::concurrent_vector<boost::optional<extractor::InputRestrictionContainer>>
        &resulting_restrictions)
{
    parameters = ProcessElementsParameters{
        osm_elements, restriction_parser, resulting_nodes, resulting_ways, resulting_restrictions,
    };
    result = {};
    uv_async_send(async);
    result.get_future().get();
}

void ExtractWorker::ProcessElements(ProcessElementsParameters &params)
{
    v8::Local<v8::Value> processNode = GetFromPersistent("processNode");
    const bool hasProcessNode = processNode->IsFunction();
    v8::Local<v8::Value> processWay = GetFromPersistent("processWay");
    const bool hasProcessWay = processWay->IsFunction();

    extractor::ExtractionNode result_node;
    extractor::ExtractionWay result_way;

    v8::Local<v8::Function> nodeConstructor = Nan::New(osmiumNode);
    v8::Local<v8::Function> extractionNodeConstructor = Nan::New(extractionNode);
    v8::Local<v8::Object> resultNode = extractionNodeConstructor->NewInstance();
    resultNode->SetInternalField(0, v8::External::New(v8::Isolate::GetCurrent(), &result_node));

    v8::Local<v8::Function> wayConstructor = Nan::New(osmiumWay);
    v8::Local<v8::Function> extractionWayConstructor = Nan::New(extractionWay);
    v8::Local<v8::Object> resultWay = extractionWayConstructor->NewInstance();
    resultWay->SetInternalField(0, v8::External::New(v8::Isolate::GetCurrent(), &result_way));

    std::size_t x = 0;
    for (auto it = params.osm_elements.begin(), end = params.osm_elements.end(); it != end;
         ++it, ++x)
    {
        Nan::HandleScope scope;
        const auto entity = *it;

        switch (entity->type())
        {
        case osmium::item_type::node:
            result_node.clear();

            if (hasProcessNode)
            {
                v8::Local<v8::Object> node = nodeConstructor->NewInstance();
                node->SetInternalField(
                    0,
                    v8::External::New(v8::Isolate::GetCurrent(),
                                      &const_cast<osmium::OSMEntity &>(*entity)));
                constexpr const int argc = 2;
                v8::Local<v8::Value> argv[argc] = {node, resultNode};
                Nan::MakeCallback(
                    Nan::GetCurrentContext()->Global(), processNode.As<v8::Function>(), argc, argv);
            }

            params.resulting_nodes.push_back(std::make_pair(x, std::move(result_node)));
            break;
        case osmium::item_type::way:
            result_way.clear();

            if (hasProcessWay)
            {
                v8::Local<v8::Object> way = wayConstructor->NewInstance();
                way->SetInternalField(0,
                                      v8::External::New(v8::Isolate::GetCurrent(),
                                                        &const_cast<osmium::OSMEntity &>(*entity)));
                constexpr const int argc = 2;
                v8::Local<v8::Value> argv[argc] = {way, resultWay};
                Nan::MakeCallback(
                    Nan::GetCurrentContext()->Global(), processWay.As<v8::Function>(), argc, argv);
            }

            params.resulting_ways.push_back(std::make_pair(x, std::move(result_way)));
            break;
        case osmium::item_type::relation:
            params.resulting_restrictions.push_back(
                params.restriction_parser.TryParse(static_cast<const osmium::Relation &>(*entity)));
            break;
        default:
            break;
        }
    }

    result.set_value();
}

void ExtractWorker::ProcessTurnPenalties(std::vector<float> &angles)
{
    parameters = ProcessTurnPenaltiesParameters{angles};
    result = {};
    uv_async_send(async);
    result.get_future().get();
}

void ExtractWorker::ProcessTurnPenalties(ProcessTurnPenaltiesParameters &params)
{
    v8::Local<v8::Value> ProcessTurnPenalties = GetFromPersistent("getTurnPenalty");
    if (ProcessTurnPenalties->IsFunction())
    {
        for (auto &angle : params.angles)
        {
            constexpr const int argc = 1;
            v8::Local<v8::Value> argv[argc] = {Nan::New(angle)};
            auto retval = Nan::MakeCallback(Nan::GetCurrentContext()->Global(),
                                            ProcessTurnPenalties.As<v8::Function>(),
                                            argc,
                                            argv);
            angle = retval->ToNumber()->Value();
        }
    }
    result.set_value();
}

void ExtractWorker::ProcessSegment(const osrm::util::Coordinate &source,
                                   const osrm::util::Coordinate &target,
                                   double distance,
                                   extractor::InternalExtractorEdge::WeightData &weight)
{
    // TODO
    (void)source;
    (void)target;
    (void)distance;
    (void)weight;
}

NAN_METHOD(Extract)
{
    Nan::HandleScope scope;

    if (info.Length() < 1 || !info[0]->IsObject())
    {
        Nan::ThrowTypeError("first argument must be a configuration object");
        return;
    }
    if (info.Length() < 2 || !info[1]->IsObject())
    {
        Nan::ThrowTypeError("second argument must be a profile object");
        return;
    }

    v8::Local<v8::Object> config = info[0].As<v8::Object>();
    v8::Local<v8::Object> profile = info[1].As<v8::Object>();

    auto callback = std::make_unique<Nan::Callback>(info[2].As<v8::Function>());
    auto worker = std::make_unique<ExtractWorker>(callback.release(), profile);

    // ExtractorConfig
    worker->config.profile_path =
        toString(Nan::Get(config, Nan::New("profilePath").ToLocalChecked()));
    worker->config.input_path = toString(Nan::Get(config, Nan::New("inputPath").ToLocalChecked()));
    worker->config.requested_num_threads =
        toNumber(Nan::Get(config, Nan::New("numThreads").ToLocalChecked()));
    worker->config.small_component_size =
        toNumber(Nan::Get(config, Nan::New("smallComponentSize").ToLocalChecked()), 1000);
    worker->config.generate_edge_lookup =
        toBoolean(Nan::Get(config, Nan::New("generateEdgeLookup").ToLocalChecked()), false);

    if (worker->config.input_path.empty())
    {
        Nan::ThrowTypeError("inputPath is missing from configuration object");
        return;
    }

    // TODO: Make configurable/overrideable
    worker->config.UseDefaultOutputNames();

    if (worker->config.requested_num_threads <= 0)
    {
        worker->config.requested_num_threads = tbb::task_scheduler_init::default_num_threads();
    }

    Nan::AsyncQueueWorker(worker.release());
}

NAN_METHOD(DurationIsValid)
{
    Nan::HandleScope scope;
    info.GetReturnValue().Set(extractor::durationIsValid(toString(info, 0)));
}

NAN_METHOD(ParseDuration)
{
    Nan::HandleScope scope;
    info.GetReturnValue().Set(extractor::parseDuration(toString(info, 0)));
}

NAN_METHOD(TrimLaneString)
{
    Nan::HandleScope scope;
    info.GetReturnValue().Set(
        Nan::New(
            extractor::trimLaneString(toString(info, 0), toInteger(info, 1), toInteger(info, 2)))
            .ToLocalChecked());
}

template <typename T> class WrapClass
{
  public:
    WrapClass(const char *name)
    {
        tpl = Nan::New<v8::FunctionTemplate>();
        tpl->SetClassName(Nan::New(name).ToLocalChecked());
        itpl = tpl->InstanceTemplate();
        itpl->SetInternalFieldCount(1);
    }

    void method(const char *name, Nan::FunctionCallback cb)
    {
        Nan::SetPrototypeMethod(tpl, name, cb);
    }

    void property(const char *name, Nan::GetterCallback getter)
    {
        Nan::SetAccessor(itpl,
                         Nan::New(name).ToLocalChecked(),
                         getter,
                         0,
                         v8::Local<v8::Value>(),
                         v8::AccessControl::DEFAULT,
                         static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::ReadOnly |
                                                            v8::PropertyAttribute::DontDelete));
    }

    void property(const char *name, Nan::GetterCallback getter, Nan::SetterCallback setter)
    {
        Nan::SetAccessor(itpl,
                         Nan::New(name).ToLocalChecked(),
                         getter,
                         setter,
                         v8::Local<v8::Value>(),
                         v8::AccessControl::DEFAULT,
                         static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontDelete));
    }

    template <typename U> static T &Unwrap(const U &info)
    {
        v8::Local<v8::Object> self = info.Holder();
        v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
        return *static_cast<T *>(wrap->Value());
    }

    v8::Local<v8::Function> get() { return tpl->GetFunction(); }

  private:
    v8::Local<v8::FunctionTemplate> tpl;
    v8::Local<v8::ObjectTemplate> itpl;
};

void freeze(v8::Local<v8::Value> obj)
{
    v8::Local<v8::Object> global = Nan::GetCurrentContext()->Global();
    v8::Local<v8::Object> object =
        Nan::Get(global, Nan::New("Object").ToLocalChecked()).ToLocalChecked().As<v8::Object>();
    v8::Local<v8::Value> freeze =
        Nan::Get(object, Nan::New("freeze").ToLocalChecked()).ToLocalChecked();
    Nan::MakeCallback(global, freeze.As<v8::Function>(), 1, &obj);
}

NAN_MODULE_INIT(Initialize)
{
    Nan::Export(target, "extract", Extract);
    Nan::Export(target, "durationIsValid", DurationIsValid);
    Nan::Export(target, "parseDuration", ParseDuration);
    Nan::Export(target, "trimLaneString", TrimLaneString);

    v8::Local<v8::Object> mode = Nan::New<v8::Object>();
    Nan::Set(mode, Nan::New("inaccessible").ToLocalChecked(), Nan::New(TRAVEL_MODE_INACCESSIBLE));
    Nan::Set(mode, Nan::New("driving").ToLocalChecked(), Nan::New(TRAVEL_MODE_DRIVING));
    Nan::Set(mode, Nan::New("cycling").ToLocalChecked(), Nan::New(TRAVEL_MODE_CYCLING));
    Nan::Set(mode, Nan::New("walking").ToLocalChecked(), Nan::New(TRAVEL_MODE_WALKING));
    Nan::Set(mode, Nan::New("ferry").ToLocalChecked(), Nan::New(TRAVEL_MODE_FERRY));
    Nan::Set(mode, Nan::New("train").ToLocalChecked(), Nan::New(TRAVEL_MODE_TRAIN));
    Nan::Set(mode, Nan::New("pushingBike").ToLocalChecked(), Nan::New(TRAVEL_MODE_PUSHING_BIKE));
    Nan::Set(mode, Nan::New("stepsUp").ToLocalChecked(), Nan::New(TRAVEL_MODE_STEPS_UP));
    Nan::Set(mode, Nan::New("stepsDown").ToLocalChecked(), Nan::New(TRAVEL_MODE_STEPS_DOWN));
    Nan::Set(mode, Nan::New("riverUp").ToLocalChecked(), Nan::New(TRAVEL_MODE_RIVER_UP));
    Nan::Set(mode, Nan::New("riverDown").ToLocalChecked(), Nan::New(TRAVEL_MODE_RIVER_DOWN));
    Nan::Set(mode, Nan::New("route").ToLocalChecked(), Nan::New(TRAVEL_MODE_ROUTE));
    freeze(mode);
    Nan::Set(target, Nan::New<v8::String>("mode").ToLocalChecked(), mode);

    {
        using Class = WrapClass<const osmium::Node>;
        Class node("Node");

        node.method("tag", [](const auto &info) {
            if (info.Length() < 1)
            {
                Nan::ThrowTypeError("first argument must be a tag name");
                return;
            }

            v8::String::Utf8Value utfValue(info[0]->ToString());
            auto value = Class::Unwrap(info).get_value_by_key(*utfValue);
            if (value)
            {
                info.GetReturnValue().Set(Nan::New(value).ToLocalChecked());
            }
        });

        node.property("id", [](auto, const auto &info) {
            info.GetReturnValue().Set(static_cast<double>(Class::Unwrap(info).id()));
        });

        node.property("lat", [](auto, const auto &info) {
            info.GetReturnValue().Set(static_cast<double>(Class::Unwrap(info).location().lat()));
        });

        node.property("lon", [](auto, const auto &info) {
            info.GetReturnValue().Set(static_cast<double>(Class::Unwrap(info).location().lon()));
        });

        osmiumNode.Reset(node.get());
    }

    {
        using Class = WrapClass<const osmium::Way>;
        Class way("Way");

        way.method("tag", [](const auto &info) {
            if (info.Length() < 1)
            {
                Nan::ThrowTypeError("first argument must be a tag name");
                return;
            }

            const v8::String::Utf8Value utfValue(info[0]->ToString());
            auto value = Class::Unwrap(info).get_value_by_key(*utfValue);
            if (value)
            {
                info.GetReturnValue().Set(Nan::New(value).ToLocalChecked());
            }
        });

        way.property("id", [](auto, const auto &info) {
            info.GetReturnValue().Set(static_cast<double>(Class::Unwrap(info).id()));
        });

        // TODO: get_nodes_for_way

        osmiumWay.Reset(way.get());
    }

    {
        using Class = WrapClass<extractor::ExtractionNode>;
        Class node("ResultNode");

        node.property("trafficLights",
                      [](auto, const auto &info) {
                          info.GetReturnValue().Set(Class::Unwrap(info).traffic_lights);
                      },
                      [](auto, auto value, const auto &info) {
                          Class::Unwrap(info).traffic_lights = value->ToBoolean()->Value();
                      });

        node.property(
            "barrier",
            [](auto, const auto &info) { info.GetReturnValue().Set(Class::Unwrap(info).barrier); },
            [](auto, auto value, const auto &info) {
                Class::Unwrap(info).barrier = value->ToBoolean()->Value();
            });

        extractionNode.Reset(node.get());
    }

    {
        using Class = WrapClass<extractor::ExtractionWay>;
        Class way("ResultWay");

        way.property("forwardSpeed",
                     [](auto, const auto &info) {
                         info.GetReturnValue().Set(Class::Unwrap(info).forward_speed);
                     },
                     [](auto, auto value, const auto &info) {
                         Class::Unwrap(info).forward_speed = value->ToNumber()->Value();
                     });

        way.property("backwardSpeed",
                     [](auto, const auto &info) {
                         info.GetReturnValue().Set(Class::Unwrap(info).backward_speed);
                     },
                     [](auto, auto value, const auto &info) {
                         Class::Unwrap(info).backward_speed = value->ToNumber()->Value();
                     });

        way.property(
            "duration",
            [](auto, const auto &info) { info.GetReturnValue().Set(Class::Unwrap(info).duration); },
            [](auto, auto value, const auto &info) {
                Class::Unwrap(info).duration = value->ToNumber()->Value();
            });

        way.property(
            "name",
            [](auto, const auto &info) {
                info.GetReturnValue().Set(Nan::New(Class::Unwrap(info).name).ToLocalChecked());
            },
            [](auto, auto value, const auto &info) {
                const v8::String::Utf8Value utfValue(value->ToString());
                Class::Unwrap(info).name = {*utfValue, static_cast<size_t>(utfValue.length())};
            });

        way.property("pronunciation",
                     [](auto, const auto &info) {
                         info.GetReturnValue().Set(
                             Nan::New(Class::Unwrap(info).pronunciation).ToLocalChecked());
                     },
                     [](auto, auto value, const auto &info) {
                         const v8::String::Utf8Value utfValue(value->ToString());
                         Class::Unwrap(info).pronunciation = {
                             *utfValue, static_cast<size_t>(utfValue.length())};
                     });

        way.property("destinations",
                     [](auto, const auto &info) {
                         info.GetReturnValue().Set(
                             Nan::New(Class::Unwrap(info).destinations).ToLocalChecked());
                     },
                     [](auto, auto value, const auto &info) {
                         const v8::String::Utf8Value utfValue(value->ToString());
                         Class::Unwrap(info).destinations = {
                             *utfValue, static_cast<size_t>(utfValue.length())};
                     });

        way.property("turnLanesForward",
                     [](auto, const auto &info) {
                         info.GetReturnValue().Set(
                             Nan::New(Class::Unwrap(info).turn_lanes_forward).ToLocalChecked());
                     },
                     [](auto, auto value, const auto &info) {
                         const v8::String::Utf8Value utfValue(value->ToString());
                         Class::Unwrap(info).turn_lanes_forward = {
                             *utfValue, static_cast<size_t>(utfValue.length())};
                     });

        way.property("turnLanesBackward",
                     [](auto, const auto &info) {
                         info.GetReturnValue().Set(
                             Nan::New(Class::Unwrap(info).turn_lanes_backward).ToLocalChecked());
                     },
                     [](auto, auto value, const auto &info) {
                         const v8::String::Utf8Value utfValue(value->ToString());
                         Class::Unwrap(info).turn_lanes_backward = {
                             *utfValue, static_cast<size_t>(utfValue.length())};
                     });

        way.property("roundabout",
                     [](auto, const auto &info) {
                         info.GetReturnValue().Set(Class::Unwrap(info).roundabout);
                     },
                     [](auto, auto value, const auto &info) {
                         Class::Unwrap(info).roundabout = value->ToBoolean()->Value();
                     });

        way.property("isAccessRestricted",
                     [](auto, const auto &info) {
                         info.GetReturnValue().Set(Class::Unwrap(info).is_access_restricted);
                     },
                     [](auto, auto value, const auto &info) {
                         Class::Unwrap(info).is_access_restricted = value->ToBoolean()->Value();
                     });

        way.property("isStartpoint",
                     [](auto, const auto &info) {
                         info.GetReturnValue().Set(Class::Unwrap(info).is_startpoint);
                     },
                     [](auto, auto value, const auto &info) {
                         Class::Unwrap(info).is_startpoint = value->ToBoolean()->Value();
                     });

        way.property("forwardMode",
                     [](auto, const auto &info) {
                         info.GetReturnValue().Set(
                             static_cast<uint32_t>(Class::Unwrap(info).forward_travel_mode));
                     },
                     [](auto, auto value, const auto &info) {
                         Class::Unwrap(info).forward_travel_mode =
                             static_cast<extractor::TravelMode>(value->ToUint32()->Value());
                     });

        way.property("backwardMode",
                     [](auto, const auto &info) {
                         info.GetReturnValue().Set(
                             static_cast<uint32_t>(Class::Unwrap(info).backward_travel_mode));
                     },
                     [](auto, auto value, const auto &info) {
                         Class::Unwrap(info).backward_travel_mode =
                             static_cast<extractor::TravelMode>(value->ToUint32()->Value());
                     });

        extractionWay.Reset(way.get());
    }
}
}
}

NODE_MODULE(osrm_process, osrm::node::Initialize)
