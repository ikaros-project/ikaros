# Upsample

## Description

Upscales an image 2x using nearest neighbor. Upsample increases the spatial resolution of an image-
like input by a factor of two. The module is used when a coarser representation needs to be expanded
again, and the implementation description indicates a nearest-neighbor style reconstruction for the
output image.

It consumes INPUT and produces OUTPUT. A meaningful use case is to project coarse spatial decisions
back into image coordinates, for example when a dorsal-stream style attention map or a low-
resolution occupancy estimate must guide detailed visual or motor processing downstream.

Upsampling becomes important when decisions made at a coarse spatial scale need to be projected back
to a finer one. This can happen when an attention map, occupancy estimate, or low-resolution
prediction must be expanded so that later visual or motor stages can use it in the same coordinate
system as detailed image data.

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| INPUT | 2D input image |  |

## Outputs

| Name | Description |
| --- | --- |
| OUTPUT | Upscaled image (2x) |

*This description was automatically created and may not be an accurate description of the module.*
