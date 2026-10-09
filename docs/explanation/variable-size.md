Variable size types in rbe:

Till now types in rbe have been fixed size. However it's really convinient to have variable size types. A type is considered variable size if its size is not known at compile time. These are different from extended headers/payloads which size (sizeof(T)) is known at compile time but its length (wire value) comes from a field in the header.
A type's size is not known at compile time if:

- It contains a repetition group or another variable size type such as strings.
- It contains another user defined variable size type.

A repeating group is nothing else than a range of elements that are repeated count times with a stride between them. The count always need to be specified by the wire in a previous field. The stride generally is equal to the size of the element but can be user/field specified to be greater. Both can be specified by the user using annotations:

- \[\[=rbe::count()\]\]: the count is obtained from the value of field\_name. Must be a field that is serialized before the repetition group.
- \[\[=rbe::stride("field\_name")\]\]: the stride is obtained from the value of field\_name. Must be a field that is serialized before the repetition group. The stride of the wire must be greater or equal to the size of the element, otherwise the behavior by the library is undefined.
- \[\[=rbe::stride(number)\]\]: the stride is constant value. If the stride is constant it will be evaluated at compile time that wire\_size\_of(element) >= stride.

Any C++ type that suits the following requirements should be feasable to be stored as a repeating group:

- It must be a sized contiguous range. (actually should we allow more kind of ranges does it make sense from a semantic persperctive to serialize std::list, and so on as repeating groups? Maps, sets, unordered\_maps, unordered\_sets, etc?
- It must be dynamic (e.g. std::vector, std::inplace\_vector, std::list, etc).
- Non-owning ranges can be used as well, however some aditional rules need to be followed:
   NOTE: (I'll consider providing a type that can hold a view over a range if its constructed by left value and owning if constructed by right value or initilizer list?)
  - During serialization any non-owning range can be used without any restrictions.
  - During eager deserialization only non-owning ranges make sense.

Serialization:
When serializing repeating groups we need to decide which behavior the library should follow. There are two options:

- Serializing count and stride is user responsibility.
- Serializing count and stride is library responsibility.
In one hand puting count and stride in the user side gives more flexibility to the user, but on the other hand it can be a source of bugs if count and stride are not correctly set.
In the other hand, if the library handles it for the user we lose the flexibility but we gain safety. Under this approach what happens if the user sets manually the count and stride?Do we override it or do we throw an error?

struct Hola {
  int num{251525};
  \[\[=rbe::count("num")]] int* vec;
};

->

struct Hola2 {
   std::vector<int> vec;
};

auto view = deserialize<Hola2>(buffer, lazy);
std::vector<int> vec = view.field<"vec">();

->

struct Hola2 {
  rbe::dynamic\_group<int, std::uint8_t> vec;
  rbe::static\_group<int, 50> array;
};

auto view = deserialize<Hola2>(buffer);
rbe::group<int, std::uint8_t> vec = view.field<"vec">(); // this is a std::span rather than a std::vector

After discussing with the team we decided to change totally the direction of the count annotation. Ranges don't need to be annotated with count we can simply use the size of the range and always serialize the size of the range just before the elements. The problem with this approach is: how can we differentiate between an static sized array and a dynamic sized array in C++? Maybe we can use an annotation
to distinguish between them.

Okey differentiating between static and dynamic sized arrays is possible check <https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3928r0.html>. It's not yet in the standard library however its library code and implementation is trivial. So problem solved.

Dynamic sized arrays are serialized with their size just before the elements and static sized arrays are serialized without their size.

However this leads me to a different problem what if the size of the dynamic sized range in an original protocol is not just before? Some protocols might define its count somewhere else in the structure. We need to allow the user to handle those cases but for ranges by default behavior is what we've said. Maybe we can keep count annotation to explicitly tell where the count is located in the structure? Or maybe we should use other mechanism? Having pointers seemed an option to me, however they cannot be used to deserialize.
