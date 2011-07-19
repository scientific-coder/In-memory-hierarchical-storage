#include <string>
#include <unordered_map>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <typeinfo>
#include "boost/variant.hpp"
#include "boost/variant/recursive_variant.hpp"
#include "boost/token_iterator.hpp"

typedef boost::make_recursive_variant<
  std::unordered_map< std::string, boost::recursive_variant_ >
  , int
  , float
  , std::string
  >::type storage_type;
typedef std::unordered_map<std::string, storage_type> kbase_impl_type;


template<typename T, typename It> T const& get_rec( storage_type const& root, It b, It e)
{
  if (b == e) { return boost::get<T>(root); }
  kbase_impl_type const& ref(boost::get<kbase_impl_type>(root));
  typename kbase_impl_type::const_iterator it(ref.find(*b));
  if (it == ref.end()) {
    std::ostringstream os;
    os<<" key: ["<< *b << "] not found !";
    throw std::invalid_argument(os.str());
  }
  ++b;
  return (b == e)
    ? boost::get<T>((*it).second)
    : get_rec<T>(boost::get<kbase_impl_type>((*it).second), b, e);
}

// fully qualified names access
// :TODO: implement set()
template<typename T> T const& get( storage_type const& root, std::string const& fully_qualified) {
  static boost::char_separator<char> const sep(".");

  boost::token_iterator_generator< boost::char_separator<char> >::type b(boost::make_token_iterator<std::string>(fully_qualified.begin(), fully_qualified.end(),sep)), e;
  return get_rec<T>(root, b, e);
}
// rw for read_write acces in ...
// :TODO: const read_only accessor and dispatching overloads
struct rw_in {
  boost::variant<storage_type*, kbase_impl_type*> data;
  rw_in(kbase_impl_type& kb) : data(&kb) {}
  rw_in(storage_type& s) : data(&s) {}
  struct kbase_getter : boost::static_visitor<kbase_impl_type&> {
    kbase_impl_type& operator()(kbase_impl_type * v) const { return *v;}
    kbase_impl_type& operator()(storage_type* v) const { return boost::get<kbase_impl_type>(*v);}
  };
  rw_in operator[](std::string const& name) const
  {  return rw_in(boost::apply_visitor(kbase_getter(), data)[name]); }
  template<typename T> operator T&() const 
  { return boost::get<T&>(*boost::get<storage_type*>(data));}

  template<typename T> T const& operator=(T const& v) const 
  { return boost::get<T const&>((*boost::get<storage_type*>(data))= v);}  
};

namespace boost {
  template<typename T> T& get( rw_in const& in){ return in.template operator T&();}
}

int main (int argc, char* argv[]){
  
  kbase_impl_type knowledge_base;

  // direct acces to the base  
  knowledge_base["hei!"]=1;
  knowledge_base["pupu"]=1.5f;
  knowledge_base["c++"]="clojure";
  
  
  std::cout<< boost::get<int>(knowledge_base["hei!"]) << std::endl;
  std::cout<< boost::get<float>(knowledge_base["pupu"]) << std::endl;
  std::cout<< boost::get<std::string>(knowledge_base["c++"]) << std::endl;

  // gets cumbersome for inner maps !
  boost::get<kbase_impl_type>(knowledge_base["test"])["hierarchical"]= "Win!";
  std::cout<<boost::get<std::string>(boost::get<kbase_impl_type>(knowledge_base["test"])["hierarchical"]) <<std::endl;
  // a fully qualified name accessor
  std::cout<< get<std::string>(knowledge_base,"test.hierarchical") <<std::endl;
  
  rw_in in(knowledge_base); // enable some syntaxic sugar
  in ["test"]["hierarchical"]= std::string("sugar!"); // could add char const* in the variant
  std::cout<< get<std::string>(knowledge_base,"test.hierarchical") <<std::endl;
  int i= in["hei!"];
  std::cout<< i <<std::endl;
  float f= in["pupu"];
  std::cout<< f <<std::endl;
  std::string & s(in["test"]["hierarchical"]);
  std::cout<< s <<std::endl;
  in["test"]["hi"]=2;
  std::cerr<<"setting done\n";
  i=(in["test"])["hi"];
  std::cout<< i <<std::endl;
  return 0;
}
