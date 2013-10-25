
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_PACKAGED_TASK_HPP
#define BOOST_FIBERS_PACKAGED_TASK_HPP

#include <memory>

#include <boost/config.hpp>
#include <boost/move/move.hpp>
#include <boost/thread/detail/memory.hpp> // boost::allocator_arg_t
#include <boost/throw_exception.hpp>
#include <boost/type_traits/decay.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/exceptions.hpp>
#include <boost/fiber/future/detail/task_base.hpp>
#include <boost/fiber/future/detail/task_object.hpp>
#include <boost/fiber/future/future.hpp>

namespace boost {
namespace fibers {

template< typename Signature >
class packaged_task;

template< typename R >
class packaged_task< R() > : private noncopyable
{
private:
    typedef typename detail::task_base< R >::ptr_t   ptr_t;

    struct dummy
    { void nonnull() {} };

    typedef void ( dummy::*safe_bool)();

    bool            obtained_;
    ptr_t           task_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( packaged_task);

public:
    packaged_task() BOOST_NOEXCEPT :
        obtained_( false),
        task_()
    {
        //TODO: constructs a packaged_task object with
        //       no task and no shared state
    }

    ~packaged_task()
    {
        //TODO: abandons the shared state and destroys the stored task object
        //      a usual, if the shared state is abandoned before it was made
        //      ready, an std::future_error exception is stored with the error
        //      code future_errc::broken_promise
        if ( task_)
            task_->owner_destroyed();
    }

#ifndef BOOST_NO_RVALUE_REFERENCES
#ifdef BOOST_MSVC
    typedef R ( * task_fn)();

    explicit packaged_task( task_fn fn) :
        obtained_( false),
        task_()
    {
        //TODO: constructs a std::packaged_task object
        //       with a shared state and a copy of the task,
        //       initialized with forward< Fn >( fn)
        typedef detail::task_object<
            task_fn,
            std::allocator< packaged_task< R() > >,
            R
        >                                       object_t;
        std::allocator< packaged_task< R() > > alloc;
        typename object_t::allocator_t a( alloc);
        task_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< task_fn >( fn), a) );
    }

    template< typename Allocator >
    explicit packaged_task( boost::allocator_arg_t, Allocator const& alloc, task_fn fn) :
        obtained_( false),
        task_()
    {
        //TODO: constructs a std::packaged_task object
        //       with a shared state and a copy of the task,
        //       initialized with forward< Fn >( fn)
        //       uses the provided allocator to allocate
        //       memory necessary to store the task
        typedef detail::task_object<
            task_fn,
            Allocator,
            R
        >                                       object_t;
        typename object_t::allocator_t a( alloc);
        task_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< task_fn >( fn), a) );
    }
#endif
    template< typename Fn >
    explicit packaged_task( Fn && fn,
                            typename disable_if<
                                is_same<
                                    typename decay< Fn >::type,
                                    packaged_task
                                >,
                                dummy * >::type = 0) :
        obtained_( false),
        task_()
    {
        //TODO: constructs a std::packaged_task object
        //       with a shared state and a copy of the task,
        //       initialized with forward< Fn >( fn)
        typedef detail::task_object<
            Fn,
            std::allocator< packaged_task< R() > >,
            R
        >                                       object_t;
        std::allocator< packaged_task< R() > > alloc;
        typename object_t::allocator_t a( alloc);
        task_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), a) );
    }

    template< typename Fn, typename Allocator >
    explicit packaged_task( boost::allocator_arg_t, Allocator const& alloc, Fn && fn,
                            typename disable_if<
                                is_same<
                                    typename decay< Fn >::type,
                                    packaged_task
                                >,
                                dummy * >::type = 0) :
        obtained_( false),
        task_()
    {
        //TODO: constructs a std::packaged_task object
        //       with a shared state and a copy of the task,
        //       initialized with forward< Fn >( fn)
        //       uses the provided allocator to allocate
        //       memory necessary to store the task
        typedef detail::task_object<
            Fn,
            Allocator,
            R
        >                                       object_t;
        typename object_t::allocator_t a( alloc);
        task_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), a) );
    }
#else
    template< typename Fn >
    explicit packaged_task( Fn fn,
                            typename disable_if<
                                is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                                dummy *
                            >::type = 0) :
        obtained_( false),
        task_()
    {
        //TODO: constructs a std::packaged_task object
        //       with a shared state and a copy of the task,
        //       initialized with forward< Fn >( fn)
        typedef detail::task_object<
            Fn,
            std::allocator< packaged_task< R() > >,
            R
        >                                       object_t;
        std::allocator< packaged_task< R() > > alloc;
        typename object_t::allocator_t a( alloc);
        task_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, a) );
    }

    template< typename Fn, typename Allocator >
    explicit packaged_task( boost::allocator_arg_t, Allocator const& alloc, Fn const& fn,
                            typename disable_if<
                                is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                                dummy *
                            >::type = 0) :
        obtained_( false),
        task_()
    {
        //TODO: constructs a std::packaged_task object
        //       with a shared state and a copy of the task,
        //       initialized with forward< Fn >( fn)
        //       uses the provided allocator to allocate
        //       memory necessary to store the task
        typedef detail::task_object<
            Fn,
            Allocator,
            R
        >                                       object_t;
        typename object_t::allocator_t a( alloc);
        task_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, a) );
    }

    template< typename Fn >
    explicit packaged_task( BOOST_RV_REF( Fn) fn,
                            typename disable_if<
                                is_same< typename decay< Fn >::type, packaged_task >,
                                dummy *
                            >::type = 0) :
        obtained_( false),
        task_()
    {
        //TODO: constructs a std::packaged_task object
        //       with a shared state and a copy of the task,
        //       initialized with forward< Fn >( fn)
        typedef detail::task_object<
            Fn,
            std::allocator< packaged_task< R() > >,
            R
        >                                       object_t;
        std::allocator< packaged_task< R() > > alloc;
        typename object_t::allocator_t a( alloc);
        task_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, a) );
    }

    template< typename Fn, typename Allocator >
    explicit packaged_task( boost::allocator_arg_t, Allocator const& alloc, BOOST_RV_REF( Fn) fn) :
        obtained_( false),
        task_()
    {
        //TODO: constructs a std::packaged_task object
        //       with a shared state and a copy of the task,
        //       initialized with forward< Fn >( fn)
        //       uses the provided allocator to allocate
        //       memory necessary to store the task
        typedef detail::task_object<
            Fn,
            Allocator,
            R
        >                                       object_t;
        typename object_t::allocator_t a( alloc);
        task_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, a) );
    }
#endif

    packaged_task( BOOST_RV_REF( packaged_task) other) BOOST_NOEXCEPT :
        obtained_( false),
        task_()
    {
        //TODO: constructs a std::packaged_task with thes
        //       shared state and task formerly owned by rhs,
        //       leaving rhs with no shared state and a moved-from task
        swap( other);
    }

    packaged_task & operator=( BOOST_RV_REF( packaged_task) other) BOOST_NOEXCEPT
    {
        //TODO: releases the shared state, if any, destroys the
        //       previously-held task, and moves the shared state
        //       and the task owned by rhs into *this
        //       rhs is left without a shared state and with a
        //       moved-from task
        packaged_task tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }

    void swap( packaged_task & other) BOOST_NOEXCEPT
    {
        //TODO: exchange the shared states of two packaged_task
        std::swap( obtained_, other.obtained_);
        task_.swap( other.task_);
    }

    operator safe_bool() const BOOST_NOEXCEPT
    { return valid() ? & dummy::nonnull : 0; }

    bool operator!() const BOOST_NOEXCEPT
    { return ! valid(); }

    bool valid() const BOOST_NOEXCEPT
    {
        //TODO: checks whether *this has a shared state
        return 0 != task_.get();
    }

    future< R > get_future()
    {
        //TODO: returns a future which shares the same shared state as *this
        //      get_future can be called only once for each packaged_task
        if ( obtained_)
            boost::throw_exception(
                future_already_retrieved() );
        if ( ! valid() )
            boost::throw_exception(
                packaged_task_uninitialized() );
        obtained_ = true;
        return future< R >( task_);
    }

    void operator()()
    {
        //TODO: calls the stored task with args as the arguments
        //      the return value of the task or any exceptions thrown are
        //      stored in the shared state
        //      the shared state is made ready and any threads waiting for
        //      this are unblocked
        if ( ! valid() )
            boost::throw_exception(
                packaged_task_uninitialized() );
        task_->run();
    }

    void reset()
    {
        //TODO: resets the state abandoning the results of previous executions
        //      new shared state is constructed
        //      equivalent to *this = packaged_task(std::move(f)), where f is
        //      the stored task
        if ( ! valid() )
            boost::throw_exception(
                packaged_task_uninitialized() );
        obtained_ = false;
        task_->reset();
    }
};

template<>
class packaged_task< void() > : private noncopyable
{
private:
    typedef detail::task_base< void >::ptr_t   ptr_t;

    struct dummy
    { void nonnull() {} };

    typedef void ( dummy::*safe_bool)();

    bool            obtained_;
    ptr_t           task_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( packaged_task);

public:
    packaged_task() BOOST_NOEXCEPT :
        obtained_( false),
        task_()
    {
        //TODO: constructs a packaged_task object with
        //       no task and no shared state
    }

    ~packaged_task()
    {
        //TODO: abandons the shared state and destroys the stored task object
        //      a usual, if the shared state is abandoned before it was made
        //      ready, an std::future_error exception is stored with the error
        //      code future_errc::broken_promise
        if ( task_)
            task_->owner_destroyed();
    }

#ifndef BOOST_NO_RVALUE_REFERENCES
#ifdef BOOST_MSVC
    typedef void ( * task_fn)();

    explicit packaged_task( task_fn fn) :
        obtained_( false),
        task_()
    {
        //TODO: constructs a std::packaged_task object
        //       with a shared state and a copy of the task,
        //       initialized with forward< Fn >( fn)
        typedef detail::task_object<
            task_fn,
            std::allocator< packaged_task< void() > >,
            void
        >                                       object_t;
        std::allocator< packaged_task< void() > > alloc;
        object_t::allocator_t a( alloc);
        task_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< task_fn >( fn), a) );
    }

    template< typename Allocator >
    explicit packaged_task( Allocator const& alloc, task_fn fn) :
        obtained_( false),
        task_()
    {
        //TODO: constructs a std::packaged_task object
        //       with a shared state and a copy of the task,
        //       initialized with forward< Fn >( fn)
        //       uses the provided allocator to allocate
        //       memory necessary to store the task
        typedef detail::task_object<
            task_fn,
            Allocator,
            void
        >                                       object_t;
        object_t::allocator_t a( alloc);
        task_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< task_fn >( fn), a) );
    }
#endif

    template< typename Fn >
    explicit packaged_task( Fn && fn,
                            typename disable_if<
                                is_same<
                                    typename decay< Fn >::type,
                                    packaged_task
                                >,
                                dummy * >::type = 0) :
        obtained_( false),
        task_()
    {
        //TODO: constructs a std::packaged_task object
        //       with a shared state and a copy of the task,
        //       initialized with forward< Fn >( fn)
        typedef detail::task_object<
            Fn,
            std::allocator< packaged_task< void() > >,
            void
        >                                       object_t;
        std::allocator< packaged_task< void() > > alloc;
        typename object_t::allocator_t a( alloc);
        task_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), a) );
    }

    template< typename Fn, typename Allocator >
    explicit packaged_task( Allocator const& alloc, Fn && fn,
                            typename disable_if<
                                is_same<
                                    typename decay< Fn >::type,
                                    packaged_task
                                >,
                                dummy * >::type = 0) :
        obtained_( false),
        task_()
    {
        //TODO: constructs a std::packaged_task object
        //       with a shared state and a copy of the task,
        //       initialized with forward< Fn >( fn)
        //       uses the provided allocator to allocate
        //       memory necessary to store the task
        typedef detail::task_object<
            Fn,
            Allocator,
            void
        >                                       object_t;
        typename object_t::allocator_t a( alloc);
        task_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), a) );
    }

    packaged_task( packaged_task && other) BOOST_NOEXCEPT :
        obtained_( false),
        task_()
    {
        //TODO: constructs a std::packaged_task with thes
        //       shared state and task formerly owned by rhs,
        //       leaving rhs with no shared state and a moved-from task
        swap( other);
    }

    packaged_task & operator=( packaged_task && other) BOOST_NOEXCEPT
    {
        //TODO: releases the shared state, if any, destroys the
        //       previously-held task, and moves the shared state
        //       and the task owned by rhs into *this
        //       rhs is left without a shared state and with a
        //       moved-from task
        packaged_task tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }
#else
    template< typename Fn >
    explicit packaged_task( Fn fn,
                            typename disable_if<
                                is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                                dummy *
                            >::type = 0) :
        obtained_( false),
        task_()
    {
        //TODO: constructs a std::packaged_task object
        //       with a shared state and a copy of the task,
        //       initialized with forward< Fn >( fn)
        typedef detail::task_object<
            Fn,
            std::allocator< packaged_task< void() > >,
            void
        >                                       object_t;
        std::allocator< packaged_task< void() > > alloc;
        typename object_t::allocator_t a( alloc);
        task_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, a) );
    }

    template< typename Fn, typename Allocator >
    explicit packaged_task( Allocator const& alloc, Fn const& fn,
                            typename disable_if<
                                is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                                dummy *
                            >::type = 0) :
        obtained_( false),
        task_()
    {
        //TODO: constructs a std::packaged_task object
        //       with a shared state and a copy of the task,
        //       initialized with forward< Fn >( fn)
        //       uses the provided allocator to allocate
        //       memory necessary to store the task
        typedef detail::task_object<
            Fn,
            Allocator,
            void
        >                                       object_t;
        typename object_t::allocator_t a( alloc);
        task_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, a) );
    }

    template< typename Fn >
    explicit packaged_task( BOOST_RV_REF( Fn) fn,
                            typename disable_if<
                                is_same< typename decay< Fn >::type, packaged_task >,
                                dummy *
                            >::type = 0) :
        obtained_( false),
        task_()
    {
        //TODO: constructs a std::packaged_task object
        //       with a shared state and a copy of the task,
        //       initialized with forward< Fn >( fn)
        typedef detail::task_object<
            Fn,
            std::allocator< packaged_task< void() > >,
            void
        >                                       object_t;
        std::allocator< packaged_task< void() > > alloc;
        typename object_t::allocator_t a( alloc);
        task_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), a) );
    }

    template< typename Fn, typename Allocator >
    explicit packaged_task( Allocator const& alloc, BOOST_RV_REF( Fn) fn) :
        obtained_( false),
        task_()
    {
        //TODO: constructs a std::packaged_task object
        //       with a shared state and a copy of the task,
        //       initialized with forward< Fn >( fn)
        //       uses the provided allocator to allocate
        //       memory necessary to store the task
        typedef detail::task_object<
            Fn,
            Allocator,
            void
        >                                       object_t;
        typename object_t::allocator_t a( alloc);
        task_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), a) );
    }

    packaged_task( BOOST_RV_REF( packaged_task) other) BOOST_NOEXCEPT :
        obtained_( false),
        task_()
    {
        //TODO: constructs a std::packaged_task with thes
        //       shared state and task formerly owned by rhs,
        //       leaving rhs with no shared state and a moved-from task
        swap( other);
    }

    packaged_task & operator=( BOOST_RV_REF( packaged_task) other) BOOST_NOEXCEPT
    {
        //TODO: releases the shared state, if any, destroys the
        //       previously-held task, and moves the shared state
        //       and the task owned by rhs into *this
        //       rhs is left without a shared state and with a
        //       moved-from task
        packaged_task tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }
#endif

    void swap( packaged_task & other) BOOST_NOEXCEPT
    {
        //TODO: exchange the shared states of two packaged_task
        std::swap( obtained_, other.obtained_);
        task_.swap( other.task_);
    }

    operator safe_bool() const BOOST_NOEXCEPT
    { return valid() ? & dummy::nonnull : 0; }

    bool operator!() const BOOST_NOEXCEPT
    { return ! valid(); }

    bool valid() const BOOST_NOEXCEPT
    {
        //TODO: checks whether *this has a shared state
        return 0 != task_.get();
    }

    future< void > get_future()
    {
        //TODO: returns a future which shares the same shared state as *this
        //      get_future can be called only once for each packaged_task
        if ( obtained_)
            boost::throw_exception(
                future_already_retrieved() );
        if ( ! valid() )
            boost::throw_exception(
                packaged_task_uninitialized() );
        obtained_ = true;
        return future< void >( task_);
    }

    void operator()()
    {
        //TODO: calls the stored task with args as the arguments
        //      the return value of the task or any exceptions thrown are
        //      stored in the shared state
        //      the shared state is made ready and any threads waiting for
        //      this are unblocked
        if ( ! valid() )
            boost::throw_exception(
                packaged_task_uninitialized() );
        task_->run();
    }

    void reset()
    {
        //TODO: resets the state abandoning the results of previous executions
        //      new shared state is constructed
        //      equivalent to *this = packaged_task(std::move(f)), where f is
        //      the stored task
        if ( ! valid() )
            boost::throw_exception(
                packaged_task_uninitialized() );
        obtained_ = false;
        task_->reset();
    }
};

template< typename Signature >
void swap( packaged_task< Signature > & l, packaged_task< Signature > & r)
{ l.swap( r); }

}}

#endif // BOOST_FIBERS_PACKAGED_TASK_HPP
