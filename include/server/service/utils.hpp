#include <boost/format.hpp>

namespace osrm
{
namespace server
{
namespace service
{

const constexpr char PARAMETER_SIZE_MISMATCH_MSG[] =
    "Number of elements in %1% size %2% does not match coordinate size %3%";

template <typename ParamT>
bool constrainParamSize(const char *msg_template,
                        const char *name,
                        const ParamT &param,
                        const std::size_t target_size,
                        std::string &help)
{
    if (param.size() > 0 && param.size() != target_size)
    {
        help = (boost::format(msg_template) % name % param.size() % target_size).str();
        return true;
    }
    return false;
}
} // namespace service
} // namespace server
} // namespace osrm
