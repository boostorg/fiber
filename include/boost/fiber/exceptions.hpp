
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_fiber_EXCEPTIONS_H
#define BOOST_fiber_EXCEPTIONS_H

#include <stdexcept>
#include <string>

#include <boost/config.hpp>

#include <boost/fiber/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

class fiber_canceled : public std::runtime_error
{
public:
	fiber_canceled() :
		std::runtime_error("fiber was canceled")
	{}
};

class fiber_error : public std::runtime_error
{
public:
	fiber_error( std::string const& msg) :
		std::runtime_error( msg)
	{}
};

class fiber_interrupted
{};

class fiber_moved : public std::logic_error
{
public:
    fiber_moved() :
		std::logic_error("fibers moved")
	{}
};

class fiber_terminated : public std::runtime_error
{
public:
    fiber_terminated() :
		std::runtime_error("fiber terminated")
	{}
};

class invalid_watermark : public std::runtime_error
{
public:
    invalid_watermark() :
		std::runtime_error("invalid watermark")
	{}
};

class lock_error : public std::logic_error
{
public:
    lock_error() :
		std::logic_error("lock invalid")
	{}
};

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_fiber_EXCEPTIONS_H
