namespace boost {
namespace fibers {
namespace asio {

inline void timer_handler(deadline_timer& timer) {
    detail::scheduler::instance()->yield();
    boost::system_clock::time_point wakeup = detail::scheduler::instance()->next_wakeup();

    timer.expires_from_now(wakeup);
    timer.async_wait(boost::bind(timer_handler, boost::ref(timer));
}

inline void run_service(boost::asio::io_service& io_service) {
    deadline_timer timer(io_service, boost::posix_time::seconds(0));
    timer.async_wait(boost::bind(timer_handler, boost::ref(timer));

    io_service.run();
}

}
}
}

