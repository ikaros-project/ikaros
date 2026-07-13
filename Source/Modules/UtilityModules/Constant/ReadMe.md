# Constant

## Description

Outputs a constant value. Constant provides a fixed matrix value to the rest of the network. The
configured `data` parameter is copied directly to `OUTPUT` every tick, preserving the shape of
`data`.

The `data` parameter can use normal matrix notation such as `1, 2; 3, 4`, or bracketed notation for
higher-rank tensors. For example, this produces an output with shape `[2, 2, 2]`:

```xml
<module class="Constant" name="Tensor" data="[[[1, 2], [3, 4]], [[5, 6], [7, 8]]]" />
```

![Constant](Constant.svg)

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| data | Value copied to `OUTPUT`; bracketed notation may be used for higher-rank tensors | matrix | 1, 2, 3, 4, 5, 6 |

## Outputs

| Name | Shape | Description |
| --- | --- | --- |
| OUTPUT | `data.shape` | Copy of `data` |
