# Model Translation Utility

The objective of the utility is to simplify the translation of one object model to another.

A model represents a list of classes. A single object of a class in the source model can be translated to a number of objects of different classes in the destination model.

The utility generates in compile time a special structure, which is `std::tuple<std::list<T1>, std::list<T2>, ...>`, where each `std::list<Tn>` holds a number of objects of the destination model. In your translation operator you call the provided `add()` method which fills the appropriate list. After the translation is complete (= the structure is filled), you may visit all the generated objects in the order of types that you specified in the model definition. You define the model as a `boost::mpl::list<T1, T2, ...>`.

Examples of such transformations are: BOM to API, BOM to ORM etc. One may need to support several versions of such transformations.

# Usage

The usage consists of these steps:

1. Define the destination model as a list of the classes of the model in the order you want to visit them later;
1. Define the translation functions for your objects in a structure derived from `a2b::Translator`;
1. Apply the transformation by calling the `translate` method of the translator;
1. Visit the objects with a visitor or manually by inspecting the result of the translation.

To define a model declare a `boost::mpl::list` like this:

```c++
typedef boost::mpl::list<Object1, Object2> ModelB;
```

You need also to tell how to translate your Object of some class to the Model B. Method `add` fills the destination list with the 

```c++
class Translate2B: public a2b::Translator<Translate2B, ModelB> {
public:
  bool translate(Object const& obj) {
    return add(Object1{ obj.x, obj.y });
  }
};
```

The `translate` method should return `true` if it has translated everything or `false`
if the framework should try to continue the translation recursively.

Now use your translator class to do the job:

```c++
std::vector<Object> const objs = { ... };
auto result = Translate2B().translate(objs);
```

The result will then consist of the objects added during the translation:

```c++
std::list<Object1>& o1 = result.get<Object1>();
```

To visit all the objects of all the types use the `visit` method of the result:

```c++
class MyVisitor {
public:
  template<typename T> void operator()(T const& obj) {
    print(obj);
  }
};

result.visit(MyVisitor());
```

Use `reverse_visit` to visit in the reverse order.

# Run tests

```sh
cmake CMakeLists.txt
make
./runTests

```

# Example
https://github.com/0x656b694d/a2b/blob/master/test/uttranslator.cpp
