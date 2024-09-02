Parameters in Ikaros 3
======================

### Types of parameters

A parameter can have one of five types. 

- __number__ represents a numerical value, a double or integer.
-  __rate__ is a magnitude per second that will scale with the tick_duration used. The value is interpreted as v per second. 
- __bool__ represents a boolean value.
- __string__ is a sequence of character
- __matrix__ is a an array or two-dimansional matrix.


## Constraining values with the option attribute

The number of posible values of a parameter can be constained by setting the permissble options. For exampole, if the options are A, B and C, the value of the parameter can be 0, 1, or 2 if the parameter is a number or "A", "B" or "C", if the parameter is a string. Options cannot be used for matrix value. For a bool value there should be exactly two options, the first of which is mapped to false and the seond is mapped to true.

## Selecting control for the WebUI with the control attribute

A paramater can be changed using different controls in the inspector of the WebUI. The control can be set with the control attribute. The alternatives are:


- textedit
- checkbox
- menu
- slider

Declaring parameters in IKC-files
----------------------------------

A parameter is declared in an element in the main class element of the file. A minimal parameter declaration should include the  attributes _name_, _type_ and _description_. The description attribute is used in various places for documentation.

```xml
<class>
    <parameter 
        name="myparam" 
        type="number" 
        description="a numerical parameter" 
    />
</class>
```



There can also be  attributes to set options or control for the WebUI. In the example below the numerical value will be constrained to be 0, 1 or 2 because there are three options and these will show up as a menu in the inspector in the webui.

```xml
<class>
    <parameter 
        name="myparam" 
        type="number" 
        description="a numerical parameter"
        options="aaa,bbb,ccc"
        control="menu"
    />
</class>
```
In addition to these standard attributes, it is also possible to declare other attributes that are to be used by a module or the webui. These are ignored by the kernel.


Declaring parameters in IKG-files
----------------------------------

Parameters can also be defined in a group in the same was as in a module class.


```xml
<group>
    <parameter 
        name="myparam" 
        type="number" 
        description="a numerical parameter" 
    />
</group>
```

Using parameters in modules
----------------------------------

Binding a prameter to a name

```C++
    parameter p;
    Bind(p,"PARAM_NAME);
```

Getting the value of a parameter

```C++
    double x = p;
    std::string s = p;
    bool b = p;
```

Setting the value of a parameter

```C++
    p = 12;
    p = 2.3;
    p = "12";
    p = "abc";
```

Printing a parameter

```C++
    p.print();
    p.orint("param");
```
