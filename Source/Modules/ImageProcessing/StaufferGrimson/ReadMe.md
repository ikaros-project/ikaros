# StaufferGrimson

## Description

Forground/background segmentation. Implementation of the Stauffer-Grimson forground/background
segmentation for grayscale and color images. Initialize the background model

It consumes INPUT and produces OUTPUT, WEIGHTS, MEANS, and VARIANCES while parameters such as T,
threshold, max_gaussians, initial_variance, and learning_rate shape its behavior. In a non-trivial
vision stack, that makes it useful for building layered perception pipelines that support active
attention, object-directed reaching, scene segmentation, or visually guided robot navigation.

External computer-vision practice is useful here because these operations are usually strongest when
combined into layered pipelines rather than used alone. Their role is often to reduce raw image data
into spatial cues that later modules can use for recognition, action selection, and visually guided
control.

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| T | The background portion | float | 0.8 |
| threshold | The threshold as value times standard deviation | number | 2.5 |
| max_gaussians | Number of gaussians per pixel | number | 5 |
| initial_variance | The initial variance | float | 100 |
| learning_rate | The learning rate | rate | 0.01 |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| INPUT | The input image |  |

## Outputs

| Name | Description |
| --- | --- |
| OUTPUT | The processed image |
| WEIGHTS | The weights |
| MEANS | The mean of each gaussian |
| VARIANCES | The variance of each gaussian |

*This description was automatically created and may not describe the full function of the module.*
