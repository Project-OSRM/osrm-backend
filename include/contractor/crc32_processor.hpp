#ifndef ITERATOR_BASED_CRC32_H
#define ITERATOR_BASED_CRC32_H

#if defined(__x86_64__) && !defined(__MINGW64__)
#include <cpuid.h>
#endif

#include <boost/crc.hpp> // for boost::crc_32_type

#include <iterator>

namespace osrm
{
namespace contractor
{

class IteratorbasedCRC32
{
  public:
    bool UsingHardware() const { return use_hardware_implementation; }

    IteratorbasedCRC32() : crc(0) { use_hardware_implementation = DetectHardwareSupport(); }

    template <class Iterator> unsigned operator()(Iterator iter, const Iterator end)
    {
        unsigned crc = 0;
        while (iter != end)
        {
            using value_type = typename std::iterator_traits<Iterator>::value_type;
            const char *data = reinterpret_cast<const char *>(&(*iter));

            if (use_hardware_implementation)
            {
                crc = ComputeInHardware(data, sizeof(value_type));
            }
            else
            {
                crc = ComputeInSoftware(data, sizeof(value_type));
            }
            ++iter;
        }
        return crc;
    }

  private:
    bool DetectHardwareSupport() const
    {
        static const int sse42_bit = 0x00100000;
        const unsigned ecx = cpuid();
        const bool sse42_found = (ecx & sse42_bit) != 0;
        return sse42_found;
    }

    unsigned ComputeInSoftware(const char *str, unsigned len)
    {
        crc_processor.process_bytes(str, len);
        return crc_processor.checksum();
    }

    // adapted from http://byteworm.com/2010/10/13/crc32/
    unsigned ComputeInHardware(const char *str, unsigned len)
    {
#if defined(__x86_64__)
        unsigned q = len / sizeof(unsigned);
        unsigned r = len % sizeof(unsigned);
        unsigned *p = (unsigned *)str;

        // crc=0;
        while (q--)
        {
            __asm__ __volatile__(".byte 0xf2, 0xf, 0x38, 0xf1, 0xf1;"
                                 : "=S"(crc)
                                 : "0"(crc), "c"(*p));
            ++p;
        }

        str = reinterpret_cast<char *>(p);
        while (r--)
        {
            __asm__ __volatile__(".byte 0xf2, 0xf, 0x38, 0xf1, 0xf1;"
                                 : "=S"(crc)
                                 : "0"(crc), "c"(*str));
            ++str;
        }
#else
        (void)str;
        (void)len;
#endif
        return crc;
    }

    inline unsigned cpuid() const
    {
        unsigned eax = 0, ebx = 0, ecx = 0, edx = 0;
        // on X64 this calls hardware cpuid(.) instr. otherwise a dummy impl.
        __get_cpuid(1, &eax, &ebx, &ecx, &edx);
        return ecx;
    }

#if defined(__MINGW64__) || defined(_MSC_VER) || !defined(__x86_64__)
    inline void __get_cpuid(int /*param*/,
                            unsigned * /*eax*/,
                            unsigned * /*ebx*/,
                            unsigned *ecx,
                            unsigned * /*edx*/) const
    {
        *ecx = 0;
    }
#endif

    boost::crc_optimal<32, 0x1EDC6F41, 0x0, 0x0, true, true> crc_processor;
    unsigned crc;
    bool use_hardware_implementation;
};

struct RangebasedCRC32
{
    template <typename Iteratable> unsigned operator()(const Iteratable &iterable)
    {
        return crc32(std::begin(iterable), std::end(iterable));
    }

    bool UsingHardware() const { return crc32.UsingHardware(); }

  private:
    IteratorbasedCRC32 crc32;
};
}
}

#endif /* ITERATOR_BASED_CRC32_H */
