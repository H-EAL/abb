#pragma once

namespace abb {
    
    namespace units {

        inline constexpr unsigned long long operator "" _B(unsigned long long N)
        {
            return N;
        }
        inline constexpr unsigned long long operator "" _KiB(unsigned long long N)
        {
            return N * 1024;
        }
        inline constexpr unsigned long long operator "" _MiB(unsigned long long N)
        {
            return N * 1024_KiB;
        }
        inline constexpr unsigned long long operator "" _GiB(unsigned long long N)
        {
            return N * 1024_MiB;
        }
        inline constexpr unsigned long long operator "" _KB(unsigned long long N)
        {
            return N * 1000;
        }
        inline constexpr unsigned long long operator "" _MB(unsigned long long N)
        {
            return N * 1000_KB;
        }
        inline constexpr unsigned long long operator "" _GB(unsigned long long N)
        {
            return N * 1000_MB;
        }
    
    } /*units*/

    using namespace units;

} /*abb*/
