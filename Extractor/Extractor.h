#ifndef EXTRACTOR_H_
#define EXTRACTOR_H_

#include "../Util/FingerPrint.h"

#include <boost/filesystem.hpp>


class ExtractorCallbacks;

/** \brief Class of 'extract' utility. */
class Extractor
{
    ExtractorCallbacks * extractor_callbacks;
    FingerPrint fingerprint;
    const char* git_description;

protected:
    unsigned requested_num_threads;
    boost::filesystem::path config_file_path;
    boost::filesystem::path input_path;
    boost::filesystem::path profile_path;

    std::string output_file_name;
    std::string restriction_file_name;
    bool file_has_pbf_format;

protected:
    /** \brief Parses "extractor's" command line arguments */
    bool ParseArguments(int argc, char *argv[]);

    /** \brief Parses config file, if present in options */
    void GenerateOutputFilesNames();

public:
    explicit Extractor(const char* git_desc);
    Extractor(const Extractor&) = delete;
    virtual ~Extractor();

    int Execute(int argc, char *argv[]);
};
#endif /* EXTRACTOR_H_ */
