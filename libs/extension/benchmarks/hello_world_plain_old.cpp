/*
 * Boost.Extension / hello_world_plain_old benchmark
 *         hello world plugin implemented in dl* style
 *
 * (C) Copyright Mariano G. Consoni 2007
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */

#include "../examples/word.hpp"

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#  define EXPORT_DECL __declspec(dllexport)
#else
#  define EXPORT_DECL
#endif


class world : public word
{
public:
  virtual const char * get_val(){return "world!";}
};
class hello : public word
{
public:
  virtual const char * get_val(){return "hello";}
};

extern "C" void EXPORT_DECL extension_export_words(word **h, word **w)
{
  *h = new hello;
  *w = new world;
}
