#include <boost/format.hpp>

namespace osrm::server::service
{

const constexpr char PARAMETER_SIZE_MISMATCH_MSG[] =
    "Number of elements in %1% size %2% does not match %3% size %4%";

template <typename ParamT>
bool constrainParamSize(const char *msg_template,
                        const char *param_name,
                        const ParamT &param,
                        const char *target_name,
                        const std::size_t target_size,
                        std::string &help)
{
    if (param.size() > 0 && param.size() != target_size)
    {
        help = (boost::format(msg_template) % param_name % param.size() % target_name % target_size).str();
        return true;
    }
    return false;
}
} // namespace osrm::server::service
