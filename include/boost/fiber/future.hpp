//  (C) Copyright 2008-10 Anthony Williams 
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_FUTURE_HPP
#define BOOST_FIBERS_FUTURE_HPP

#include <algorithm>
#include <list>
#include <stdexcept>
#include <vector>

#include <boost/bind.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/config.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/function.hpp>
#include <boost/move/move.hpp>
#include <boost/mpl/if.hpp>
#include <boost/next_prior.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/ref.hpp>
#include <boost/scoped_array.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/throw_exception.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/is_fundamental.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/utility/result_of.hpp>

#include <boost/fiber/condition.hpp>
#include <boost/fiber/mutex.hpp>
#include <boost/fiber/operations.hpp>

namespace boost {
namespace fibers {

    class future_uninitialized:
        public std::logic_error
    {
    public:
        future_uninitialized():
            std::logic_error("Future Uninitialized")
        {}
    };
    class broken_promise:
        public std::logic_error
    {
    public:
        broken_promise():
            std::logic_error("Broken promise")
        {}
    };
    class future_already_retrieved:
        public std::logic_error
    {
    public:
        future_already_retrieved():
            std::logic_error("Future already retrieved")
        {}
    };
    class promise_already_satisfied:
        public std::logic_error
    {
    public:
        promise_already_satisfied():
            std::logic_error("Promise already satisfied")
        {}
    };

    class task_already_started:
        public std::logic_error
    {
    public:
        task_already_started():
            std::logic_error("Task already started")
        {}
    };

    class task_moved:
        public std::logic_error
    {
    public:
        task_moved():
            std::logic_error("Task moved")
        {}
    };

    namespace future_state
    {
        enum state { uninitialized, waiting, ready, moved };
    }

    namespace detail
    {
        struct future_continuation_base
        {
            future_continuation_base() {}

            virtual ~future_continuation_base() {}

            virtual void do_continuation(boost::unique_lock<boost::fibers::mutex>& ) {};

        private:
            future_continuation_base(future_continuation_base const&);
            future_continuation_base& operator=(future_continuation_base const&);
        };

        template <typename F, typename R, typename C>
        struct future_continuation;

        struct future_object_base
        {
            boost::exception_ptr exception;
            bool done;
            boost::fibers::mutex mutex;
            boost::fibers::condition waiters;
            typedef std::list<boost::fibers::condition*> waiter_list;
            waiter_list external_waiters;
            boost::function<void()> callback;
            shared_ptr<future_continuation_base>    continuation_ptr;

            future_object_base():
                done(false)
            {}
            virtual ~future_object_base()
            {}

            waiter_list::iterator register_external_waiter(boost::fibers::condition& cv)
            {
                boost::unique_lock<boost::fibers::mutex> lock(mutex);
                do_callback(lock);
                return external_waiters.insert(external_waiters.end(),&cv);
            }
            
            void remove_external_waiter(waiter_list::iterator it)
            {
                boost::unique_lock<boost::fibers::mutex> lock(mutex);
                external_waiters.erase(it);
            }

            void do_continuation(boost::unique_lock<boost::fibers::mutex>& lock)
            {
                if (continuation_ptr) {
                    continuation_ptr->do_continuation(lock);
                }
            }

            void set_continuation_ptr(future_continuation_base* continuation,
                    boost::unique_lock<boost::fibers::mutex>& lock)
            {
                continuation_ptr.reset(continuation);
                if (done) {
                    do_continuation(lock);
                }
            }

            void mark_finished_internal(boost::unique_lock<boost::fibers::mutex>& lock)
            {
                done=true;
                waiters.notify_all();
                for(waiter_list::const_iterator it=external_waiters.begin(),
                        end=external_waiters.end();it!=end;++it)
                {
                    (*it)->notify_all();
                }
                do_continuation(lock);
            }

            void make_ready()
            {
                boost::unique_lock<boost::fibers::mutex> lock(mutex);
                mark_finished_internal(lock);
            }

            struct relocker
            {
                boost::unique_lock<boost::fibers::mutex>& lock;
                
                relocker(boost::unique_lock<boost::fibers::mutex>& lock_):
                    lock(lock_)
                {
                    lock.unlock();
                }
                ~relocker()
                {
                    lock.lock();
                }
            private:
                relocker& operator=(relocker const&);
            };

            void do_callback(boost::unique_lock<boost::fibers::mutex>& lock)
            {
                if(callback && !done)
                {
                    boost::function<void()> local_callback=callback;
                    relocker relock(lock);
                    local_callback();
                }
            }
            

            void wait(bool rethrow=true)
            {
                boost::unique_lock<boost::fibers::mutex> lock(mutex);
                do_callback(lock);
                while(!done)
                {
                    if ( this_fiber::is_fiberized() )
                        waiters.wait(lock);
                    else
                    {
                        lock.unlock();
                        boost::fibers::run();
                        lock.lock();
                    }
                }
                if(rethrow && exception)
                {
                    boost::rethrow_exception(exception);
                }
            }

            bool timed_wait_until(chrono::system_clock::time_point const& target_time)
            {
                boost::unique_lock<boost::fibers::mutex> lock(mutex);
                do_callback(lock);
                while(!done)
                {
                    bool const success=waiters.timed_wait(lock,target_time);
                    if(!success && !done)
                    {
                        return false;
                    }
                }
                return true;
            }

            void mark_exceptional_finish_internal(boost::exception_ptr const& e,
                                          boost::unique_lock<boost::fibers::mutex>& lock)
            {
                exception=e;
                mark_finished_internal(lock);
            }
            void mark_exceptional_finish()
            {
                boost::unique_lock<boost::fibers::mutex> lock(mutex);
                mark_exceptional_finish_internal(boost::current_exception(),lock);
            }

            bool has_value()
            {
                boost::unique_lock<boost::fibers::mutex> lock(mutex);
                return done && !exception;
            }
            bool has_exception()
            {
                boost::unique_lock<boost::fibers::mutex> lock(mutex);
                return done && exception;
            }

            template<typename F,typename U>
            void set_wait_callback(F f,U* u)
            {
                callback=boost::bind(f,boost::ref(*u));
            }
            
        private:
            future_object_base(future_object_base const&);
            future_object_base& operator=(future_object_base const&);
        };

        template<typename T>
        struct future_traits
        {
            typedef boost::scoped_ptr<T> storage_type;
#ifndef BOOST_NO_RVALUE_REFERENCES
            typedef T const& source_reference_type;
            struct dummy;
            typedef typename boost::mpl::if_<boost::is_fundamental<T>,dummy&,T&&>::type rvalue_source_type;
            typedef typename boost::mpl::if_<boost::is_fundamental<T>,T,T&&>::type move_dest_type;
#else
            typedef T& source_reference_type;
            typedef typename boost::mpl::if_<boost::is_convertible<T&,boost::detail::thread_move_t<T> >,boost::detail::thread_move_t<T>,T const&>::type rvalue_source_type;
            typedef typename boost::mpl::if_<boost::is_convertible<T&,BOOST_RV_REF( T) >,BOOST_RV_REF(T),T>::type move_dest_type;
#endif

            static void init(storage_type& storage,source_reference_type t)
            {
                storage.reset(new T(t));
            }
            
            static void init(storage_type& storage,rvalue_source_type t)
            {
                storage.reset(new T(static_cast<rvalue_source_type>(t)));
            }

            static void cleanup(storage_type& storage)
            {
                storage.reset();
            }
        };
        
        template<typename T>
        struct future_traits<T&>
        {
            typedef T* storage_type;
            typedef T& source_reference_type;
            struct rvalue_source_type
            {};
            typedef T& move_dest_type;

            static void init(storage_type& storage,T& t)
            {
                storage=&t;
            }

            static void cleanup(storage_type& storage)
            {
                storage=0;
            }
        };

        template<>
        struct future_traits<void>
        {
            typedef bool storage_type;
            typedef void move_dest_type;

            static void init(storage_type& storage)
            {
                storage=true;
            }

            static void cleanup(storage_type& storage)
            {
                storage=false;
            }

        };

        template<typename T>
        struct future_object:
            detail::future_object_base
        {
            typedef typename future_traits<T>::storage_type storage_type;
            typedef typename future_traits<T>::source_reference_type source_reference_type;
            typedef typename future_traits<T>::rvalue_source_type rvalue_source_type;
            typedef typename future_traits<T>::move_dest_type move_dest_type;
            
            storage_type result;

            future_object():
                result(0)
            {}

            void mark_finished_with_result_internal(source_reference_type result_, boost::unique_lock<boost::fibers::mutex>& lock)
            {
                future_traits<T>::init(result,result_);
                mark_finished_internal(lock);
            }
            void mark_finished_with_result_internal(rvalue_source_type result_, boost::unique_lock<boost::fibers::mutex>& lock)
            {
                future_traits<T>::init(result,static_cast<rvalue_source_type>(result_));
                mark_finished_internal( lock);
            }

            void mark_finished_with_result(source_reference_type result_)
            {
                boost::unique_lock<boost::fibers::mutex> lock(mutex);
                mark_finished_with_result_internal(result_, lock);
            }
            void mark_finished_with_result(rvalue_source_type result_)
            {
                boost::unique_lock<boost::fibers::mutex> lock(mutex);
                mark_finished_with_result_internal(result_, lock);
            }

            move_dest_type get()
            {
                wait();
                return static_cast<move_dest_type>(*result);
            }

            future_state::state get_state()
            {
                boost::unique_lock<boost::fibers::mutex> guard(mutex);
                if(!done)
                {
                    return future_state::waiting;
                }
                else
                {
                    return future_state::ready;
                }
            }

        private:
            future_object(future_object const&);
            future_object& operator=(future_object const&);
        };

        template<>
        struct future_object<void>:
            detail::future_object_base
        {
            future_object()
            {}

            void mark_finished_with_result_internal(boost::unique_lock<boost::fibers::mutex>& lock)
            {
                mark_finished_internal(lock);
            }

            void mark_finished_with_result()
            {
                boost::unique_lock<boost::fibers::mutex> lock(mutex);
                mark_finished_with_result_internal(lock);
            }

            void get()
            {
                wait();
            }
            
            future_state::state get_state()
            {
                boost::unique_lock<boost::fibers::mutex> guard(mutex);
                if(!done)
                {
                    return future_state::waiting;
                }
                else
                {
                    return future_state::ready;
                }
            }

        private:
            future_object(future_object const&);
            future_object& operator=(future_object const&);
        };

        class future_waiter
        {
            struct registered_waiter;
            typedef std::vector<registered_waiter>::size_type count_type;
            
            struct registered_waiter
            {
                boost::shared_ptr<detail::future_object_base> future;
                detail::future_object_base::waiter_list::iterator wait_iterator;
                count_type index;

                registered_waiter(boost::shared_ptr<detail::future_object_base> const& future_,
                                  detail::future_object_base::waiter_list::iterator wait_iterator_,
                                  count_type index_):
                    future(future_),wait_iterator(wait_iterator_),index(index_)
                {}

            };
            
            struct all_futures_lock
            {
                count_type count;
                boost::scoped_array<boost::unique_lock<boost::fibers::mutex> > locks;
                
                all_futures_lock(std::vector<registered_waiter>& futures):
                    count(futures.size()),locks(new boost::unique_lock<boost::fibers::mutex>[count])
                {
                    for(count_type i=0;i<count;++i)
                    {
                        locks[i]=boost::unique_lock<boost::fibers::mutex>(futures[i].future->mutex);
                    }
                }
                
                void lock()
                {
                    boost::lock(locks.get(),locks.get()+count);
                }
                
                void unlock()
                {
                    for(count_type i=0;i<count;++i)
                    {
                        locks[i].unlock();
                    }
                }
            };
            
            boost::fibers::condition cv;
            std::vector<registered_waiter> futures;
            count_type future_count;
            
        public:
            future_waiter():
                future_count(0)
            {}
            
            template<typename F>
            void add(F& f)
            {
                if(f.future)
                {
                    futures.push_back(registered_waiter(f.future,f.future->register_external_waiter(cv),future_count));
                }
                ++future_count;
            }

            count_type wait()
            {
                all_futures_lock lk(futures);
                for(;;)
                {
                    for(count_type i=0;i<futures.size();++i)
                    {
                        if(futures[i].future->done)
                        {
                            return futures[i].index;
                        }
                    }
                    cv.wait(lk);
                }
            }
            
            ~future_waiter()
            {
                for(count_type i=0;i<futures.size();++i)
                {
                    futures[i].future->remove_external_waiter(futures[i].wait_iterator);
                }
            }
            
        };
        
    }

    template <typename R>
    class unique_future;

    template <typename R>
    class shared_future;

    template <typename R>
    class promise;

    template <typename R>
    class packaged_task;

    namespace detail {

        template <typename F, typename R, typename C>
            struct future_continuation : future_continuation_base
        {
            F& parent;
            C continuation;
            promise<R> next;

            future_continuation(F& f, BOOST_THREAD_FWD_REF(C) c) :
                parent(f),
                continuation(boost::forward<C>(c)),
                next()
            {}

            ~future_continuation()
            {}

            void do_continuation(boost::unique_lock<boost::fibers::mutex>& lk)
            {
                try
                {
                    lk.unlock();
                    {
                        R val = continuation(parent);
                        next.set_value(boost::move(val));
                    }
                }
                catch (...)
                {
                    next.set_exception(boost::current_exception());
                }
            }
        private:

            future_continuation(future_continuation const&);
            future_continuation& operator=(future_continuation const&);
        };

    }

    template <typename R>
    class unique_future
    {
        typedef boost::shared_ptr<detail::future_object<R> > future_ptr;
        
        future_ptr future;

        friend class shared_future<R>;
        friend class promise<R>;
        friend class packaged_task<R>;
        friend class detail::future_waiter;
        template <typename A, typename B, typename C>
        friend struct detail::future_continuation;

        typedef typename detail::future_traits<R>::move_dest_type move_dest_type;

        unique_future(future_ptr future_):
            future(future_)
        {}

        BOOST_MOVABLE_BUT_NOT_COPYABLE( unique_future);

    public:
        typedef future_state::state state;

        unique_future()
        {}
       
        ~unique_future()
        {}

#ifndef BOOST_NO_RVALUE_REFERENCES
        unique_future(unique_future && other)
        {
            future.swap(other.future);
        }
        unique_future& operator=(unique_future && other)
        {
            future=other.future;
            other.future.reset();
            return *this;
        }
#else
        unique_future( BOOST_RV_REF( unique_future) other):
            future(other.future)
        {
            other.future.reset();
        }

        unique_future& operator=(BOOST_RV_REF( unique_future) other)
        {
            future=other.future;
            other.future.reset();
            return *this;
        }
#endif

        void swap(unique_future& other)
        {
            future.swap(other.future);
        }

        // retrieving the value
        move_dest_type get()
        {
            if(!future)
            {
                boost::throw_exception(future_uninitialized());
            }

            return future->get();
        }
        
        // functions to check state, and wait for ready
        state get_state() const
        {
            if(!future)
            {
                return future_state::uninitialized;
            }
            return future->get_state();
        }
        

        bool is_ready() const
        {
            return get_state()==future_state::ready;
        }
        
        bool has_exception() const
        {
            return future && future->has_exception();
        }
        
        bool has_value() const
        {
            return future && future->has_value();
        }
        
        void wait() const
        {
            if(!future)
            {
                boost::throw_exception(future_uninitialized());
            }
            future->wait(false);
        }

        template<typename Duration>
        bool timed_wait(Duration const& rel_time) const
        {
            return timed_wait_until(chrono::system_clock::now()+rel_time);
        }
        
        bool timed_wait_until(chrono::system_clock::time_point const& abs_time) const
        {
            if(!future)
            {
                boost::throw_exception(future_uninitialized());
            }
            return future->timed_wait_until(abs_time);
        }

        template<typename RF>
        unique_future<RF>
        then(RF(*func)(unique_future< R >&))
        {
            typedef RF future_type;

            if (future)
            {
                boost::unique_lock<boost::fibers::mutex> lock(future->mutex);
                detail::future_continuation<unique_future<R>, future_type, RF(*)(unique_future&) > *ptr =
                    new detail::future_continuation<unique_future<R>, future_type, RF(*)(unique_future&)>(*this, func);
                if (ptr==0) return unique_future<future_type>();
                future->set_continuation_ptr(ptr, lock);
                return ptr->next.get_future();
            }
            else
            {
                // fixme what to do when the future has no associated state?
                return unique_future<future_type>();
            }
        }

        template<typename F>
        unique_future<typename boost::result_of<F(unique_future<R>&)>::type>
        then(BOOST_RV_REF(F) func)
        {
            typedef typename boost::result_of<F(unique_future<R>&)>::type future_type;

            if (future)
            {
                boost::unique_lock<boost::fibers::mutex> lock(future->mtx);
                detail::future_continuation<unique_future<R>, future_type, F > *ptr =
                    new detail::future_continuation<unique_future<R>, future_type, F>(*this, boost::forward<F>(func));
                if (ptr==0) return unique_future<future_type>();
                future->set_continuation_ptr(ptr, lock);
                return ptr->next.get_future();
            }
            else
            {
                // fixme what to do when the future has no associated state?
                return unique_future<future_type>();
            }
        }
    };

    template <typename R>
    class shared_future
    {
        typedef boost::shared_ptr<detail::future_object<R> > future_ptr;
        
        future_ptr future;

//         shared_future(const unique_future<R>& other);
//         shared_future& operator=(const unique_future<R>& other);

        friend class detail::future_waiter;
        friend class promise<R>;
        friend class packaged_task<R>;
        
        shared_future(future_ptr future_):
            future(future_)
        {}

        BOOST_COPYABLE_AND_MOVABLE( shared_future);

    public:
        shared_future(shared_future const& other):
            future(other.future)
        {}

        typedef future_state::state state;

        shared_future()
        {}

        ~shared_future()
        {}

        shared_future& operator=(BOOST_COPY_ASSIGN_REF(shared_future) other)
        {
            future=other.future;
            return *this;
        }
        shared_future& operator=(BOOST_RV_REF(unique_future<R>) other)
        {
            future=other.future;
            other.future.reset();
            return *this;
        }
#ifndef BOOST_NO_RVALUE_REFERENCES
        shared_future(shared_future && other)
        {
            future.swap(other.future);
        }
        shared_future(unique_future<R> && other)
        {
            future.swap(other.future);
        }
        shared_future& operator=(shared_future && other)
        {
            future.swap(other.future);
            other.future.reset();
            return *this;
        }
#if 0
        shared_future& operator=(unique_future<R> && other)
        {
            future.swap(other.future);
            other.future.reset();
            return *this;
        }
#endif
#else            
        shared_future(BOOST_RV_REF(shared_future) other):
            future(other.future)
        {
            other.future.reset();
        }
//         shared_future(const unique_future<R> &) = delete;
        shared_future(BOOST_RV_REF(unique_future<R>) other):
            future(other.future)
        {
            other.future.reset();
        }
        shared_future& operator=(BOOST_RV_REF(shared_future) other)
        {
            future.swap(other->future);
            other->future.reset();
            return *this;
        }
#endif

        void swap(shared_future& other)
        {
            future.swap(other.future);
        }

        // retrieving the value
        R get()
        {
            if(!future)
            {
                boost::throw_exception(future_uninitialized());
            }

            return future->get();
        }
        
        // functions to check state, and wait for ready
        state get_state() const
        {
            if(!future)
            {
                return future_state::uninitialized;
            }
            return future->get_state();
        }
        

        bool is_ready() const
        {
            return get_state()==future_state::ready;
        }
        
        bool has_exception() const
        {
            return future && future->has_exception();
        }
        
        bool has_value() const
        {
            return future && future->has_value();
        }

        void wait() const
        {
            if(!future)
            {
                boost::throw_exception(future_uninitialized());
            }
            future->wait(false);
        }

        template<typename Duration>
        bool timed_wait(Duration const& rel_time) const
        {
            return timed_wait_until(chrono::system_clock::now()+rel_time);
        }
        
        bool timed_wait_until(chrono::system_clock::time_point const& abs_time) const
        {
            if(!future)
            {
                boost::throw_exception(future_uninitialized());
            }
            return future->timed_wait_until(abs_time);
        }
    };

    template <typename R>
    class promise
    {
        typedef boost::shared_ptr<detail::future_object<R> > future_ptr;
        
        future_ptr future;
        bool future_obtained;
        
        void lazy_init()
        {
            if(!atomic_load(&future))
            {
                future_ptr blank;
                atomic_compare_exchange(&future,&blank,future_ptr(new detail::future_object<R>));
            }
        }

        BOOST_MOVABLE_BUT_NOT_COPYABLE( promise);
        
    public:
//         template <class Allocator> explicit promise(Allocator a);

        promise():
            future(),future_obtained(false)
        {}
        
        ~promise()
        {
            if(future)
            {
                boost::unique_lock<boost::fibers::mutex> lock(future->mutex);

                if(!future->done)
                {
                    future->mark_exceptional_finish_internal(boost::copy_exception(broken_promise()),lock);
                }
            }
        }

        // Assignment
#ifndef BOOST_NO_RVALUE_REFERENCES
        promise(promise && rhs):
            future_obtained(rhs.future_obtained)
        {
            future.swap(rhs.future);
            rhs.future_obtained=false;
        }
        promise & operator=(promise&& rhs)
        {
            future.swap(rhs.future);
            future_obtained=rhs.future_obtained;
            rhs.future.reset();
            rhs.future_obtained=false;
            return *this;
        }
#else
        promise(BOOST_RV_REF( promise) rhs):
            future(rhs.future),future_obtained(rhs.future_obtained)
        {
            rhs.future.reset();
            rhs.future_obtained=false;
        }
        promise & operator=(BOOST_RV_REF( promise) rhs)
        {
            future=rhs.future;
            future_obtained=rhs.future_obtained;
            rhs.future.reset();
            rhs.future_obtained=false;
            return *this;
        }
#endif   
        
        void swap(promise& other)
        {
            future.swap(other.future);
            std::swap(future_obtained,other.future_obtained);
        }

        // Result retrieval
        unique_future<R> get_future()
        {
            lazy_init();
            if(future_obtained)
            {
                boost::throw_exception(future_already_retrieved());
            }
            future_obtained=true;
            return unique_future<R>(future);
        }

        void set_value(typename detail::future_traits<R>::source_reference_type r)
        {
            lazy_init();
            boost::unique_lock<boost::fibers::mutex> lock(future->mutex);
            if(future->done)
            {
                boost::throw_exception(promise_already_satisfied());
            }
            future->mark_finished_with_result_internal(r, lock);
        }

//         void set_value(R && r);
        void set_value(typename detail::future_traits<R>::rvalue_source_type r)
        {
            lazy_init();
            boost::unique_lock<boost::fibers::mutex> lock(future->mutex);
            if(future->done)
            {
                boost::throw_exception(promise_already_satisfied());
            }
            future->mark_finished_with_result_internal(static_cast<typename detail::future_traits<R>::rvalue_source_type>(r), lock);
        }

        void set_exception(boost::exception_ptr p)
        {
            lazy_init();
            boost::unique_lock<boost::fibers::mutex> lock(future->mutex);
            if(future->done)
            {
                boost::throw_exception(promise_already_satisfied());
            }
            future->mark_exceptional_finish_internal(p, lock);
        }

        template<typename F>
        void set_wait_callback(F f)
        {
            lazy_init();
            future->set_wait_callback(f,this);
        }
        
    };

    template <>
    class promise<void>
    {
        typedef boost::shared_ptr<detail::future_object<void> > future_ptr;
        
        future_ptr future;
        bool future_obtained;

        void lazy_init()
        {
            if(!atomic_load(&future))
            {
                future_ptr blank;
                atomic_compare_exchange(&future,&blank,future_ptr(new detail::future_object<void>));
            }
        }

    BOOST_MOVABLE_BUT_NOT_COPYABLE( promise);
    public:
//         template <class Allocator> explicit promise(Allocator a);

        promise():
            future(),future_obtained(false)
        {}
        
        ~promise()
        {
            if(future)
            {
                boost::unique_lock<boost::fibers::mutex> lock(future->mutex);

                if(!future->done)
                {
                    future->mark_exceptional_finish_internal(boost::copy_exception(broken_promise()),lock);
                }
            }
        }

        // Assignment
#ifndef BOOST_NO_RVALUE_REFERENCES
        promise(promise && rhs):
            future_obtained(rhs.future_obtained)
        {
            future.swap(rhs.future);
            rhs.future_obtained=false;
        }
        promise & operator=(promise&& rhs)
        {
            future.swap(rhs.future);
            future_obtained=rhs.future_obtained;
            rhs.future.reset();
            rhs.future_obtained=false;
            return *this;
        }
#else
        promise(BOOST_RV_REF(promise) rhs):
            future(rhs.future),future_obtained(rhs.future_obtained)
        {
            rhs.future.reset();
            rhs.future_obtained=false;
        }
        promise & operator=(BOOST_RV_REF(promise) rhs)
        {
            future=rhs.future;
            future_obtained=rhs.future_obtained;
            rhs.future.reset();
            rhs.future_obtained=false;
            return *this;
        }
#endif
        
        void swap(promise& other)
        {
            future.swap(other.future);
            std::swap(future_obtained,other.future_obtained);
        }

        // Result retrieval
        unique_future<void> get_future()
        {
            lazy_init();
            
            if(future_obtained)
            {
                boost::throw_exception(future_already_retrieved());
            }
            future_obtained=true;
            return unique_future<void>(future);
        }

        void set_value()
        {
            lazy_init();
            boost::unique_lock<boost::fibers::mutex> lock(future->mutex);
            if(future->done)
            {
                boost::throw_exception(promise_already_satisfied());
            }
            future->mark_finished_with_result_internal( lock);
        }

        void set_exception(boost::exception_ptr p)
        {
            lazy_init();
            boost::unique_lock<boost::fibers::mutex> lock(future->mutex);
            if(future->done)
            {
                boost::throw_exception(promise_already_satisfied());
            }
            future->mark_exceptional_finish_internal(p,lock);
        }

        template<typename F>
        void set_wait_callback(F f)
        {
            lazy_init();
            future->set_wait_callback(f,this);
        }
        
    };

    namespace detail
    {
        template<typename R>
        struct task_base:
            detail::future_object<R>
        {
            bool started;

            task_base():
                started(false)
            {}

            void run()
            {
                {
                    boost::unique_lock<boost::fibers::mutex> lk(this->mutex);
                    if(started)
                    {
                        boost::throw_exception(task_already_started());
                    }
                    started=true;
                }
                do_run();
            }

            void owner_destroyed()
            {
                boost::unique_lock<boost::fibers::mutex> lk(this->mutex);
                if(!started)
                {
                    started=true;
                    this->mark_exceptional_finish_internal(boost::copy_exception(boost::fibers::broken_promise()),lk);
                }
            }
            
            
            virtual void do_run()=0;
        };
        
        
        template<typename R,typename F>
        struct task_object:
            task_base<R>
        {
            F f;
            task_object(F const& f_):
                f(f_)
            {}
            task_object(boost::detail::thread_move_t<F> f_):
                f(f_)
            {}
            
            void do_run()
            {
                try
                {
                    this->mark_finished_with_result(f());
                }
                catch(...)
                {
                    this->mark_exceptional_finish();
                }
            }
        };

        template<typename F>
        struct task_object<void,F>:
            task_base<void>
        {
            F f;
            task_object(F const& f_):
                f(f_)
            {}
            task_object(boost::detail::thread_move_t<F> f_):
                f(f_)
            {}
            
            void do_run()
            {
                try
                {
                    f();
                    this->mark_finished_with_result();
                }
                catch(...)
                {
                    this->mark_exceptional_finish();
                }
            }
        };

    }
    

    template<typename R>
    class packaged_task
    {
        boost::shared_ptr<detail::task_base<R> > task;
        bool future_obtained;

        BOOST_MOVABLE_BUT_NOT_COPYABLE( packaged_task);
    public:
        packaged_task():
            future_obtained(false)
        {}
        
        // construction and destruction
        template <class F>
        explicit packaged_task(F const& f):
            task(new detail::task_object<R,F>(f)),future_obtained(false)
        {}
        explicit packaged_task(R(*f)()):
            task(new detail::task_object<R,R(*)()>(f)),future_obtained(false)
        {}

//         template <class F, class Allocator>
//         explicit packaged_task(F const& f, Allocator a);
//         template <class F, class Allocator>
//         explicit packaged_task(F&& f, Allocator a);


        ~packaged_task()
        {
            if(task)
            {
                task->owner_destroyed();
            }
        }

        // assignment
#ifndef BOOST_NO_RVALUE_REFERENCES
        packaged_task(packaged_task&& other):
            future_obtained(other.future_obtained)
        {
            task.swap(other.task);
            other.future_obtained=false;
        }
        packaged_task& operator=(packaged_task&& other)
        {
            packaged_task temp(static_cast<packaged_task&&>(other));
            swap(temp);
            return *this;
        }
#else
        packaged_task(BOOST_RV_REF( packaged_task) other):
            future_obtained(other.future_obtained)
        {
            task.swap(other.task);
            other.future_obtained=false;
        }
        packaged_task& operator=(BOOST_RV_REF( packaged_task) other)
        {
            packaged_task temp(other);
            swap(temp);
            return *this;
        }
#endif

        void swap(packaged_task& other)
        {
            task.swap(other.task);
            std::swap(future_obtained,other.future_obtained);
        }

        // result retrieval
        unique_future<R> get_future()
        {
            if(!task)
            {
                boost::throw_exception(task_moved());
            }
            else if(!future_obtained)
            {
                future_obtained=true;
                return unique_future<R>(task);
            }
            else
            {
                boost::throw_exception(future_already_retrieved());
            }
        }
        

        // execution
        void operator()()
        {
            if(!task)
            {
                boost::throw_exception(task_moved());
            }
            task->run();
        }

        template<typename F>
        void set_wait_callback(F f)
        {
            task->set_wait_callback(f,this);
        }
        
    };

    template <class R>
    unique_future<R> async(R(*f)())
    {
        typedef packaged_task<R> packaged_task_type;

        packaged_task_type pt( f);
        unique_future<R> ret = pt.get_future();
        fiber( boost::move(pt) ).detach();

        return boost::move( ret);
    }

    template <class F>
    unique_future<typename boost::result_of<typename decay<F>::type()>::type>
    async(BOOST_RV_REF(F) f)
    {
        typedef typename boost::result_of<typename decay<F>::type()>::type R;
        typedef packaged_task<R> packaged_task_type;

        packaged_task_type pt( boost::forward<F>(f) );
        unique_future<R> ret = pt.get_future();
        fiber( boost::move(pt) ).detach();

        return boost::move( ret);
    }

#define BOOST_FIBERS_WAITFOR_FUTURE_FN_ARG(z,n,unused) \
    BOOST_PP_CAT(F,n) & BOOST_PP_CAT(f,n)

#define BOOST_FIBERS_WAITFOR_FUTURE_FN_ARGS(n) \
	BOOST_PP_ENUM(n,BOOST_FIBERS_WAITFOR_FUTURE_FN_ARG,~)

#define BOOST_FIBERS_WAITFOR_FUTURE_WAIT(z,n,t) \
	BOOST_PP_CAT(f,n).wait();

#define BOOST_FIBERS_WAITFOR_FUTURE_WAITER(z,n,t) \
	waiter.add( BOOST_PP_CAT(f,n) );

#define BOOST_FIBERS_WAITFOR_FUTURE_ALL(z,n,unused) \
template< BOOST_PP_ENUM_PARAMS(n, typename F) > \
void waitfor_all( BOOST_FIBERS_WAITFOR_FUTURE_FN_ARGS(n) ) \
{ \
    BOOST_PP_REPEAT(n,BOOST_FIBERS_WAITFOR_FUTURE_WAIT,~); \
}

#define BOOST_FIBERS_WAITFOR_FUTURE_ANY(z,n,unused) \
template< BOOST_PP_ENUM_PARAMS(n, typename F) > \
unsigned int waitfor_any( BOOST_FIBERS_WAITFOR_FUTURE_FN_ARGS(n) ) \
{ \
    detail::future_waiter waiter; \
    BOOST_PP_REPEAT(n,BOOST_FIBERS_WAITFOR_FUTURE_WAITER,~); \
    return waiter.wait(); \
}

#ifndef BOOST_FIBERS_WAITFOR_FUTURE_MAX_ARITY
#define BOOST_FIBERS_WAITFOR_FUTURE_MAX_ARITY 12
#endif

BOOST_PP_REPEAT_FROM_TO( 2, BOOST_FIBERS_WAITFOR_FUTURE_MAX_ARITY, BOOST_FIBERS_WAITFOR_FUTURE_ALL, ~)
BOOST_PP_REPEAT_FROM_TO( 2, BOOST_FIBERS_WAITFOR_FUTURE_MAX_ARITY, BOOST_FIBERS_WAITFOR_FUTURE_ANY, ~)

#undef BOOST_FIBERS_WAITFOR_FUTURE_WAIT
#undef BOOST_FIBERS_WAITFOR_FUTURE_WAITER
#undef BOOST_FIBERS_WAITFOR_FUTURE_ALL
#undef BOOST_FIBERS_WAITFOR_FUTURE_ANY
#undef BOOST_FIBERS_WAITFOR_FUTURE_FN_ARGS
#undef BOOST_FIBERS_WAITFOR_FUTURE_FN_ARG

}}

#endif
