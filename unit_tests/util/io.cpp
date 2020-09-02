#include "storage/io.hpp"
#include "storage/serialization.hpp"
#include "util/exception.hpp"
#include "util/fingerprint.hpp"
#include "util/typedefs.hpp"
#include "util/version.hpp"

#include <boost/test/unit_test.hpp>

#include <exception>
#include <numeric>
#include <string>

const static std::string IO_TMP_FILE = "test_io.tmp";
const static std::string IO_NONEXISTENT_FILE = "non_existent_test_io.tmp";
const static std::string IO_TOO_SMALL_FILE = "file_too_small_test_io.tmp";
const static std::string IO_CORRUPT_FINGERPRINT_FILE = "corrupt_fingerprint_file_test_io.tmp";
const static std::string IO_INCOMPATIBLE_FINGERPRINT_FILE =
    "incompatible_fingerprint_file_test_io.tmp";
const static std::string IO_TEXT_FILE = "plain_text_file.tmp";

using namespace osrm;

BOOST_AUTO_TEST_SUITE(osrm_io)

BOOST_AUTO_TEST_CASE(io_data)
{
    std::vector<int> data_in(53), data_out;
    std::iota(begin(data_in), end(data_in), 0);

    {
        osrm::storage::io::FileWriter outfile(IO_TMP_FILE,
                                              osrm::storage::io::FileWriter::GenerateFingerprint);
        outfile.WriteElementCount64(data_in.size());
        outfile.WriteFrom(data_in.data(), data_in.size());
    }

    osrm::storage::io::FileReader infile(IO_TMP_FILE,
                                         osrm::storage::io::FileReader::VerifyFingerprint);
    data_out.resize(infile.ReadElementCount64());
    infile.ReadInto(data_out.data(), data_out.size());

    BOOST_REQUIRE_EQUAL(data_in.size(), data_out.size());
    BOOST_CHECK_EQUAL_COLLECTIONS(data_out.begin(), data_out.end(), data_in.begin(), data_in.end());
}

BOOST_AUTO_TEST_CASE(io_buffered_data)
{
    std::vector<int> data_in(53), data_out;
    std::iota(begin(data_in), end(data_in), 0);

    std::string result;
    {
        storage::io::BufferWriter writer;
        storage::serialization::write(writer, data_in);
        result = writer.GetBuffer();
    }

    storage::io::BufferReader reader(result);
    storage::serialization::read(reader, data_out);

    BOOST_REQUIRE_EQUAL(data_in.size(), data_out.size());
    BOOST_CHECK_EQUAL_COLLECTIONS(data_out.begin(), data_out.end(), data_in.begin(), data_in.end());
}

BOOST_AUTO_TEST_CASE(io_nonexistent_file)
{
    try
    {
        osrm::storage::io::FileReader infile(IO_NONEXISTENT_FILE,
                                             osrm::storage::io::FileReader::VerifyFingerprint);
        BOOST_REQUIRE_MESSAGE(false, "Should not get here");
    }
    catch (const osrm::util::RuntimeError &e)
    {
        const std::string expected("Problem opening file: " + IO_NONEXISTENT_FILE);
        const std::string got(e.what());
        BOOST_REQUIRE(std::equal(expected.begin(), expected.end(), got.begin()));
        BOOST_REQUIRE(e.GetCode() == osrm::ErrorCode::FileOpenError);
    }
}

BOOST_AUTO_TEST_CASE(file_too_small)
{
    {
        std::vector<int> v(53);
        std::iota(begin(v), end(v), 0);

        {
            osrm::storage::io::FileWriter outfile(
                IO_TOO_SMALL_FILE, osrm::storage::io::FileWriter::GenerateFingerprint);
            outfile.WriteElementCount64(v.size());
            outfile.WriteFrom(v.data(), v.size());
        }

        std::fstream f(IO_TOO_SMALL_FILE);
        f.seekp(sizeof(osrm::util::FingerPrint), std::ios_base::beg);
        std::uint64_t badcount = 100;
        f.write(reinterpret_cast<char *>(&badcount), sizeof(badcount));
    }

    try
    {
        osrm::storage::io::FileReader infile(IO_TOO_SMALL_FILE,
                                             osrm::storage::io::FileReader::VerifyFingerprint);
        std::vector<int> buffer;
        buffer.resize(infile.ReadElementCount64());
        infile.ReadInto(buffer.data(), buffer.size());
        BOOST_REQUIRE_MESSAGE(false, "Should not get here");
    }
    catch (const osrm::util::RuntimeError &e)
    {
        const std::string expected("Unexpected end of file: " + IO_TOO_SMALL_FILE);
        const std::string got(e.what());
        BOOST_REQUIRE(std::equal(expected.begin(), expected.end(), got.begin()));
        BOOST_REQUIRE(e.GetCode() == osrm::ErrorCode::UnexpectedEndOfFile);
    }
}

BOOST_AUTO_TEST_CASE(io_corrupt_fingerprint)
{
    {
        std::vector<int> v(153);
        std::iota(begin(v), end(v), 0);

        osrm::storage::io::FileWriter outfile(IO_CORRUPT_FINGERPRINT_FILE,
                                              osrm::storage::io::FileWriter::HasNoFingerprint);

        outfile.WriteFrom(0xDEADBEEFCAFEFACE);
        outfile.WriteElementCount64(v.size());
        outfile.WriteFrom(v.data(), v.size());
    }

    try
    {
        osrm::storage::io::FileReader infile(IO_CORRUPT_FINGERPRINT_FILE,
                                             osrm::storage::io::FileReader::VerifyFingerprint);
        BOOST_REQUIRE_MESSAGE(false, "Should not get here");
    }
    catch (const osrm::util::RuntimeError &e)
    {
        const std::string expected("Fingerprint did not match the expected value: " +
                                   IO_CORRUPT_FINGERPRINT_FILE);
        const std::string got(e.what());
        BOOST_REQUIRE(std::equal(expected.begin(), expected.end(), got.begin()));
        BOOST_REQUIRE(e.GetCode() == osrm::ErrorCode::InvalidFingerprint);
    }
}

BOOST_AUTO_TEST_CASE(io_incompatible_fingerprint)
{
    {
        std::vector<int> v(153);
        std::iota(begin(v), end(v), 0);

        {
            osrm::storage::io::FileWriter outfile(IO_INCOMPATIBLE_FINGERPRINT_FILE,
                                                  osrm::storage::io::FileWriter::HasNoFingerprint);

            const auto fingerprint = osrm::util::FingerPrint::GetValid();
            outfile.WriteFrom(fingerprint);
            outfile.WriteElementCount64(v.size());
            outfile.WriteFrom(v.data(), v.size());
        }

        std::fstream f(IO_INCOMPATIBLE_FINGERPRINT_FILE);
        f.seekp(5, std::ios_base::beg); // Seek past `OSRN` and Major version byte
        std::uint8_t incompatibleminor = static_cast<std::uint8_t>(OSRM_VERSION_MAJOR) + 1;
        f.write(reinterpret_cast<char *>(&incompatibleminor), sizeof(incompatibleminor));
    }

    try
    {
        osrm::storage::io::FileReader infile(IO_INCOMPATIBLE_FINGERPRINT_FILE,
                                             osrm::storage::io::FileReader::VerifyFingerprint);
        BOOST_REQUIRE_MESSAGE(false, "Should not get here");
    }
    catch (const osrm::util::RuntimeError &e)
    {
        const std::string expected("Fingerprint did not match the expected value: " +
                                   IO_INCOMPATIBLE_FINGERPRINT_FILE);
        const std::string got(e.what());
        BOOST_REQUIRE(std::equal(expected.begin(), expected.end(), got.begin()));
        BOOST_REQUIRE(e.GetCode() == osrm::ErrorCode::InvalidFingerprint);
    }
}

BOOST_AUTO_TEST_SUITE_END()
