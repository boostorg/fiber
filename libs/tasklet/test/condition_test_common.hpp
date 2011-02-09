#ifndef TASK_CONDITION_TEST_COMMON_HPP
#define TASK_CONDITION_TEST_COMMON_HPP
// Copyright (C) 2007 Anthony Williams
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/utility.hpp>

#include <boost/tasklet.hpp>

unsigned const timeout_seconds=5;

struct wait_for_flag : private boost::noncopyable
{
    boost::tasklet::mutex mutex;
    boost::tasklet::condition cond_var;
    bool flag;
    unsigned woken;
        
    wait_for_flag():
        flag(false),woken(0)
    {}

    struct check_flag
    {
        bool const& flag;
            
        check_flag(bool const& flag_):
            flag(flag_)
        {}
            
        bool operator()() const
        {
            return flag;
        }
    private:
        void operator=(check_flag&);
    };

        
    void wait_without_predicate()
    {
        boost::tasklet::mutex::scoped_lock lock(mutex);
        while(!flag)
        {
            cond_var.wait(lock);
        }
        ++woken;
    }

    void wait_with_predicate()
    {
        boost::tasklet::mutex::scoped_lock lock(mutex);
        cond_var.wait(lock,check_flag(flag));
        if(flag)
        {
            ++woken;
        }
    }
};

#endif
