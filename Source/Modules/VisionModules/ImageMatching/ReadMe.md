# ImageMatching

## Description

Claculates disaprity between two camera inputs, using a focus region in the left input to seach in
the right. Compute means

It consumes LEFT and RIGHT and produces OUTPUT, FOCUS, TARGET, and PROFILE while parameters such as
focus_size and search_size shape its behavior. In a non-trivial vision stack, that makes it useful
for building layered perception pipelines that support active attention, object-directed reaching,
scene segmentation, or visually guided robot navigation.

Image matching methods are particularly helpful when a system must localize a known pattern or align
a current observation with a remembered visual fragment. This supports landmark finding, fiducial
tracking, object re-detection, and calibration tasks in robotics, especially when a learned control
module needs robust visual anchors to stabilize behavior in a changing scene.

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| focus_size | Size of focus region for disparity matching in fraction of the total (left) image height; used for both height and width. | number | 0.01 |
| search_size | Heaigh of the search region in the right image as a fraction of the (right) image height. | number | 0.4 |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| LEFT | The left input |  |
| RIGHT | The right input |  |

## Outputs

| Name | Description |
| --- | --- |
| OUTPUT | The calculated disparity output in the range [-0.5, 0.5] |
| FOCUS | The focus area in the left image |
| TARGET | The found target area in the right image |
| PROFILE | The calculated disparity output |

*This description was automatically created and may not describe the full function of the module.*
