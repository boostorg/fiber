/*
 * Boost.Extension / compilation example header:
 *         main header for compilations
 *
 * (C) Copyright Jeremy Pack 2008
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

#ifndef BOOST_EXTENSION_COMPILATION_HPP
#define BOOST_EXTENSION_COMPILATION_HPP

#include <vector>
#include <string>

#include <boost/extension/type_map.hpp>

namespace boost {
namespace extensions {
class compilation {
public:
  virtual bool run(const std::string& library_name,
                   const std::string& external_function_contents,
                   type_map& types,
                   const std::string& earlier_file_contents = "") const = 0;
  void add_header(const std::string& location) {
    headers_.push_back(location);
  }
  void vector_compilation_directory(const std::string& directory) {
    compilation_directory_ = directory;
  }
  void add_include_path(const std::string& location) {
    headers_.push_back(location);
  }
  void add_source(const std::string& location) {
    headers_.push_back(location);
  }
  void add_static_library(const std::string& name) {
    headers_.push_back(name);
  }
  compilation() {
    headers_.push_back("<boost/extension/extension.hpp>");
    headers_.push_back("<boost/extension/type_map.hpp>");
  }
  virtual ~compilation() {}
protected:
  std::vector<std::string> headers_;
  std::vector<std::string> include_path_;
  std::vector<std::string> sources_;
  std::vector<std::string> libraries_;
  std::string compilation_directory_;
};
}  // namespace extensions
}  // namespace boost
#endif  // BOOST_EXTENSION_COMPILATION_HPP
