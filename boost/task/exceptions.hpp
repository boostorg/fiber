
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TASKS_EXCEPTIONS_H
#define BOOST_TASKS_EXCEPTIONS_H

#include <stdexcept>
#include <string>

#include <boost/config/abi_prefix.hpp>

namespace boost {
namespace tasks {

class invalid_poolsize : public std::invalid_argument
{
public:
	invalid_poolsize() :
		std::invalid_argument("core poolsize must be greater than zero")
	{}
};

class invalid_stacksize : public std::invalid_argument
{
public:
    invalid_stacksize() :
		std::invalid_argument("stacksize must be greater than zero")
	{}
};

class invalid_watermark : public std::invalid_argument
{
public:
    invalid_watermark() :
		std::invalid_argument("invalid watermark")
	{}
};

class task_uninitialized : public std::logic_error
{
public:
    task_uninitialized() :
		std::logic_error("task uninitialized")
	{}
};

class task_already_executed : public std::logic_error
{
public:
    task_already_executed() :
		std::logic_error("task already executed")
	{}
};

class task_moved : public std::logic_error
{
public:
    task_moved() :
		std::logic_error("task moved")
	{}
};

class broken_task : public std::logic_error
{
public:
    broken_task() :
		std::logic_error("broken task")
	{}
};

struct task_interrupted
{};

class task_rejected : public std::runtime_error
{
public:
    task_rejected( std::string const& msg) :
		std::runtime_error( msg)
	{}
};

class pool_moved : public std::logic_error
{
public:
    pool_moved() :
		std::logic_error("pool moved")
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

#include <boost/config/abi_suffix.hpp>

#endif // BOOST_TASKS_EXCEPTIONS_H
