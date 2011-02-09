
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// this file is based on config.hpp of boost.thread

#ifndef BOOST_TASKS_DETAIL_CONFIG_H
#define BOOST_TASKS_DETAIL_CONFIG_H

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>

# if BOOST_WORKAROUND(__BORLANDC__, < 0x600)
#   pragma warn -8008 // Condition always true/false
#   pragma warn -8080 // Identifier declared but never used
#   pragma warn -8057 // Parameter never used
#   pragma warn -8066 // Unreachable code
# endif

# if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_TASKS_DYN_LINK)
#   undef  BOOST_TASKS_USE_LIB
#   define BOOST_TASKS_USE_DLL
# endif

# if defined(BOOST_TASKS_BUILD_DLL)   //Build dll
# elif defined(BOOST_TASKS_BUILD_LIB) //Build lib
# elif defined(BOOST_TASKS_USE_DLL)   //Use dll
# elif defined(BOOST_TASKS_USE_LIB)   //Use lib
# else //Use default
#   if defined(BOOST_TASKS_PLATFORM_WIN32)
#      if defined(BOOST_MSVC) || defined(BOOST_INTEL_WIN)
            //For compilers supporting auto-tss cleanup
            //with Boost.Threads lib, use Boost.Threads lib
#         define BOOST_TASKS_USE_LIB
#      else
            //For compilers not yet supporting auto-tss cleanup
            //with Boost.Threads lib, use Boost.Threads dll
#         define BOOST_TASKS_USE_DLL
#      endif
#   else
#      define BOOST_TASKS_USE_LIB
#   endif
# endif

# if defined(BOOST_HAS_DECLSPEC)
#   if defined(BOOST_TASKS_BUILD_DLL) //Build dll
#      define BOOST_TASKS_DECL __declspec(dllexport)
#   elif defined(BOOST_TASKS_USE_DLL) //Use dll
#      define BOOST_TASKS_DECL __declspec(dllimport)
#   else
#      define BOOST_TASKS_DECL
#   endif
# else
#   define BOOST_TASKS_DECL
# endif

// Automatically link to the correct build variant where possible.
# if ! defined(BOOST_ALL_NO_LIB) && ! defined(BOOST_TASKS_NO_LIB) && ! defined(BOOST_TASKS_BUILD_DLL) && ! defined(BOOST_TASKS_BUILD_LIB)

// Tell the autolink to link dynamically, this will get undef'ed by auto_link.hpp
# if defined(BOOST_TASKS_USE_DLL)
#   define BOOST_DYN_LINK
# endif

// Set the name of our library, this will get undef'ed by auto_link.hpp
# if defined(BOOST_TASKS_LIB_NAME)
#   define BOOST_LIB_NAME BOOST_TASKS_LIB_NAME
# else
#   define BOOST_LIB_NAME boost_task
# endif

// If we're importing code from a dll, then tell auto_link.hpp about it
// And include the header that does the work
#include <boost/config/auto_link.hpp>
# endif  // auto-linking disabled

#endif // BOOST_TASKS_DETAIL_CONFIG_H

