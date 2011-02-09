/*
 * Boost.Extension / fruit interface
 *
 * (C) Copyright Jeremy Pack 2007
 * Distributed under the Boost Software License, Version 1.0. (See             
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

#ifndef BOOST_EXTENSION_TEST_FRUIT_HPP
#define BOOST_EXTENSION_TEST_FRUIT_HPP

class fruit
{
public:
  fruit(int a = 0, int b = 0){}
  virtual ~fruit(){}
  virtual int get_cost(){return 5;}
};
class banana : public fruit
{
public:
  banana(int a = 0, int b = 0){}

  virtual int get_cost(){return 7;}
};
class nectarine : public fruit
{
public:
  nectarine(int a = 0, int b = 0){}

  virtual int get_cost(){return 18;}
};
class apple : public fruit
{
public:
  apple(int a = 0, int b = 0){}

  virtual int get_cost(){return 21;}
  static bool meets_requirements(int val){return val == 8;}
};

class vegetable
{
private:
  float f_;
public:
  virtual ~vegetable(){}
  vegetable(float f):f_(f){}
  virtual float get_weight(){return f_ * 2.0f;}
};

//I know it's not really a vegetable - but it as used as one
class tomato : public fruit, public vegetable
{
public:
  virtual ~tomato(){}
  tomato(float f, int a, int b):fruit(a, b),vegetable(f){}
  tomato(float f):fruit(0, 0), vegetable(f){}
  tomato(int a, int b):fruit(a, b), vegetable(0){}
  virtual float get_weight(){return 1.5 * vegetable::get_weight();}
};
class vegetable_info
{
private:
  std::string name_;
  int num_calories_;
public:
  vegetable_info(const char * name, int calories)
    :name_(name), num_calories_(calories){}
  int get_calories() const {return num_calories_;}
  const char * get_name() const {return name_.c_str();}
  friend inline bool 
    operator<(const vegetable_info & first,
              const vegetable_info & second) {
    return first.name_ < second.name_ ||
           (first.name_ == second.name_ &&
            first.num_calories_ < second.num_calories_);
  }
  
};
#endif
