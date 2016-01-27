#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include <string>

namespace osrm
{
class OSRM;
namespace engine
{
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
    RequestHandler &operator=(const RequestHandler &) = delete;

    void handle_request(const http::request &current_request, http::reply &current_reply);
    void RegisterRoutingMachine(OSRM *osrm);

  private:
    OSRM *routing_machine;
};
}
}

#endif // REQUEST_HANDLER_HPP
