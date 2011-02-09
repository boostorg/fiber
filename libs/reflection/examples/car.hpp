/*
 * Boost.Reflection / basic example (car interface)
 *
 * (C) Copyright Mariano G. Consoni 2007
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */


#include <iostream>
#include <string>

class car {
private:
  std::string name_;
  bool started_;

public:
  car(std::string car_name) : name_(car_name), started_(false) {}

  bool start(void) { 
    std::cout << name_ << " started." << std::endl; 
    started_ = true;
    return true;
  }

  bool turn(float angle)
  {
    if(started_) {
      std::cout << "Turning " << name_ << " "<< angle << " degrees." 
                << std::endl;
      return true;
    } else {
      std::cout << "Cannot turn before starting the engine of " 
                << name_ << "." << std::endl;
      return false;
    }
  }

  void accelerate(float qty, bool direction)
  {
    if(started_) {
      std::cout << "Accelerating " << name_ << " "<< qty << " m/h." 
                << std::endl;
    } else {
      std::cout << "Cannot accelerate before starting the engine of " 
                << name_ << "." << std::endl;
    }
  }

  virtual ~car(void) {}
}; 
