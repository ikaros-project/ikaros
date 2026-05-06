# FaceClassifier

## Description

`FaceClassifier` is an online descriptor classifier. It stores several descriptor vectors per class
and outputs a one-hot vector for the class selected by k-NN voting.

If the nearest stored descriptor is farther than `threshold`, it is treated as an unknown candidate.
New classes are stored only when learning is enabled and a similar unknown descriptor has appeared
for `confirmation_count` ticks. `NOVELTY` is `1` only on ticks where a new class is added.

When a known class is matched and learning is enabled, the descriptor is stored as another sample for
that class until `vectors_per_class` is reached. Near-identical samples are ignored.

The optional `LEARN` input gates new class creation. If `LEARN` is unconnected, learning is enabled.
If it is connected, a nonzero value enables learning and zero disables learning.

An input descriptor containing only zeros is ignored and is never stored as a class.

For a 2D input matrix, the first row is used as the descriptor. This matches descriptor outputs that
emit one descriptor row for the current face.

## Outputs

| Output | Description |
| --- | --- |
| `OUTPUT` | One-hot vector with length `capacity`. |
| `NOVELTY` | Scalar, `1` when a new class was stored, otherwise `0`. |
| `DISTANCE` | Distance to the selected nearest neighbor, or `0` when a new class was stored. |
| `CLASSES` | Number of currently stored classes. |
| `COUNT` | Selection count for each class, with length `capacity`. |
| `VECTORS` | Stored descriptor vector count for each class, with length `capacity`. |

## Inputs

| Input | Description |
| --- | --- |
| `INPUT` | Descriptor vector, or first row of a descriptor matrix. |
| `LEARN` | Optional scalar learning gate. Nonzero enables new class storage. |

## Parameters

| Name | Description |
| --- | --- |
| `capacity` | Maximum number of stored classes and output vector length. |
| `threshold` | Store a new class when nearest-neighbor distance is greater than this value. |
| `confirmation_count` | Number of similar unknown inputs required before storing a new class. |
| `k` | Number of nearest stored vectors used for voting. |
| `vectors_per_class` | Maximum stored descriptor vectors per class. |
