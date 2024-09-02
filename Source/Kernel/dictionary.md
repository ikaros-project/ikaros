Dictionary
==============

The class **dictionary** defines a flexible dictionary-like class in C++ capable of storing various types of values and providing JSON serialization and deserialization functionalities. It supports basic operations such as adding, retrieving, and merging key-value pairs and converting the dictionary to string and JSON formats. It also supports reading and saving the dictionary as XML.

The data structures use shared pointers to let different dictionaries and lists share the same data.

The dictionary class is used extensively in ikaros 3 kernel to hold all data that is not time critical. All modules have a member names info_ that contains a dictionary with data that describes that module. Altough this is not usually necessary, a module can acces this dictionary for special functionality.

## Main classes

The main classes are **dictionary**, **list** and **value**.

### dictionary

The dictionary class is a data structure that stores key-value pairs, where keys are strings and values are of type value. It provides methods for manipulating and accessing these pairs, merging dictionaries, and serializing the dictionary to a string or JSON format.

### list

The **list** class represents a sequence of value objects, providing methods for element access, deep copying, and serialization. This class complements the dictionary class by allowing the storage of ordered collections of values. It is similar to a dynamic array or a standard library vector in C++.

### value

The value class encapsulates different data types to be stored and managed uniformly within the dictionary. The class is designed to hold various types of data that can be stored in the dictionary. It acts as a wrapper for different data types and provides mechanisms for type conversions, comparisons, and serialization.

## Basic Examples

This section illustrates the basic function of a dictionary and how it is used.

```C++
    dictionary d;

    d["a"] = "AAA"; // string value
    d["b"] = true;  // bool value
    d["c"] = 3.14;  // double value
    d["d"] = 32;    // double value

    d.print();

   list l;

    l[0] = "First";
    l[1] = 2;
    l[5] = 5; // list will be padded with null values to hold 6 elements
    l.push_back(6); // add value to the back of list

    l.print();

    dictionary e;

    e["x"] = "XXX";
    e["y"] = d;     // dictionary value
    e["z"] = l;     // list value

    e.print();
```

## Shared data

The data is shared when one dictionary or list is assigned to another.

```C++
    dictionary d;
    dictionary e;

    d["a"] = "AAA";
    d["b"] = true;


    e["c"] = 3.14;
    e["d"] = d;

    d["b"] = false; // Changes value of e["d"]["b"] as well.

    e.print();      // {"c": 3.14, "d": {"a": "AAA", "b": false}}
 
```

When the data should not be shared, the copy-function can be used. It makes a deep copy of the data stucture for the dictionary or list.


```C++
    dictionary d;
    dictionary e;

    d["a"] = "AAA";
    d["b"] = true;


    e["c"] = 3.14;
    e["d"] = d.copy();

    d["b"] = false; // Does NOT change the value of e["d"]["b"].

    e.print();      // {"c": 3.14, "d": {"a": "AAA", "b": true}}
 
```

## Iteration

Iterating over key and value.

```C++

      for (const auto& [key, val] : d)
        std::cout << "Key: " << key << ", Value: " << val << std::endl;
```

Iterating over items in a list

```C++
      for (const auto& val : l)
        std::cout << "Value: " << val << std::endl;
```

## Serialization

A string containing JSON can be parsed using the function parse_json.


```C++
     

    std::string s =  R"({"name": "John Doe", "age": 30, "is_student": false, "scores": [85.5, 90.2, 78], "address": {"city": "New York", "zip": "10001"}})";
  
    dictionary d = parse_json(s);
    d.print();

```

The json-function can be use to generate a string with the json-representation of the dictionary or list.

```C++
    std::string s = d.json()
    std::cout << s << std::endl;
```
## More

```C++
    d.contains("x");
```