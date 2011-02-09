/*
 * Boost.Extension / multilanguage hello world implementations
 *
 * (C) Copyright Mariano G. Consoni 2007
 * Distributed under the Boost Software License, Version 1.0. (See             
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/ for latest version.
 */
#include <boost/extension/extension.hpp>
#include "word_description.hpp"
#include <boost/extension/factory_map.hpp>


class world : public word
{
public:
  virtual const char * get_val(){return "world!";}
};

class mundo : public word
{
public:
  virtual const char * get_val(){return "mundo!";}
};

class monde : public word
{
public:
  virtual const char * get_val(){return "monde!";}
};

class mondo : public word
{
public:
  virtual const char * get_val(){return "mondo!";}
};


class hello : public word
{
public:
  virtual const char * get_val(){return "hello";}
};

class hola : public word
{
public:
  virtual const char * get_val(){return "hola";}
};

class bonjour : public word
{
public:
  virtual const char * get_val(){return "bonjour";}
};

class buonasera : public word
{
public:
  virtual const char * get_val(){return "buonasera";}
};


extern "C" void BOOST_EXTENSION_EXPORT_DECL 
extension_export_multilanguage_word(boost::extensions::factory_map & fm)
{
  fm.get<word, word_description>()[word_description("spanish", "hello")]
    .set<hola>();
  fm.get<word, word_description>()[word_description("spanish", "world!")]
    .set<mundo>();

  fm.get<word, word_description>()[word_description("french", "hello")]
    .set<bonjour>();
  fm.get<word, word_description>()[word_description("french", "world!")]
    .set<monde>();
  fm.get<word, word_description>()[word_description("italian", "hello")]
    .set<buonasera>();
  fm.get<word, word_description>()[word_description("italian", "world!")]
    .set<mondo>();
  fm.get<word, word_description>()[word_description("english", "hello")]
    .set<hello>();
  fm.get<word, word_description>()[word_description("english", "world!")]
    .set<world>();
}
