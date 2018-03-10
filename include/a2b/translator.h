/*
 * Model Translation Utility
 *
 * https://github.com/0x656b694d/a2b
 *
 */
#include <list>
#include <tuple>

#include <boost/mpl/at.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/list.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/transform.hpp>

namespace a2b {

  namespace internal {
    /*
     * Transforming boost::mpl to std::tuple
     */
    template <typename SEQ, size_t N>
    struct vector_at : boost::mpl::at_c<SEQ, N>
    {};

    template <typename... TYPES, size_t N>
    struct vector_at<std::tuple<TYPES...>, N>
    : std::tuple_element<N, std::tuple<TYPES...>>
    {};

    template <typename SEQ, size_t N, typename... ARGS>
    struct convert_helper : convert_helper<SEQ, N-1, typename vector_at<SEQ, N-1>::type, ARGS...>
    {};

    template <typename SEQ, typename... ARGS>
    struct convert_helper<SEQ, 0, ARGS...> {
      typedef std::tuple<ARGS...> type;
    };

    template <typename SEQ>
    using Tuplified = convert_helper<SEQ, boost::mpl::size<SEQ>::value>;

    /*
    * Transforming a type list to a type list of containers of the internal types,
    * i.e. boost::mpl::list<T> -> boost::mpl::list<std::list<T>>
    */
    template<typename T, template<class...> class C>
    using Listified = typename boost::mpl::transform<T, C<boost::mpl::_1>>::type;

    template<typename F, typename Tuple, std::size_t N>
    class TupleVisitor {
    public:
      explicit TupleVisitor(F& f): _func(f) {}
      void accept(Tuple const& t)
      {
        TupleVisitor<F, Tuple, N-1>(_func).accept(t);
        _func(std::get<N-1>(t));
      }
      void reverse_accept(Tuple const& t)
      {
        _func(std::get<N-1>(t));
        TupleVisitor<F, Tuple, N-1>(_func).reverse_accept(t);
      }
    private:
      F &_func;
    };

    template<typename F, typename Tuple>
    class TupleVisitor<F, Tuple, 1> {
    public:
      explicit TupleVisitor(F& f): _func(f) {}
      void accept(Tuple const& t) {
        _func(std::get<0>(t));
      }
      void reverse_accept(Tuple const& t) {
        _func(std::get<0>(t));
      }
    private:
      F &_func;
    };

    template<typename F, template<class...> class C>
    class Applicator {
    public:
      explicit Applicator(F& f): _func(f)
      {}

      template<typename D>
      void operator()(C<D> const& x) const {
        for (D const& d: x) {
          _func(d);
        }
      }

      template<typename D>
      void operator()(C<D>& x) const {
        for (D& d: x) {
          _func(d);
        }
      }

    private:
      F &_func;
    };

    template<template<class...> class C, typename F, typename... Args>
    F& visit(std::tuple<Args...> const& t, F& f)
    {
      using tmp = TupleVisitor<Applicator<F, C>, decltype(t), sizeof...(Args)>;
      Applicator<F, C> a(f);
      tmp(a).accept(t);
      return f;
    }

    template<template<class...> class C, typename F, typename... Args>
    F& visit(std::tuple<Args...>& t, F& f)
    {
      using tmp = TupleVisitor<Applicator<F, C>, decltype(t), sizeof...(Args)>;
      Applicator<F, C> a(f);
      tmp(a).accept(t);
      return f;
    }

    template<template<class...> class C, typename F, typename... Args>
    F& reverse_visit(std::tuple<Args...> const& t, F& f)
    {
      using tmp = TupleVisitor<Applicator<F, C>, decltype(t), sizeof...(Args)>;
      Applicator<F, C> a(f);
      tmp(a).reverse_accept(t);
      return f;
    }

    template<template<class...> class C, typename F, typename... Args>
    F& reverse_visit(std::tuple<Args...>& t, F& f)
    {
      using tmp = TupleVisitor<Applicator<F, C>, decltype(t), sizeof...(Args)>;
      Applicator<F, C> a(f);
      tmp(a).reverse_accept(t);
      return f;
    }

  } // internal

  template<typename ListifiedModel, template<class...> class C>
  class Instance {
  public:
    using listified_type = ListifiedModel;
    using value_type = typename internal::Tuplified<listified_type>::type;

    template<typename ...T>
    using container_type = C<T...>;

    template<typename T>
    C<T>& get() {
      return std::get<Index<T>::value>(_value);
    }

    template<typename T>
    C<T> const& get() const {
      return std::get<Index<T>::value>(_value);
    }

    template<typename T>
    value_type& add(T const& value) {
      std::get<Index<T>::value>(_value).push_back(value);
      return _value;
    }

    value_type const& getValue() const { return _value; }
    value_type& getValue() { return _value; }

    template<typename F>
    void visit(F& f) {
      internal::visit<container_type>(_value, f);
    }
    template<typename F>
    void visit(F& f) const {
      internal::visit<container_type>(_value, f);
    }
    template<typename F>
    void reverse_visit(F& f) {
      internal::reverse_visit<container_type>(_value, f);
    }
    template<typename F>
    void reverse_visit(F& f) const {
      internal::reverse_visit<container_type>(_value, f);
    }
  private:

    template<typename T>
    struct Index {
      static size_t const value = boost::mpl::distance<
        typename boost::mpl::begin<listified_type>::type,
        typename boost::mpl::find<listified_type, C<T>>::type>::value;
    };

    value_type _value;
  };

  template<typename UserTranslator, typename Model,
           template<class...> class C = std::list,
           typename V = Instance<internal::Listified<Model, C>, C>>
  class Translator {
  public:
    using value_type = V;
    using result_type = V&;
    using model_type = Model;
    using base_type = Translator<UserTranslator, Model, C, V>;
    template<typename ...T>
    using container_type = C<T...>;

    // Generic translators
    template<typename T>
    result_type translate(T const&) const {
      throw std::runtime_error(std::string("Missing translation for ") + typeid(T).name());
    }
    
    template<template<class...> class CC, typename T>
    value_type translate(CC<T> const& sequence) && {
      return translate_sequence(sequence);
    }

    template<template<class...> class CC, typename T>
    result_type translate(CC<T> const& sequence) & {
      return translate_sequence(sequence);
    }
    // end of generic translators

    template<typename T>
    result_type add(T const& obj) {
      _result.add(obj);
      return _result;
    }

    value_type const& getResult() const {
      return _result;
    }

    value_type& getResult() {
      return _result;
    }

  protected:
    Translator() = default;

  private:
    template<typename T>
    result_type translate_sequence(T const& sequence) {
      for (auto&& obj: sequence) {
        static_cast<UserTranslator*>(this)->translate(obj);
      }
      return _result;
    }

    value_type _result;

  }; // class Translator

  template<typename I, typename F>
  void visit(I const& result, F& f) {
    internal::visit<I::container_type>(result.getValue(), f);
  }

  template<typename I, typename F>
  void visit(I& result, F& f) {
    internal::visit<I::template container_type>(result.getValue(), f);
  }

  template<typename I, typename F>
  void reverse_visit(I const& result, F& f) {
    internal::reverse_visit<I::container_type>(result.getValue(), f);
  }

  template<typename I, typename F>
  void reverse_visit(I& result, F& f) {
    internal::reverse_visit<I::template container_type>(result.getValue(), f);
  }

  struct Visitor {};

  template<typename... Args>
  using model = boost::mpl::list<Args...>;

} // namespace a2b
