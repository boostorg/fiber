
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKS_WATER_MARK_H
#define BOOST_TASKS_WATER_MARK_H

#include <cstddef>

#include <boost/task/detail/config.hpp>

#include <boost/config/abi_prefix.hpp>

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4251 4275)
# endif

namespace boost {
namespace tasks {

class BOOST_TASKS_DECL high_watermark
{
private:
	std::size_t	value_;

public:
	explicit high_watermark( std::size_t value);

	operator std::size_t () const;
};

class BOOST_TASKS_DECL low_watermark
{
private:
	std::size_t	value_;

public:
	explicit low_watermark( std::size_t value);

	operator std::size_t () const;
};

}}

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#include <boost/config/abi_suffix.hpp>

#endif // BOOST_TASKS_WATER_MARK_H
