* 'test.ikc'

This file contains its training points in the .data file.
Few enough points so that it is easy to see that the correct
neighbors are chosen. It was used for debugging during writing
of code.

Chosen neighbors are output to 'test.output'.


* 'Xlearn_Yfind.ikc'

These ikc files get their training points from the Randomizer
module. Too many points to draw the tree on paper... These were
run for debugging and benchmarking with CHECK_WITH_QS defined
(a quicksort algorithm was implemented with which the KD-Tree
compared its results in bigger tests).

For instance, 10000learn_1000find.ikc learns 10.000 (random)
points and then does 1.000 (random) searches in the tree.
If you choose to activate auto_rebuild you will see how often
the tree is rebuilt (and how big part of the tree). The values
you see withing parenthesis (if you have verbose="true"), eg (-17),
tell how many nodes in the tree are marked for deletion.
NOTE: when a part of the tree is rebuilt it might not be the
part of the tree where those to be deleted nodes are (so they might
still linger in the tree after a rebuild).

The benchmarks suggested that the KD-Tree is much faster than
using quicksort.

No output of chosen neighbors.
