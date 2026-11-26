// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/fiber/all.hpp>

int main() {
    boost::fibers::fiber f([]{
        boost::fibers::fiber::id id = boost::this_fiber::get_id();
        (void)id;
    });
    f.join();
    return 0;
}
