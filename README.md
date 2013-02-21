boost.fiber
===========

boost.fiber provides a framework for micro-/userland-threads (fibers) scheduled cooperativly.
The API contains classes and functions to manage and synchronize fibers similiar to boost.thread.
This library is intended to support quasi-concurrency on embedded system or to replace boost.thread
for testing puposes (for instance checking for raise conditions) and to solve the many depended
task problem.

A fiber is able to store the current execution state, including all registers and CPU flags, the 
instruction pointer, and the stack pointer and later restore this state. The idea is to have multiple 
execution paths running on a single thread using a sort of cooperative scheduling (threads are 
preemptively scheduled) - the running fiber decides explicitly when its yields to allow another fiber to
run (context switching).

Fibers can be synchronized running in different threads. In order to support task steeling it boost.fiber
support migration of fibers between threads.

A context switch between threads costs usally thousends of CPU cycles on x86 compared to a fiber switch 
with less than 100 cycles. A fiber can only run on a single thread at any point in time but may be 
migrated between threads.

Buiding: Detailed instructions can be found at https://svn.boost.org/trac/boost/wiki/TryModBoost.

git clone http://github.com/boostorg/boost modular-boost

cd modular-boost

git submodule update --init

cd libs

git clone http://github.com/olk/boost-fiber fiber

cd ..

cmake -P forward_headers.cmake

./bootstrap.sh

cp b2 /usr/local/bin

cd libs/fiber/test

b2
