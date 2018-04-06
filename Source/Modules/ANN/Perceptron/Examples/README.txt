All the ikc example files for the Perceptron
module both train and activate from the same source.
They are also all connected to WebUI views and
should be started with 'ikaros -w name.ikc', then
you will see how the perceptron's (usually) error
averages go down and correctly classified examples
go up.


* 'logical_operators_and.data'

Data with the logical AND operator.
Both targets are 1 when both inputs are 1,
otherwise both targets are 0.


* 'multi.data'

The target is 1 when the product of both
inputs is > 0.42. Note that here we have
inputs with negative values. Inputs are
between -1.0 and +1.0.

* 'vowel.data'

Some complex training data. Does not look
easy to learn.
