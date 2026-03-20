# CannyEdgeDetector

## Description

Canny edge detector for grayscale images. Non-maximum suppression

It consumes INPUT and produces OUTPUT while parameters such as sigma, low_threshold, and
high_threshold shape its behavior. In a non-trivial vision stack, that makes it useful for building
layered perception pipelines that support active attention, object-directed reaching, scene
segmentation, or visually guided robot navigation.

Canny-style edge detection is usually valued for balancing sensitivity, localization, and
suppression of multiple responses to the same contour. In a larger model this makes it useful as an
early visual stage for contour extraction, object-boundary proposals, and attention guidance when
the downstream system needs a sparse but spatially meaningful description of scene structure.

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| sigma | Standard deviation for Gaussian blur | float | 1.0 |
| low_threshold | Low threshold for edge detection | float | 0.1 |
| high_threshold | High threshold for edge detection | float | 0.3 |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| INPUT | Grayscale input image (2D matrix) |  |

## Outputs

| Name | Description |
| --- | --- |
| OUTPUT | Binary edge map (2D matrix) |

*This description was automatically created and may not describe the full function of the module.*
