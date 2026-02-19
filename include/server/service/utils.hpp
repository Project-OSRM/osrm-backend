#include "util/format.hpp"

namespace osrm::server::service
{

const constexpr char PARAMETER_SIZE_MISMATCH_MSG[] =
    "Number of elements in {} size {} does not match coordinate size {}";

template <typename ParamT>
bool constrainParamSize(const char *msg_template,
                        const char *name,
                        const ParamT &param,
                        const std::size_t target_size,
                        std::string &help)
{
    if (param.size() > 0 && param.size() != target_size)
    {
#ifdef OSRM_HAS_STD_FORMAT
        help = osrm::util::compat::format(
            osrm::util::compat::runtime_format(msg_template), name, param.size(), target_size);
#else
        help = osrm::util::compat::format(
            osrm::util::compat::runtime(msg_template), name, param.size(), target_size);
#endif
        return true;
    }
    return false;
}
} // namespace osrm::server::service
