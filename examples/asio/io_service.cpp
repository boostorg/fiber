io_service::io_service( boost::asio::io_service & svc) BOOST_NOEXCEPT :
    io_service_( svc),
    active_fiber_()
{}

io_service::~io_service() BOOST_NOEXCEPT
{
#if 0
    BOOST_FOREACH( detail::fiber_base::ptr_t const& p, rqueue_)
    { p->release(); }

    BOOST_FOREACH( detail::fiber_base::ptr_t const& p, wqueue_)
    { p->release(); }
#endif
}

void
io_service::spawn( detail::fiber_base::ptr_t const& f)
{
    BOOST_ASSERT( f);
    BOOST_ASSERT( f->is_ready() );

    // store active fiber in local var
    detail::fiber_base::ptr_t tmp = active_fiber_;
    try
    {
        // assign new fiber to active fiber
        active_fiber_ = f;
        // set active fiber to state_running
        active_fiber_->set_running();
        // resume active fiber
        active_fiber_->resume();
        // fiber is resumed

        BOOST_ASSERT( f == active_fiber_);
    }
    catch (...)
    {
        // reset active fiber to previous
        active_fiber_ = tmp;
        throw;
    }
    // reset active fiber to previous
    active_fiber_ = tmp;
}

bool
io_service::run()
{
    // loop over waiting queue
    wqueue_t wqueue;
    wqueue_work_t wqueue_work;
    BOOST_FOREACH( detail::fiber_base::ptr_t const& f, wqueue_)
    {
        BOOST_ASSERT( ! f->is_running() );
        BOOST_ASSERT( ! f->is_terminated() );

        // set fiber to state_ready if interruption was requested
        // or the fiber was woken up
        if ( f->interruption_requested() )
            f->set_ready();
        if ( f->is_ready() )
        {
            io_service_.post(
              [this, f]()
              {
                  if ( f->is_waiting() )
                  {
                      wqueue_.push_back( f);
                      wqueue_work_.push_back( boost::asio::io_service::work( io_service_) );
                  }
                  else if ( f->is_ready() ) spawn( f);
                  else BOOST_ASSERT_MSG( false, "fiber with invalid state in ready-queue");
              });
        }
        else
        {
            wqueue.push_back( f);
            wqueue_work.push_back( boost::asio::io_service::work( io_service_) );
        }
    }
    // exchange local with global waiting queue
    wqueue_.swap( wqueue);
    wqueue_work_.swap( wqueue_work);

    return io_service_.run_one() > 0;
}

void
io_service::wait()
{
    BOOST_ASSERT( active_fiber_);
    BOOST_ASSERT( active_fiber_->is_running() );

    // set active_fiber to state_waiting
    active_fiber_->set_waiting();
    // push active fiber to wqueue_
    wqueue_.push_back( active_fiber_);
    wqueue_work_.push_back( boost::asio::io_service::work( io_service_) );
    // store active fiber in local var
    detail::fiber_base::ptr_t tmp = active_fiber_;
    // suspend active fiber
    active_fiber_->suspend();
    // fiber is resumed

    BOOST_ASSERT( tmp == active_fiber_);
    BOOST_ASSERT( active_fiber_->is_running() );
}

void
io_service::yield()
{
    BOOST_ASSERT( active_fiber_);
    BOOST_ASSERT( active_fiber_->is_running() );

    // set active fiber to state_waiting
    active_fiber_->set_ready();
    // push active fiber to wqueue_
    wqueue_.push_back( active_fiber_);
    wqueue_work_.push_back( boost::asio::io_service::work( io_service_) );
    // store active fiber in local var
    detail::fiber_base::ptr_t tmp = active_fiber_;
    // suspend acitive fiber
    active_fiber_->suspend();
    // fiber is resumed

    BOOST_ASSERT( tmp == active_fiber_);
    BOOST_ASSERT( active_fiber_->is_running() );
}

void
io_service::join( detail::fiber_base::ptr_t const& f)
{
    BOOST_ASSERT( f);
    BOOST_ASSERT( f != active_fiber_);

    if ( active_fiber_)
    {
        // set active fiber to state_waiting
        active_fiber_->set_waiting();
        // push active fiber to wqueue_
        wqueue_.push_back( active_fiber_);
        wqueue_work_.push_back( boost::asio::io_service::work( io_service_));
        // add active fiber to joinig-list of f
        if ( ! f->join( active_fiber_) )
            // f must be already terminated therefore we set
            // active fiber to state_ready
            // FIXME: better state_running and no suspend
            active_fiber_->set_ready();
        // store active fiber in local var
        detail::fiber_base::ptr_t tmp = active_fiber_;
        // suspend fiber until f terminates
        active_fiber_->suspend();
        // fiber is resumed by f

        BOOST_ASSERT( tmp == active_fiber_);
        BOOST_ASSERT( active_fiber_->is_running() );

        // check if fiber was interrupted
        this_fiber::interruption_point();
    }
    else
    {
        while ( ! f->is_terminated() )
            run();
    }

    // check if joined fiber has an exception
    // and rethrow exception
    if ( f->has_exception() ) f->rethrow();

    BOOST_ASSERT( f->is_terminated() );
}
