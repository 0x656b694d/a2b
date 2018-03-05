#include <gtest/gtest.h>
#include <a2b/translator.h>

#include <sstream>

namespace A {

  struct Person {
    std::string name;
    int age;
  };

  struct Room {
    int number;
  };

  struct Team {
    std::string name;
    std::vector<Person> people;
  };

} // namespace A

namespace B {

  struct Person {
    int team_id;
    std::string name;
    int age;
  };

  struct Chambre {
    int number;
  };

  struct Equipe {
    int id;
    std::string name;
  };

  typedef boost::mpl::list<Person, Equipe, Chambre> Schema;
} // namespace B

struct A2B: a2b::Translator<A2B, B::Schema> {

  using base_type::translate;

  bool translate(A::Person const& ap, int const team_id) {
    return add(B::Person{ team_id, ap.name, ap.age });
  }

  bool translate(A::Person const& ap) {
    return translate(ap, 0);
  }

  bool translate(A::Team const& at) {
    int const team_id = 0;
    bool result = add(B::Equipe{ team_id, at.name });
    for (A::Person const& ap: at.people) {
        translate(ap, team_id);
    }
    return result;
  }

  bool translate(A::Room const& aroom) {
    return add(B::Chambre{ aroom.number });
  }

};
TEST(Translate, Errors) {
  A2B a2b;
  ASSERT_THROW(a2b::translate(a2b, 15), std::runtime_error);
  ASSERT_THROW(a2b::translate(a2b, std::string("abc")), std::runtime_error);
}

TEST(Translate, Person) {
  std::list<A::Person> const a = {{"name", 13}};
  auto result = A2B().translate(a);
  ASSERT_EQ(size_t(1), result.get<B::Person>().size());
  EXPECT_EQ(13, result.get<B::Person>().front().age);
}

TEST(Translate, Team) {
  std::vector<A::Team> const t = {{ "A",
    {{"Howling Mad Murdock", 13},
    {"B. A. Baracus", 14}}
  }};

  auto result = A2B().translate(t);
  ASSERT_EQ(size_t(1), result.get<B::Equipe>().size());
  EXPECT_EQ(std::string("A"), std::string("A"));
  EXPECT_EQ(std::string("A"), result.get<B::Equipe>().front().name);

  ASSERT_EQ(size_t(2), result.get<B::Person>().size());
  EXPECT_EQ(13, result.get<B::Person>().front().age);
  EXPECT_EQ(14, result.get<B::Person>().back().age);
}

class Printer {
public:
  template<typename T>
  void operator()(T const& obj) {
    _os << " | const " << typeid(T).name();
  }
  template<typename T>
  void operator()(T& obj) {
    _os << " | " << typeid(T).name();
  }
  void operator()(B::Person const& obj) {
    _os << " | const Person " << obj.name;
  }
  void operator()(B::Chambre const& obj) {
    _os << " | const Chambre " << obj.number;
  }
  void operator()(B::Person& obj) {
    obj.name = "overridden";
    _os << " | Person " << obj.name;
  }
  void operator()(B::Chambre& obj) {
    obj.number = 17;
    _os << " | Chambre " << obj.number;
  }
  std::string str() const {
    return _os.str();
  }
private:
  std::ostringstream _os;
};

TEST(Translate, Visit) {
  std::vector<A::Person> const ap = {{ "name", 13 }};
  std::vector<A::Room> const ar = {{ 42 }};
  A2B a2b;
  a2b.translate(ap);
  a2b.translate(ar);
  auto result = a2b.getResult();
  auto const& const_result = result;
  {
    Printer p;
    const_result.visit(p);
    EXPECT_EQ(std::string(" | const Person name | const Chambre 42"), p.str());
  }
  {
    Printer p;
    const_result.reverse_visit(p);
    EXPECT_EQ(std::string(" | const Chambre 42 | const Person name"), p.str());
  }
  {
    Printer p;
    a2b::visit(result, p);
    EXPECT_EQ(std::string(" | Person overridden | Chambre 17"), p.str());
  }
  {
    Printer p;
    a2b::reverse_visit(result, p);
    EXPECT_EQ(std::string(" | Chambre 17 | Person overridden"), p.str());
  }
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}

