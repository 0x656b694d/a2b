#include <gtest/gtest.h>
#include <a2b/translator.h>

#include <sstream>

/*
 * We're going to convert objects from namespace A to the model of namespace B
 */

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

  struct Personne {
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

  // Definition of the model of the namespace B
  typedef boost::mpl::list<Personne, Equipe, Chambre> Model;
} // namespace B

// The list of translations for each object of namespace A to lists of objects of namespace B
class A2B: public a2b::Translator<A2B, B::Model> {
public:

  using base_type::translate;
 
  A2B(): team_id(0) {}

  result_type translate(A::Person const& ap) {
    return add(B::Personne{ team_id, ap.name, ap.age });
  }

  result_type translate(A::Team const& at) {
    add(B::Equipe{ team_id, at.name });
    result_type result = translate(at.people);
    ++team_id;
    return result;
  }

  result_type translate(A::Room const& aroom) {
    return add(B::Chambre{ aroom.number });
  }

private:
  int team_id;
};

TEST(Translate, Errors) {
  A2B tr;
  // The utility cannot translate random types
  ASSERT_THROW(tr.translate(15), std::runtime_error);
  ASSERT_THROW(tr.translate(std::string("abc")), std::runtime_error);
}

TEST(Translate, Person) {
  A2B tr;
  // Having a single Person
  A::Person const simpson = { "Homer", 39 };
  auto const& result = tr.translate(simpson);
  ASSERT_EQ(size_t(1), result.get<B::Personne>().size());
  // and having a list of Persons
  std::list<A::Person> const simpsons = {{"Lisa", 8}, {"Bart", 10}};
  tr.translate(simpsons);
  // ... we get a list of Personnes
  ASSERT_EQ(size_t(3), result.get<B::Personne>().size());
  EXPECT_EQ(39, result.get<B::Personne>().front().age);
  EXPECT_EQ("Lisa", (++result.get<B::Personne>().begin())->name);
  EXPECT_EQ(10, result.get<B::Personne>().back().age);
}

TEST(Translate, Team) {

  std::vector<A::Team> const t = {{ "A",
    {{"Howling Mad Murdock", 13},
    {"B. A. Baracus", 14}}
  }};

  auto result = A2B().translate(t);
  // From A team we get une Equipe
  ASSERT_EQ(size_t(1), result.get<B::Equipe>().size());
  EXPECT_EQ(std::string("A"), std::string("A"));
  EXPECT_EQ(std::string("A"), result.get<B::Equipe>().front().name);

  // ... and 2 Personnes
  ASSERT_EQ(size_t(2), result.get<B::Personne>().size());
  EXPECT_EQ(13, result.get<B::Personne>().front().age);
  EXPECT_EQ(14, result.get<B::Personne>().back().age);
}

// To execute an action on the resulting objects we need a visitor.
// This visitor prints the objects to a stream and modifies them if possible
class Printer: a2b::Visitor {
public:

  void operator()(B::Equipe const& obj) {
    _os << " | const Equipe " << obj.name;
  }
  void operator()(B::Personne const& obj) {
    _os << " | const Personne " << obj.name;
  }
  void operator()(B::Chambre const& obj) {
    _os << " | const Chambre " << obj.number;
  }

  // actions can modify values in the objects if needed
  void operator()(B::Personne& obj) {
    obj.name = "modified";
    _os << " | Personne " << obj.name;
  }
  void operator()(B::Chambre& obj) {
    obj.number = 17;
    _os << " | Chambre " << obj.number;
  }
  void operator()(B::Equipe& obj) {
    _os << " | Equipe " << obj.name;
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
  A2B tr;
  // First let's translate people
  tr.translate(ap);
  // ... then rooms
  tr.translate(ar);
  auto result = tr.getResult();
  auto const& const_result = result;

  // Applied on constant: no modifications are possible
  {
    Printer p;
    const_result.visit(p);
    EXPECT_EQ(std::string(" | const Personne name | const Chambre 42"), p.str());
  }
  {
    Printer p;
    const_result.reverse_visit(p);
    EXPECT_EQ(std::string(" | const Chambre 42 | const Personne name"), p.str());
  }

  // Applied on mutable: modifications are possible
  {
    Printer p;
    // Both syntaxes are valid: `result.visit(p)` and `a2b::visit(result, p)`
    a2b::visit(result, p);
    EXPECT_EQ(std::string(" | Personne modified | Chambre 17"), p.str());
  }
  {
    Printer p;
    a2b::reverse_visit(result, p);
    EXPECT_EQ(std::string(" | Chambre 17 | Personne modified"), p.str());
  }
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}

