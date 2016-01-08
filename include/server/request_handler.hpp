#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include <string>

namespace osrm
{
namespace engine
{
class OSRM;
struct RouteParameters;
}
namespace server
{
template <typename Iterator, class HandlerT> struct APIGrammar;

namespace http
{
class reply;
struct request;
}

class RequestHandler
{

  public:
    using APIGrammarParser = APIGrammar<std::string::iterator, engine::RouteParameters>;

    RequestHandler();
    RequestHandler(const RequestHandler &) = delete;

    void handle_request(const http::request &current_request, http::reply &current_reply);
    void RegisterRoutingMachine(engine::OSRM *osrm);

  private:
    engine::OSRM *routing_machine;
};
}
}

#endif // REQUEST_HANDLER_HPP
