
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/exceptions.hpp"

namespace boost {
namespace fibers {

class future_error_category : public std::error_category {
public:
    virtual const char* name() const noexcept {
        return "future";
    }

    virtual std::error_condition default_error_condition( int ev) const noexcept {
        switch ( ev) {
            case 1: return std::error_condition(
                            static_cast< int >( future_errc::broken_promise),
                            future_category() );
            case 2: return std::error_condition(
                            static_cast< int >( future_errc::future_already_retrieved),
                            future_category() );
            case 3: return std::error_condition(
                            static_cast< int >( future_errc::promise_already_satisfied),
                            future_category() );
            case 4: return std::error_condition(
                            static_cast<
                            int >( future_errc::no_state),
                            future_category() );
            default:
                    return std::error_condition(
                            static_cast<
                            int >( future_errc::unknown),
                            future_category() );
        }
    }

      virtual bool equivalent( std::error_code const& code, int condition) const noexcept {
          return * this == code.category() &&
              static_cast< int >( default_error_condition( code.value() ).value() ) == condition;
      }

    virtual std::string message( int ev) const {
        switch ( static_cast< future_errc >( ev) ) {
        case future_errc::unknown:
            return std::string("Unknown error");
        case future_errc::broken_promise:
            return std::string("The associated promise has been destructed prior "
                          "to the associated state becoming ready.");
        case future_errc::future_already_retrieved:
            return std::string("The future has already been retrieved from "
                          "the promise or packaged_task.");
        case future_errc::promise_already_satisfied:
            return std::string("The state of the promise has already been set.");
        case future_errc::no_state:
            return std::string("Operation not permitted on an object without "
                          "an associated state.");
        }
        return std::string("unspecified future_errc value\n");
    }
};

std::error_category const& future_category() noexcept {
    static fibers::future_error_category cat;
    return cat;
}

}}
