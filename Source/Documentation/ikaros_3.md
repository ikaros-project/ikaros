Major Changes in Ikaros 3.0
============================

## WebUI

Widgets in views now need to be named widget insetad of its type. Instead the type is set by the class attributes (This little change made it possible to remove hundreds of lines of code in the kernel):

```xml
<image x="10" y="20" .... />
```

Should be changed to:

```xml
<widget class="image" x="10" y="20" .... />
```

## Connections

Connections now support arbitrary ranges and delays.

### Ranges in connections

```xml
<connection source="M.OUTPUT[4:7][2:5]" target="N.INPUT[6:9][0:4]" />
```

Target indices can be omitted but dimensionality is required using empty brackets. The ranges will be filled in auotmaatically, starting at 0 foe each dimension.

```xml
<connection source="M.OUTPUT[4:7][2:5]" target="N.INPUT[][]" />
```

Ranges can include a step value (2 in the example below which will copy every other value):

```xml
<connection source=M.OUTPUT[4:20:2] ... />"
```

A negative step gives exactly the same sequence as a positive one but backards. This can be used to reverse the order of the elements of an array or matrix. (Note that this looks a little like ranges in Python but the result of a negative step value is different).

```xml
<connection source=M.OUTPUT[4:20:-2] ... />"
```

Indices that are zero can be omitted in ranges. For example [:9] is the same as [0:9]. A completely empty range is filled in from the size of the source. A single number in a range denotes a single element, for example [5].

Many possible rearrangements of matrices are possible using ranges in connections depending on the source and target ranges. Reasonable default assumptions are made in most cases when the full ranges are not specified.

### Delays in connections

```xml
<connection source=M.OUTPUT target="N.INPUT" delay="3:7:2" />
```



Delays are specified as a range using the delay attribute. The actual range is described in the same way as ranges above.

When a single delay i required, it is possible to instead use the _last_() function directly on a matrix or input. See separate documentation for matrix operations.

### Aliases in connections

Connections can be assiged a name using the alias-attribute. This is useful wen the actual name of an input does not say enough of the data, for example in a print function or when exporting to file. An alias can also be used to name a particular dimension of a stacked input matrices (see below under Inputs).

```xml
<connection source=M.OUTPUT target="N.INPUT" alias="my_data" />
```

The alias is used to set the name of the matrix used as input or as labels for individual dimensions depending on context – the type of connection and input. One possible use is to name the individual dimensions of an image. Here the three two-dimensional outputs will be stacked in a single three-dimensional input matrix. and each of new dimensions will be labeled "RED", "GREEN" and "BLUE".

```xml
<connection source="M.R" target="N.INPUT" alias="RED" />
<connection source="M.G" target="N.INPUT" alias="GREEN" />
<connection source="M.B" target="N.INPUT" alias="BLUE" />
```

## Inputs

Inputs are represented by matrices in modules and are bound using the Bind() function.

```C++
matrix input;

Bind(input, "INPUT");
```

If an input is not connected it will be empty. This can be tested with the function empty() or the function connected():

```C++
if(input.empty())
{
    // Input is not connected
}

if(input.connected())
{
    // Input is connected
}
```

It is not always necessary to check if an input is connected as several functions such as apply() works also on empty matrices (See matrix operations).

- Inputs that receive a single connection has the same size of the connected output.

- If several outputs are connected to the same input, they are stacked in a high-dimensional matrix.

- When an input is declared in an ikc-file the flatten-attribute can be set to true to avoid this stacking behavior and instead put all inputs into a single one-dimensional matrix (array). In addition, if the use_alias-attribute is set, the dimension-labels of the input matix will be set (See the OutputFile module for examples).

```xml
<input name="INPUT" flatten="true"  use_alias="true" />
```

### Delayed inputs

When a connection includes a delay-attribute, the target input obtains an additional dimension for the delayed signals unless the flatten-attribute is set on the input. If output M.OUTPUT is a two-dimensional matrix with dimensions a x b that is connected to N.INPUT with a delay range of 10 then the input wil become a three-dimensional matrix with dimensionas 10 x a x b.

```xml
<connection source=M.OUTPUT target="N.INPUT" delay=":10" />
```

In the module N the input can be accessed like this:

```C++
matrix input;

Bind(input, "INPUT");

matrix m = input[2]; // Get a 2-dimensional matrix from the input, two ticks back
```



## Outputs

Not much to say about outputs.