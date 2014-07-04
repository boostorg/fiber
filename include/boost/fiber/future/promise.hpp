
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_PROMISE_HPP
#define BOOST_FIBERS_PROMISE_HPP

#include <memory>

#include <boost/config.hpp>
#include <boost/move/move.hpp>
#include <boost/thread/detail/memory.hpp> // boost::allocator_arg_t
#include <boost/throw_exception.hpp>
#include <boost/utility/explicit_operator_bool.hpp>
#include <boost/utility.hpp>

#include <boost/fiber/exceptions.hpp>
#include <boost/fiber/future/detail/shared_state.hpp>
#include <boost/fiber/future/detail/shared_state_object.hpp>
#include <boost/fiber/future/future.hpp>

namespace boost {
namespace fibers {

template< typename R >
class promise : private noncopyable
{
private:
    typedef typename detail::shared_state< R >::ptr_t   ptr_t;

    bool            obtained_;
    ptr_t           future_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( promise);

public:
    promise() :
        obtained_( false),
        future_()
    {
        // TODO: constructs the promise with an empty shared state
        //       the shared state is allocated using alloc
        //       alloc must meet the requirements of Allocator
        typedef detail::shared_state_object<
            R, std::allocator< promise< R > >
        >                                               object_t;
        std::allocator< promise< R > > alloc;
        typename object_t::allocator_t a( alloc);
        future_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( a) );
    }

    template< typename Allocator >
    promise( boost::allocator_arg_t, Allocator alloc) :
        obtained_( false),
        future_()
    {
        // TODO: constructs the promise with an empty shared state
        //       the shared state is allocated using alloc
        //       alloc must meet the requirements of Allocator
        typedef detail::shared_state_object< R, Allocator >  object_t;
        typename object_t::allocator_t a( alloc);
        future_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( a) );
    }

    ~promise()
    {
        //TODO: abandon ownership if any
        if ( future_)
            future_->owner_destroyed();
    }

#ifndef BOOST_NO_RVALUE_REFERENCES
    promise( promise && other) BOOST_NOEXCEPT :
        obtained_( false),
        future_()
    {
        //TODO: take over ownership
        //      other is valid before but in
        //      undefined state afterwards
        swap( other);
    }

    promise & operator=( promise && other) BOOST_NOEXCEPT
    {
        //TODO: take over ownership
        //      other is valid before but in
        //      undefined state afterwards
        promise tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }
#else
    promise( BOOST_RV_REF( promise) other) BOOST_NOEXCEPT :
        obtained_( false),
        future_()
    {
        //TODO: take over ownership
        //      other is valid before but in
        //      undefined state afterwards
        swap( other);
    }

    promise & operator=( BOOST_RV_REF( promise) other) BOOST_NOEXCEPT
    {
        //TODO: take over ownership
        //      other is valid before but in
        //      undefined state afterwards
        promise tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }
#endif   

    void swap( promise & other) BOOST_NOEXCEPT
    {
        //TODO: exchange the shared states of two promises
        std::swap( obtained_, other.obtained_);
        future_.swap( other.future_);
    }

    BOOST_EXPLICIT_OPERATOR_BOOL();

    bool operator!() const BOOST_NOEXCEPT
    { return 0 == future_.get(); }

    future< R > get_future()
    {
        //TODO: returns a future object associated with the same shared state
        //      exception is thrown if *this has no shared state or get_future
        //      has already been called. 
        if ( obtained_)
            boost::throw_exception(
                future_already_retrieved() );
        if ( ! future_)
            boost::throw_exception(
                promise_uninitialized() );
        obtained_ = true;
        return future< R >( future_);
    }

    void set_value( R const& value)
    {
        //TODO: store the value into the shared state and make the state ready
        //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
        //      associated with the promise object while updating the promise object
        //      an exception is thrown if there is no shared state or the shared state already
        //      stores a value or exception
        if ( ! future_)
            boost::throw_exception(
                promise_uninitialized() );
        future_->set_value( value);
    }

#ifndef BOOST_NO_RVALUE_REFERENCES
    void set_value( R && value)
    {
        //TODO: store the value into the shared state and make the state ready
        //      rhe operation is atomic, i.e. it behaves as though they acquire a single mutex
        //      associated with the promise object while updating the promise object
        //      an exception is thrown if there is no shared state or the shared state already
        //      stores a value or exception
        if ( ! future_)
            boost::throw_exception(
                promise_uninitialized() );
        future_->set_value( boost::move( value) );
    }
#else
    void set_value( BOOST_RV_REF( R) value)
    {
        //TODO: store the value into the shared state and make the state ready
        //      rhe operation is atomic, i.e. it behaves as though they acquire a single mutex
        //      associated with the promise object while updating the promise object
        //      an exception is thrown if there is no shared state or the shared state already
        //      stores a value or exception
        if ( ! future_)
            boost::throw_exception(
                promise_uninitialized() );
        future_->set_value( boost::move( value) );
    }
#endif

    void set_exception( exception_ptr p)
    {
        //TODO: store the exception pointer p into the shared state and make the state ready
        //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
        //      associated with the promise object while updating the promise object
        //      an exception is thrown if there is no shared state or the shared state already
        //      stores a value or exception
        if ( ! future_)
            boost::throw_exception(
                promise_uninitialized() );
        future_->set_exception( p);
    }
};

template< typename R >
class promise< R & > : private noncopyable
{
private:
    typedef typename detail::shared_state< R & >::ptr_t   ptr_t;

    bool            obtained_;
    ptr_t           future_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( promise);

public:
    promise() :
        obtained_( false),
        future_()
    {
        // TODO: constructs the promise with an empty shared state
        //       the shared state is allocated using alloc
        //       alloc must meet the requirements of Allocator
        typedef detail::shared_state_object<
            R &, std::allocator< promise< R & > >
        >                                               object_t;
        std::allocator< promise< R > > alloc;
        typename object_t::allocator_t a( alloc);
        future_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( a) );
    }

    template< typename Allocator >
    promise( boost::allocator_arg_t, Allocator alloc) :
        obtained_( false),
        future_()
    {
        // TODO: constructs the promise with an empty shared state
        //       the shared state is allocated using alloc
        //       alloc must meet the requirements of Allocator
        typedef detail::shared_state_object< R &, Allocator >  object_t;
        typename object_t::allocator_t a( alloc);
        future_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( a) );
    }

    ~promise()
    {
        //TODO: abandon ownership if any
        if ( future_)
            future_->owner_destroyed();
    }

#ifndef BOOST_NO_RVALUE_REFERENCES
    promise( promise && other) BOOST_NOEXCEPT :
        obtained_( false),
        future_()
    {
        //TODO: take over ownership
        //      other is valid before but in
        //      undefined state afterwards
        swap( other);
    }

    promise & operator=( promise && other) BOOST_NOEXCEPT
    {
        //TODO: take over ownership
        //      other is valid before but in
        //      undefined state afterwards
        promise tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }
#else
    promise( BOOST_RV_REF( promise) other) BOOST_NOEXCEPT :
        obtained_( false),
        future_()
    {
        //TODO: take over ownership
        //      other is valid before but in
        //      undefined state afterwards
        swap( other);
    }

    promise & operator=( BOOST_RV_REF( promise) other) BOOST_NOEXCEPT
    {
        //TODO: take over ownership
        //      other is valid before but in
        //      undefined state afterwards
        promise tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }
#endif

    void swap( promise & other) BOOST_NOEXCEPT
    {
        //TODO: exchange the shared states of two promises
        std::swap( obtained_, other.obtained_);
        future_.swap( other.future_);
    }

    BOOST_EXPLICIT_OPERATOR_BOOL();

    bool operator!() const BOOST_NOEXCEPT
    { return 0 == future_.get(); }

    future< R & > get_future()
    {
        //TODO: returns a future object associated with the same shared state
        //      exception is thrown if *this has no shared state or get_future
        //      has already been called.
        if ( obtained_)
            boost::throw_exception(
                future_already_retrieved() );
        if ( ! future_)
            boost::throw_exception(
                promise_uninitialized() );
        obtained_ = true;
        return future< R & >( future_);
    }

    void set_value( R & value)
    {
        //TODO: store the value into the shared state and make the state ready
        //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
        //      associated with the promise object while updating the promise object
        //      an exception is thrown if there is no shared state or the shared state already
        //      stores a value or exception
        if ( ! future_)
            boost::throw_exception(
                promise_uninitialized() );
        future_->set_value( value);
    }

    void set_exception( exception_ptr p)
    {
        //TODO: store the exception pointer p into the shared state and make the state ready
        //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
        //      associated with the promise object while updating the promise object
        //      an exception is thrown if there is no shared state or the shared state already
        //      stores a value or exception
        if ( ! future_)
            boost::throw_exception(
                promise_uninitialized() );
        future_->set_exception( p);
    }
};

template<>
class promise< void > : private noncopyable
{
private:
    typedef detail::shared_state< void >::ptr_t   ptr_t;

    bool            obtained_;
    ptr_t           future_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( promise);

public:
    promise() :
        obtained_( false),
        future_()
    {
        // TODO: constructs the promise with an empty shared state
        //       the shared state is allocated using alloc
        //       alloc must meet the requirements of Allocator
        typedef detail::shared_state_object<
            void, std::allocator< promise< void > >
        >                                               object_t;
        std::allocator< promise< void > > alloc;
        object_t::allocator_t a( alloc);
        future_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( a) );
    }

    template< typename Allocator >
    promise( boost::allocator_arg_t, Allocator alloc) :
        obtained_( false),
        future_()
    {
        // TODO: constructs the promise with an empty shared state
        //       the shared state is allocated using alloc
        //       alloc must meet the requirements of Allocator
        typedef detail::shared_state_object< void, Allocator >  object_t;
#if BOOST_MSVC
        object_t::allocator_t a( alloc);
#else
        typename object_t::allocator_t a( alloc);
#endif
        future_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( a) );
    }

    ~promise()
    {
        //TODO: abandon ownership if any
        if ( future_)
            future_->owner_destroyed();
    }

#ifndef BOOST_NO_RVALUE_REFERENCES
    promise( promise && other) BOOST_NOEXCEPT :
        obtained_( false),
        future_()
    {
        //TODO: take over ownership
        //      other is valid before but in
        //      undefined state afterwards
        swap( other);
    }

    promise & operator=( promise && other) BOOST_NOEXCEPT
    {
        //TODO: take over ownership
        //      other is valid before but in
        //      undefined state afterwards
        promise tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }
#else
    promise( BOOST_RV_REF( promise) other) BOOST_NOEXCEPT :
        obtained_( false),
        future_()
    {
        //TODO: take over ownership
        //      other is valid before but in
        //      undefined state afterwards
        swap( other);
    }

    promise & operator=( BOOST_RV_REF( promise) other) BOOST_NOEXCEPT
    {
        //TODO: take over ownership
        //      other is valid before but in
        //      undefined state afterwards
        promise tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }
#endif   

    void swap( promise & other) BOOST_NOEXCEPT
    {
        //TODO: exchange the shared states of two promises
        std::swap( obtained_, other.obtained_);
        future_.swap( other.future_);
    }

    BOOST_EXPLICIT_OPERATOR_BOOL();

    bool operator!() const BOOST_NOEXCEPT
    { return 0 == future_.get(); }

    future< void > get_future()
    {
        //TODO: returns a future object associated with the same shared state
        //      exception is thrown if *this has no shared state or get_future
        //      has already been called. 
        if ( obtained_)
            boost::throw_exception(
                future_already_retrieved() );
        if ( ! future_)
            boost::throw_exception(
                promise_uninitialized() );
        obtained_ = true;
        return future< void >( future_);
    }

    void set_value()
    {
        //TODO: store the value into the shared state and make the state ready
        //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
        //      associated with the promise object while updating the promise object
        //      an exception is thrown if there is no shared state or the shared state already
        //      stores a value or exception
        if ( ! future_)
            boost::throw_exception(
                promise_uninitialized() );
        future_->set_value();
    }

    void set_exception( exception_ptr p)
    {
        //TODO: store the exception pointer p into the shared state and make the state ready
        //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
        //      associated with the promise object while updating the promise object
        //      an exception is thrown if there is no shared state or the shared state already
        //      stores a value or exception
        if ( ! future_)
            boost::throw_exception(
                promise_uninitialized() );
        future_->set_exception( p);
    }
};

template< typename R >
void swap( promise< R > & l, promise< R > & r)
{ l.swap( r); }

}}

#endif // BOOST_FIBERS_PROMISE_HPP
