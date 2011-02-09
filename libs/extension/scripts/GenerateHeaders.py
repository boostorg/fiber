# Boost.Extension - GenerateHeaders script
#       Use for generation of factory.h and shared_library.h
#
# Copyright 2007 Jeremy Pack
#
# See http://www.boost.org/ for latest version.
#

import sys

class header_generator:
  def __init__(self, max_params):
    self.max_params = max_params
    
  def generate(self):
    self.generate_factory_hpp()
    self.generate_counted_factory_hpp()
    self.generate_shared_library_hpp()
    self.generate_factory_map_hpp("factory", "", "", "", """
      factory_container() {}
     // factory_container(basic_factory_map & z)
      //  :std::list<factory<Interface, Info, Param1, Param2, Param3, Param4, Param5, Param6> >(z.get<Interface, Param1, Param2, Param3, Param4, Param5, Param6>()){}
      virtual ~factory_container(){}
    """)
    self.generate_factory_map_hpp("counted_factory",
      "\n    f.set_library(current_library_.c_str());\n    f.set_counter(current_counter_);",
      "\n    std::string current_library_;\n    int default_counter_;\n    int * current_counter_;",
      "\n    virtual bool remove_library(const char * library_name) = 0;", """
    int * counter_;
    factory_container(int * counter) : counter_(counter) {++(*counter_);}
    virtual ~factory_container() {--(*counter_);}
    virtual bool remove_library(const char * library_name)
    {
      for (typename std::list<counted_factory<Interface, Info, Param1, Param2, Param3, 
              Param4, Param5, Param6> >::iterator it = this->begin(); 
              it != this->end();)
      {
        if (strcmp(it->library(), library_name) == 0)
          this->erase(it++); 
        else
          ++it;
      }
      return this->size() == 0;
    }""", 
      "\n    basic_counted_factory_map() : default_counter_(0), current_counter_(&default_counter_){}",
      "current_counter_")
      
  def template_header(self, start_string, start, count, add_void):
    if add_void:
      v = " = void"
    else:
      v = ""
    center = ", ".join(start + ["class Param" + str(i) + v 
                        for i in range(1, count + 1)])
    out_str = "".join([start_string, center, ">\n"])
    return out_str
    
  def nameless_param_list(self, count, start):
    return ", ".join(start + ["Param" + str(i) for i in range(1, count+1)])
    
  def param_names(self, count, start):
    return ", ".join(start + ["p" + str(i) for i in range(1, count+1)])
    
  def named_param_list(self, count, start):
    return ", ".join(start + ["".join(["Param", str(i), " p", str(i)]) 
                      for i in range(1, count+1)])
  
  def generate_factory_hpp(self):
    out = open('../../../boost/extension/factory.hpp', mode='w')
    out.write("""/* (C) Copyright Jeremy Pack 2007
* Distributed under the Boost Software License, Version 1.0. (See
* accompanying file LICENSE_1_0.txt or copy at
* http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_EXTENSION_FACTORY_HPP
#define BOOST_EXTENSION_FACTORY_HPP
#include <string>
namespace boost{namespace extensions{
  """)
    for i in range(self.max_params, -1, -1):
      out.write(self.template_header('template <', 
                                     ['class Interface', 'class Info'], 
                                     i, i == self.max_params))
      if  i != self.max_params:
        factory_template = ("".join(["<",
                            self.nameless_param_list(i, ["Interface", "Info"]),
                            ">\n"]))
      else:
        factory_template = "\n"
      out.write("".join(["class factory",
        factory_template, 
        """{
protected:
  class generic_factory_function
  {
  public:
    virtual ~generic_factory_function(){}
    virtual Interface * operator()(""",
    self.nameless_param_list(i, []),
    """) = 0;
    virtual generic_factory_function * copy() const = 0;
  };
  template <class T>
  class factory_function : public generic_factory_function
  {
  public:
    virtual ~factory_function(){}
    virtual Interface * operator()
      (""", 
    self.named_param_list(i, []),
      """)
    {return new T(""",
    self.param_names(i, []),
    """);}
    virtual generic_factory_function * copy() const {return new factory_function<T>;}
  };
  std::auto_ptr<generic_factory_function> factory_func_ptr_;
  Info info_;
public:
  template <class Actual>
    void set_type_special(Actual *){factory_func_ptr_.reset(new factory_function<Actual>());}
  template <class Actual>
    void set_type(){factory_func_ptr_ = new factory_function<Actual>();}
  factory(Info info)
    :factory_func_ptr_(0),
    info_(info)
  {}
  factory(const factory & first)
    :factory_func_ptr_(first.factory_func_ptr_->copy()),
    info_(first.info_)
                       {}
  Interface * operator()(""",
    self.named_param_list(i, []),
    """)
    {return create(""",
    self.param_names(i, []),
    """);}
  Interface * create(""",
    self.named_param_list(i, []),
    """){return (*factory_func_ptr_)
  (""",
    self.param_names(i, []),
    """);}
  Info & get_info(){return info_;}
};
"""]))  
   # For loop over!
    out.write("""}}
#endif
""")

  def generate_counted_factory_hpp(self):
    out = open('../../../boost/extension/counted_factory.hpp', mode='w')
    out.write("""/* (C) Copyright Jeremy Pack 2007
* Distributed under the Boost Software License, Version 1.0. (See
* accompanying file LICENSE_1_0.txt or copy at
* http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_EXTENSION_COUNTED_FACTORY_HPP
#define BOOST_EXTENSION_COUNTED_FACTORY_HPP
#include <string>
namespace boost{namespace extensions{
  """)
    for i in range(self.max_params, -1, -1):
      out.write(self.template_header('template <', 
                                     ['class Interface', 'class Info'], 
                                     i, i == self.max_params))
      if  i != self.max_params:
        factory_template = ("".join(["<",
                            self.nameless_param_list(i, ["Interface", "Info"]),
                            ">\n"]))
      else:
        factory_template = "\n"
      out.write("".join(["class counted_factory",
        factory_template, 
        """{
protected:
  int * counter_;
  std::string library_;
  class generic_factory_function
  {
  public:
    virtual ~generic_factory_function(){}
    virtual Interface * operator()(""",
    self.nameless_param_list(i, ["int * counter"]),
    """) = 0;
    virtual generic_factory_function * copy() const = 0;
  };
  template <class T>
  class factory_function : public generic_factory_function
  {
  public:
    class counted_object : public T
    {
    private:
      int * counter_;
    public:
      counted_object(""",
      self.named_param_list(i, ["int * counter"]),
      """) : T(""",
      self.param_names(i, []),"""),
      counter_(counter)
      {
        ++(*counter_);
      }
      ~counted_object()
      {
        --(*counter_);
      }  
    };
    virtual ~factory_function(){}
    virtual Interface * operator()
      (""", 
    self.named_param_list(i, ["int * counter"]),
      """)
    {  // A compilation error here usually indicates that the
       // class you are adding is not derived from the base class
       // that you indicated.
      return static_cast<Interface*>(new counted_object(""",
    self.param_names(i, ["counter"]),
    """));}
    virtual generic_factory_function * copy() const {return new factory_function<T>;}
  };
  std::auto_ptr<generic_factory_function> factory_func_ptr_;
  Info info_;
public:
  void set_library(const char * library_name)
  {
    library_ = library_name;
  }
  void set_counter(int * counter)
  {
    counter_ = counter;
  }
  const char * library()
  {
    return library_.c_str();
  }
  template <class Actual>
    void set_type_special(Actual *){factory_func_ptr_.reset(new factory_function<Actual>());}
  template <class Actual>
    void set_type(){factory_func_ptr_ = new factory_function<Actual>();}
  counted_factory(Info info)
    :factory_func_ptr_(0),
    info_(info)
  {}
  counted_factory(const counted_factory & first)
    :counter_(first.counter_),
    library_(first.library_),
    factory_func_ptr_(first.factory_func_ptr_->copy()),
    info_(first.info_)
                       {}
  Interface * operator()(""",
    self.named_param_list(i, ["int * counter"]),
    """)
    {return create(""",
    self.param_names(i, ["counter"]),
    """);}
  Interface * create(""",
    self.named_param_list(i, []),
    """){return (*factory_func_ptr_)
  (""",
    self.param_names(i, ["counter_"]),
    """);}
  Info & get_info(){return info_;}
};
"""]))  
   # For loop over!
    out.write("""}}
#endif
""")


  def generate_shared_library_hpp(self):
    out = open('../../../boost/extension/shared_library.hpp', mode='w')
    out.write("""/* (C) Copyright Jeremy Pack 2007
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_EXTENSION_LINKED_LIBRARY_HPP
#define BOOST_EXTENSION_LINKED_LIBRARY_HPP
#include <boost/extension/impl/library_impl.hpp>


namespace boost{namespace extensions{
""")
    for i in range(self.max_params, -1, -1):
      out.write(self.template_header('template <', 
                                     ['class ReturnValue'], 
                                     i, i == self.max_params))
      if  i != self.max_params:
        functor_template = ("".join(["<",
                            self.nameless_param_list(i, ["ReturnValue"]),
                            ">\n"]))
      else:
        functor_template = "\n"
      out.write("".join(["class functor",
        functor_template,
        """{
protected:
  typedef ReturnValue (*FunctionType)(""",
        self.nameless_param_list(i, []),
        """);
  FunctionType func_;
public:
  bool is_valid(){return func_ != 0;}
  functor(FunctionType func)
    :func_(func)
  {}
  functor(generic_function_ptr func)
    :func_(FunctionType(func))
  {}
  ReturnValue operator()(""",
          self.named_param_list(i, []),
          """)
  {
    return func_(""",
          self.param_names(i, []),
          """);
  }
};

"""]))
    # end for loop
    out.write("""class shared_library
{
protected:
  std::string location_;
  library_handle handle_;
  bool auto_close_;
public:
  bool is_open(){return handle_ != 0;}
  static bool is_linkable_library(const char * file_name){return is_library(file_name);}
  bool open(){return (handle_ = load_shared_library(location_.c_str())) != 0;}
  bool close(){return close_shared_library(handle_);}""")
    for i in range(0, self.max_params + 1):
      out.write(self.template_header('  template <', 
                                     ['class ReturnValue'], 
                                     i, False))
      functor_template = ("".join(["functor<",
                          self.nameless_param_list(i, ["ReturnValue"]),
                          ">\n"]))
      out.write("".join(["  ",
        functor_template,
        "    get_functor(",
        "const char * function_name",
        """)
  {
      return """,
        functor_template,
        """        (get_function(handle_, function_name));
  }
"""]))
    # end for loop
    out.write("""shared_library(const char * location, bool auto_close = false)
    :location_(location), handle_(0), auto_close_(auto_close){}
};
}}


#endif
""")
     
  def generate_factory_map_hpp(self, factory_type, factory_type_add, new_members,
                               generic_factory_container_additions = "",
                               factory_container_contents = "", new_public_members = "",
                               factory_container_params = ""):
    out = open(''.join(['../../../boost/extension/',factory_type,'_map.hpp']), mode='w')
    out.write(''.join(["""/* (C) Copyright Jeremy Pack 2007
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_EXTENSION_""",factory_type.upper(),"""_MAP_HPP
#define BOOST_EXTENSION_""",factory_type.upper(),"""_MAP_HPP
#include <list>
#include <memory>
#include <map>
#include <boost/extension/""",factory_type,""".hpp>
#include <boost/extension/extension.hpp>
#include  <boost/extension/impl/typeinfo.hpp>

namespace boost{namespace extensions{


template <class TypeInfo>
class basic_""",factory_type,"""_map
{
protected:
  class generic_factory_container
  {
  public:""", generic_factory_container_additions, """
    virtual ~generic_factory_container(){}
  };
  template <class Interface, class Info, class Param1 = void, class Param2 = void, class Param3 = void, class Param4 = void, class Param5 = void, class Param6 = void>
  class factory_container : public std::list<""",factory_type,"""<Interface, Info, Param1, Param2, Param3, Param4, Param5, Param6> >, public generic_factory_container
  {
  public:""", factory_container_contents, """
   // factory_container() {}
   // factory_container(basic_""", factory_type, """_map & z)
    //  :std::list<""",factory_type,"""<Interface, Info, Param1, Param2, Param3, Param4, Param5, Param6> >(z.get<Interface, Param1, Param2, Param3, Param4, Param5, Param6>()){}
    //virtual ~factory_container(){}
  };
  typedef std::map<TypeInfo, generic_factory_container *> FactoryMap;
  FactoryMap factories_;""", new_members,""" 
public:""", new_public_members,"""
  ~basic_""", factory_type, """_map(){
    for(typename FactoryMap::iterator it = factories_.begin(); it != factories_.end(); ++it)
      delete it->second;
    //TODO - test for memory leaks.
  }
"""]))
    for i in range(0, self.max_params + 1):
      out.write("".join([self.template_header('  template <', 
                                     ['class Interface', 'class Info'], 
                                     i, False),
          "operator std::list<",factory_type,"<",
          self.nameless_param_list(i, ["Interface", "Info"]),
          """> > & ()
  {return this->get<""",
          self.nameless_param_list(i, ["Interface", "Info"]),
          """>();}
""",
          self.template_header('  template <', 
                                     ['class Interface', 'class Info'], 
                                     i, False),
          "std::list<", factory_type, "<",
          self.nameless_param_list(i, ["Interface", "Info"]),
          """> > & get()
  {
      TypeInfo current_type = 
         type_info_handler<TypeInfo, """, factory_type, """<""",
           self.nameless_param_list(i, ["Interface", "Info"]),
           """> >::get_class_type();
      typename FactoryMap::iterator it = 
        factories_.find(current_type);
      
      if (it == factories_.end())
      {
        factory_container<""",
           self.nameless_param_list(i, ["Interface", "Info"]),
           """> * ret = 
          new factory_container<""",
           self.nameless_param_list(i, ["Interface", "Info"]),
           """>(""",factory_container_params,""");
        factories_[current_type] = ret;
        return *ret;
      }
      else
      {
        // Change to dynamic if this fails
        return static_cast<factory_container<""",
           self.nameless_param_list(i, ["Interface", "Info"]),
           """> &>(*(it->second));
      }
  }
""",
           self.template_header('template <', 
                          ['class Actual', 'class Interface', 'class Info'], 
                          i, False),
           """void add(Info info)
  {
    typedef std::list<""",factory_type,"""<""",
            self.nameless_param_list(i, ["Interface", "Info"]),
            """> > ListType;
    ListType & s = this->get<""",
            self.nameless_param_list(i, ["Interface", "Info"]),
            """>();
    """,factory_type,"""<""",
            self.nameless_param_list(i, ["Interface", "Info"]),
            """> f(info);""",factory_type_add,"""
    //f.set_type<Actual>();
    f.set_type_special((Actual*)0);
    s.push_back(f);
    //it->set_type<Actual>(); 
  }
"""]))
    # end for loop
    out.write(''.join(["""};

typedef basic_""",factory_type,"""_map<default_type_info> """,factory_type,"""_map;
}}

#endif
"""]))
    
    
def main(argv):
  if len(argv) > 1:
    max_params = int(argv[1])
  else:
    max_params = 6
  gen = header_generator(max_params)
  gen.generate()

if __name__=='__main__':
  main(sys.argv)
