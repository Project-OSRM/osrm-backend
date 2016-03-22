#ifndef STORAGE_HPP
#define STORAGE_HPP

#include "storage/storage_config.hpp"

#include <boost/filesystem/path.hpp>

#include <string>

namespace osrm
{
namespace storage
{
class Storage
{
  public:
    Storage(StorageConfig config);
    int Run();

  private:
    StorageConfig config;
};
}
}

#endif
