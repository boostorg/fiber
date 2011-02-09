
Version 1.42.0

New Libraries

     * Uuid: A universally unique identifier, from Andy Tompkins.

Updated Libraries

     * Asio:
          + Added a new HTTP Server 4 example illustrating the use of stackless coroutines with Asio.
          + Changed handler allocation and invocation to use boost::addressof to get the address of handler objects, rather than applying operator& directly (#2977).
          + Restricted MSVC buffer debugging workaround to 2008, as it causes a crash with 2010 beta 2 (#3796, #3822).
          + Fixed a problem with the lifetime of handler memory, where Windows needs the OVERLAPPED structure to be valid until both the initiating function call has returned and the completion packet has been delivered.
          + Don't block signals while performing system calls, but instead restart the calls if they are interrupted.
          + Documented the guarantee made by strand objects with respect to order of handler invocation.
          + Changed strands to use a pool of implementations, to make copying of strands cheaper.
          + Ensured that kqueue support is enabled for BSD platforms (#3626).
          + Added a boost_ prefix to the extern "C" thread entry point function (#3809).
          + In getaddrinfo emulation, only check the socket type (SOCK_STREAM or SOCK_DGRAM) if a service name has been specified. This should allow the emulation to work with raw sockets.
          + Added a workaround for some broken Windows firewalls that make a socket appear bound to 0.0.0.0 when it is in fact bound to 127.0.0.1.
          + Applied a fix for reported excessive CPU usage under Solaris (#3670).
          + Added some support for platforms that use older compilers such as g++ 2.95 (#3743).
     * Circular Buffer:
          + Added methods erase_begin(size_type) and erase_end(size_type) with constant complexity for such types of stored elements which do not need an explicit destruction e.g. int or double.
          + Similarly changed implementation of the clear() method and the destructor so their complexity is now constant for such types of stored elements which do not require an explicit destruction (the complexity for other types remains linear).
     * Fusion:
          + The accumulator is the first argument to the functor of fusion::fold and fusion::accumulate (#2355).
          + Added support for associative iterators and views (#3473).
     * Graph:
          + Removed old interface to compressed_sparse_row_graph, making new interface the default.
     * Integer:
          + Reverted Trunk to release branch state (i.e. a "known good state").
          + Fixed issues: 653, 3084, 3177, 3180, 3568, 3657, 2134.
          + Added long long support to boost::static_log2, boost::static_signed_min, boost::static_signed_max, boost::static_unsigned_minboost::static_unsigned_max, when available.
          + The argument type and the result type of boost::static_signed_min etc are now typedef'd. Formerly, they were hardcoded as unsigned long and int respectively. Please, use the provided typedefs in new code (and update old code as soon as possible).
     * Iostreams:
          + Fixed many outstanding issues. Thanks to Richard Smith for his work on this. (#3612, #3311, #2094, #3010, #2894, #3011, #3352, #3505).
          + For more information see the library release notes.
     * Program.Options:
          + Information about option name added to a few exception classes and various clean ups in exception classes (#3423).
          + Description wordwrapping in presense of default parameters fixed (#2613).
          + Empty value in configuration file is now permitted (#1537).
          + Quotes are no longer stripped from string values (#850).
          + Fix endless loop in case of long default arguments (#689).
          + Fix compile warning caused by usage of boost::any (#2562).
          + Fix memory bug in example/response_file.cpp (#3525).
          + Most compilation warnings were fixed (#3608).
          + Make column width for description text configurable. (#3703).
          + Add general split function: split_unix() (#2561).
          + Enable open config files from given file name (#3264).
          + Additional flag for required options (#2982).
          + Enable case insensitive style for command line (#3498).
     * PropertyMap:
          + Removed old header files (directly in the boost/ directory); they were deprecated since 1.40, replaced by headers in boost/property_map/.
     * Proto:
          + Fix const correctness issues with proto::flatten and friends (#3364).
          + Accomodate recent change to fusion::fold, remove old support for Doxygen and pre-1.35 Fusion (#3553).
          + In binary operations, when one operand has a user-specified domain and the other has the default domain, the user-specified domain trumps.
          + Fix BOOST_PROTO_EXTENDS to work with elaborated types.
          + Work around EDG compiler bug with function types and cv-qualification.
     * Regex:
          + Added support for Functors rather than strings as format expressions.
          + Improved error reporting when throwing exceptions to include better more relevant information.
          + Improved performance and reduced stack usage of recursive expressions.
          + Fixed tickets #2802, #3425, #3507, #3546, #3631, #3632, #3715, #3718, #3763, #3764
     * Spirit: Spirit V2.2, see the 'What's New' section for details.
     * Unordered:
          + Support instantiating the containers with incomplete value types.
          + Add erase_return_void as a temporary workaround for the current erase which can be inefficient because it has to find the next element to return an iterator (#3693).
          + Add templated find overload for compatible keys.
          + Improved codegear compatibility.
          + Other minor changes, full details in the changelog.
     * Xpressive:
          + match_results no longer relies on undefined behavior in std::list (#3278).
          + Do NOT copy singular iterators (#3538).
          + Eliminate gcc and darwin warnings (#3734).

Compilers Tested

   Boost's primary test compilers are:
     * OS X:
          + GCC 4.0.1 on Intel Leopard.
          + GCC 4.0.1 on PowerPC Tiger.
     * Linux:
          + GCC 4.4.1 on Ubuntu Linux.
          + GCC 4.4 on Debian.
     * Windows:
          + Visual C++ 7.1 SP1, 8.0 SP1 and 9.0 SP1 on Windows XP.
          + Visual C++ 9.0 on Windows 2008, 64 bit.
          + GCC 4.3.3, using Mingw
     * FreeBSD:
          + GCC 4.2.1, 32 and 64 bit.

   Boost's additional test compilers include:
     * Linux:
          + Intel 10.1 on Red Hat Enterprise Linux.
          + Intel 10.1 on 64 bit Red Hat Enterprise Linux.
          + Intel 11.0 on 32 bit Red Hat Enterprise Linux.
          + Intel 11.0 on 64 bit Red Hat Enterprise Linux.
          + Intel 11.1 on 64 bit Red Hat Enterprise Linux.
          + Intel 11.1 on 64 bit Linux Redhat 5.1 Server.
          + Intel 11.1 on Suse Linux 64 bit.
          + GCC 3.4.6, GCC 4.2.4, GCC 4.3.4 and GCC 4.4.2 on Red Hat Enterprise Linux.
          + GCC 4.3.4 and GCC 4.4.2 with C++0x extensions on Red Hat Enterprise Linux.
          + GCC 4.4.1 on 64 bit Linux.
          + GCC 4.4.3 on Debian unstable.
          + QLogic PathScale(TM) Compiler Suite: Version 3.2 on Red Hat Enterprise Linux.
     * OS X:
          + Intel C++ Compiler 10.1, 11.0, 11.1 on Leopard.
          + GCC 4.0.1 on Intel Leopard.
          + GCC 4.0.1 on PowerPC Tiger.
     * Windows:
          + Visual C++ 7.1, 8,0, 9,0 on XP.
          + Visual C++ 9.0 using STLport 5.2 on XP and Windows Mobile 5.0.
          + Visual C++ 10.0 beta 2.
          + Visual C++ 10.0 on 32-bit Vista.
          + Borland/Codegear C++ 5.9.3, 6.1.3 (2009), 6.2.1 (2010).
          + Intel C++ 11.1, with a Visual C++ 9.0 backend, on Vista 32-bit.
          + GCC 4.4.1 on Mingw, with and without C++0x extensions.
     * AIX:
          + IBM XL C/C++ Enterprise Edition for AIX, V10.1.0.0, on AIX Version 5.3.0.40.
     * FreeBSD:
          + GCC 4.2.1 on FreeBSD 7.0, 32 bit and 64 bit.
     * Solaris:
          + Sun C++ 5.10 on Solaris 5.10.

Acknowledgements

   Beman Dawes, Eric Niebler, Rene Rivera, and Daniel James managed this release.
