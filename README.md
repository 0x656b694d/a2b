# Model translation utility

The objective of the utility is to simplify the transformation of one object model to another.
A model represents a list of objects. A single object in the source model can be transformed to a number of objects in destination model.
The utility generates a special structure that holds lists of objects of all the classes of the destination model,
provides a method to fill the destination model with the transformed objects and takes care of the order of visiting.
Examples of such transformations are: BOM to JSON model, or a set of CSV tables to BOM, BOM to ORM etc.

The usage consists of these steps:

1. You define the destination model as a list of the classes of the model in the order you want to visit them later;
1. You define the translation functions for your objects in a structure derived from `a2b::Translator`;
1. You apply the transformation by calling the `translate` method of the translator;
1. You visit the objects with a visitor or manually by inspecting the result of the translation.

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

