#ifndef BOOST_FIBERS_DETAIL_CONTAINER_HPP
#define BOOST_FIBERS_DETAIL_CONTAINER_HPP

#include <algorithm>
#include <functional>
#include <utility>
#include <vector>

#include <boost/assert.hpp>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/fiber_base.hpp>
#include <boost/fiber/detail/states.hpp>

namespace boost {
namespace fibers {
namespace detail {

struct comparator
{
    bool operator()(
        fiber_base::ptr_t const& l,
        fiber_base::ptr_t const& r) const
    { return l->state() < r->state(); }

    bool operator()(
        state_t const& l,
        fiber_base::ptr_t const& r) const
    { return l < r->state(); }

    bool operator()(
        fiber_base::ptr_t const& l,
        state_t const& r) const
    { return l->state() < r; }
};

template <
    class A = std::allocator< fiber_base::ptr_t >
>
class container
{
private:
    typedef std::vector< fiber_base::ptr_t, A >     base_t;

    base_t                                          base_;

public:
    typedef state_t                                 key_type;
    typedef typename base_t::value_type             value_type;

    typedef A                                       allocator_type;
    typedef typename base_t::iterator               iterator;
    typedef typename base_t::const_iterator         const_iterator;
    typedef typename base_t::size_type              size_type;
    typedef typename base_t::difference_type        difference_type;

    container() :
        base_()
    {}

    container( container const& other) :
        base_( other.base_)
    {}

    container& operator=( container const& other)
    { 
        container( other).swap( * this); 
        return * this;
    }
    
    bool empty() const
    { return base_.empty(); }

    size_type size() const
    { return base_.size(); }

    void sort()
    { std::stable_sort( base_.begin(), base_.end(), comparator() ); }

    void push_back( fiber_base::ptr_t const& f)
    {
        BOOST_ASSERT( f);
        base_.push_back( f);
    }

    void erase( iterator const& i)
    { base_.erase( i); }

    void erase( iterator const& i, iterator const& e)
    { base_.erase( i, e); }

    void swap( container & other)
    { base_.swap( other); }

    void clear()
    { base_.clear(); }

    iterator begin()
    { return base_.begin(); }

    const_iterator begin() const
    { return base_.begin(); }

    iterator end()
    { return base_.end(); }

    const_iterator end() const
    { return base_.end(); }

    iterator lower_bound( key_type const& k)
    {
        return std::lower_bound(
                base_.begin(), base_.end(),
                k, comparator() );
    }

    const_iterator lower_bound( key_type const& k) const
    {
        return std::lower_bound(
                base_.begin(), base_.end(),
                k, comparator() );
    }

    iterator upper_bound( key_type const& k)
    {
        return std::upper_bound(
                base_.begin(), base_.end(),
                k, comparator() );
    }

    const_iterator upper_bound( key_type const& k) const
    {
        return std::upper_bound(
                base_.begin(), base_.end(),
                k, comparator() );
    }

    std::pair< iterator, iterator > equal_range( key_type const& k)
    {
        return std::equal_range(
                base_.begin(), base_.end(),
                k, comparator() );
    }

    std::pair< const_iterator, const_iterator > equal_range( key_type const& k) const
    {
        return std::equal_range(
                base_.begin(), base_.end(),
                k, comparator() );
    }

    std::size_t capacity() const
    { return base_.capacity(); }

    void reserve( std::size_t size)
    { base_.reserve( size); }
};

template< typename A >
void swap( container< A > & l, container< A > & r)
{ l.swap( r); }
    
}}}

#endif // BOOST_FIBERS_DETAIL_CONTAINER_HPP
