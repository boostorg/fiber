/* Boost.Extension / adaptable_factory example
 *
 * (C) Copyright Jeremy Pack 2008
 * Distributed under the Boost Software License, Version 1.0. (See             
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

#include <iostream>

#include <boost/extension/adaptable_factory.hpp>
#include <boost/reflection/parameter_map.hpp>
#include <boost/scoped_ptr.hpp>

using namespace boost::extensions;
using namespace boost::reflections;

// This function would need to use some of the facilities
// from Boost.TypeTraits in order to handle references, etc.
// well.
template <class T>
void InputParameter(parameter_map& params, const std::string& name) {
  T value;
  std::cin >> value;
  std::cout << "Input value: " << value << " of type: "
            << typeid(T).name() << std::endl;
  params.insert(std::make_pair(name, new parameter<T>(value)));  
}

class Operation {
 public:
  Operation(double result) : result(result) {
  }

  double result;
};

class Multiplier : public Operation {
 public:
  Multiplier(double first_value, double second_value)
    : Operation(first_value * second_value) {
  }
};

int main() {
  std::map<default_type_info,
           void (*)(parameter_map&, const std::string&)> input_functions;
  input_functions[typeid(float)] = &InputParameter<float>;
  input_functions[typeid(double)] = &InputParameter<double>;
  input_functions[typeid(std::string)] = &InputParameter<std::string>;

  adaptable_factory<Operation> op_factory;
  op_factory.set<Multiplier, double, double>("First Double", "Second Double");

  parameter_map params;

  std::vector<std::pair<default_type_info, std::string> > missing =
    op_factory.get_missing_params(params);

  for (std::vector<std::pair<default_type_info, std::string> >::iterator
         it = missing.begin();
       it != missing.end(); ++it) {
    std::cout << "Finding type to input..." << std::endl;
    std::map<default_type_info,
             void (*)(parameter_map&, const std::string&)>::iterator func =
      input_functions.find(it->first);
    if (func == input_functions.end()) {
      std::cout << "Could not find all needed parameters." << std::endl;
      return 1;
    }

    std::cout << "Please input a value for the variable <" << it->second
              << "> of type <" << it->first.type().name() << ">:" << std::endl;
    (*func->second)(params, it->second);
    std::cout << "Input complete for parameter." << std::endl;
  }
  boost::scoped_ptr<Operation> op(op_factory.create(params));
  if (op.get()) {
    std::cout << "The result is: " << op->result << std::endl;
  } else {
    std::cout << "Creation failed!" << std::endl;
  }
}
