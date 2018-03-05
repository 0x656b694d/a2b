/*
 * Model translation utility
 *
 * https://github.com/0x656b694d/a2b
 *
 */
#include <list>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/mpl/at.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/list.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/transform.hpp>

namespace a2b {

  namespace internal {
    /*
     * Transforming a boost::mpl type list to an std::tuple so we can
     * instanciate this type list
     */
    template <typename SEQ, size_t N> struct vector_at : boost::mpl::at_c<SEQ, N>
    {};

    template <typename... TYPES, size_t N> struct vector_at<std::tuple<TYPES...>, N>
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
    struct Tuplify: convert_helper<SEQ, boost::mpl::size<SEQ>::value>
    {};

    /*
    * Transforming a type list to a list of std::list of the internal types,
    * i.e. boost::mpl::list<A> -> boost::mpl::list<std::list<A>>
    */
    template<typename T>
    using Listified = typename boost::mpl::transform<T, std::list<boost::mpl::_1>>::type;

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

    template<typename F>
    class Applicator {
    public:
      explicit Applicator(F& f): _func(f)
      {}

      template<typename D>
      void operator()(std::list<D> const& x) const {
        for (D const& d: x) {
          _func(d);
        }
      }

      template<typename D>
      void operator()(std::list<D>& x) const {
        for (D& d: x) {
          _func(d);
        }
      }

    private:
      F &_func;
    };

    template<typename F, typename... Args>
    F& visit(std::tuple<Args...> const& t, F& f)
    {
      using tmp = TupleVisitor<Applicator<F>, decltype(t), sizeof...(Args)>;
      Applicator<F> a(f);
      tmp(a).accept(t);
      return f;
    }

    template<typename F, typename... Args>
    F& visit(std::tuple<Args...>& t, F& f)
    {
      using tmp = TupleVisitor<Applicator<F>, decltype(t), sizeof...(Args)>;
      Applicator<F> a(f);
      tmp(a).accept(t);
      return f;
    }

    template<typename F, typename... Args>
    F& reverse_visit(std::tuple<Args...> const& t, F& f)
    {
      using tmp = TupleVisitor<Applicator<F>, decltype(t), sizeof...(Args)>;
      Applicator<F> a(f);
      tmp(a).reverse_accept(t);
      return f;
    }

    template<typename F, typename... Args>
    F& reverse_visit(std::tuple<Args...>& t, F& f)
    {
      using tmp = TupleVisitor<Applicator<F>, decltype(t), sizeof...(Args)>;
      Applicator<F> a(f);
      tmp(a).reverse_accept(t);
      return f;
    }

  } // internal

  template<typename ListifiedModel>
  class Instance {
  public:
    using listified_type = ListifiedModel;
    using value_type = typename internal::Tuplify<listified_type>::type;

    template<typename T>
    std::list<T>& get() {
      return std::get<Index<T>::value>(_value);
    }

    template<typename T>
    std::list<T> const& get() const {
      return std::get<Index<T>::value>(_value);
    }

    template<typename T>
    bool add(T const& value) {
      std::get<Index<T>::value>(_value).push_back(value);
      return true;
    }

    value_type const& getValue() const { return _value; }
    value_type& getValue() { return _value; }

    template<typename F>
    void visit(F& f) {
      internal::visit(_value, f);
    }
    template<typename F>
    void visit(F& f) const {
      internal::visit(_value, f);
    }
    template<typename F>
    void reverse_visit(F& f) {
      internal::reverse_visit(_value, f);
    }
    template<typename F>
    void reverse_visit(F& f) const {
      internal::reverse_visit(_value, f);
    }
  private:

    template<typename T>
    struct Index {
      static size_t const value = boost::mpl::distance<
        typename boost::mpl::begin<listified_type>::type,
        typename boost::mpl::find<listified_type, std::list<T>>::type>::value;
    };

    value_type _value;
  };

  template<typename UserTranslator, typename Model, typename V = Instance<internal::Listified<Model>>>
  class Translator {
  public:
    using value_type = V;
    using model_type = Model;
    using base_type = Translator<UserTranslator, Model, V>;

    // Generic translators
    template<typename T>
    void translate(T const&) const {
      throw std::runtime_error(std::string("Missing translation for ") + typeid(T).name());
    }

    template<typename T>
    value_type translate(std::vector<T> const& sequence) && {
      return translate_sequence(sequence);
    }

    template<typename T>
    value_type& translate(std::vector<T> const& sequence) & {
      return translate_sequence(sequence);
    }

    template<typename T>
    value_type translate(std::list<T> const& sequence) && {
      return translate_sequence(sequence);
    }

    template<typename T>
    value_type& translate(std::list<T> const& sequence) & {
      return translate_sequence(sequence);
    }
    // end of generic translators

    template<typename T>
    bool add(T const& obj) {
      return _result.add(obj);
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
    value_type& translate_sequence(T const& sequence) {
      for (typename T::value_type const& obj: sequence) {
        static_cast<UserTranslator*>(this)->translate(obj);
      }
      return _result;
    }

    value_type _result;

    template<typename UT, typename T>
    friend void translate(UT&&, T const&);
  }; // class Translator

  template<typename UserTranslator, typename T>
  void translate(UserTranslator&& tr, T const& obj) {
    tr.translate(obj);
  }

  template<typename I, typename F>
  void visit(I const& result, F& f) {
    internal::visit(result.getValue(), f);
  }

  template<typename I, typename F>
  void visit(I& result, F& f) {
    internal::visit(result.getValue(), f);
  }

  template<typename I, typename F>
  void reverse_visit(I const& result, F& f) {
    internal::reverse_visit(result.getValue(), f);
  }

  template<typename I, typename F>
  void reverse_visit(I& result, F& f) {
    internal::reverse_visit(result.getValue(), f);
  }

  struct Visitor {};

} // namespace model
