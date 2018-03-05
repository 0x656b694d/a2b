# Model translation utility

The objective of the utility is to simplify the transformation of one object model to another.
A model represents a tree of dependent objects. A single object in the source model can be transformed to a branch of objects in destination model.
The utility takes care of the transformation order and fills the destination model tree according with the transformed objects.
Examples of such transformations are: BOM to JSON model, or a set of CSV tables to BOM, BOM to ORM etc.

The usage consists of these steps:

1. You define the destination model;
1. You define the translation function for your objects;
1. You apply the transformation;
1. You visit the objects in the destination module.

The utility needs to know the destination model and how to translate an object of your source model.

To define a model declare a `boost::mpl::list` like this:

```c++
typedef boost::mpl::list<Object1, Object2> ModelB;
```

You need also to tell how to translate your Object of some kind to the Model B:

```c++
class Translate2B: public moto::Translator<Translate2B, ModelB> {
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

