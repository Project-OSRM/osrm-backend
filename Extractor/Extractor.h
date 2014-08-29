#ifndef EXTRACTOR_H_
#define EXTRACTOR_H_

#include <string>

#include <boost/filesystem.hpp>

/** \brief Class of 'extract' utility. */
struct Extractor
{
    int Run(int argc, char *argv[]);
};
#endif /* EXTRACTOR_H_ */
