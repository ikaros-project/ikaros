# CameraRectification

Rectifies grayscale `[rows, cols]` or RGB `[3, rows, cols]` camera images using camera parameters produced by OpenCV calibration.

Inputs:

- `INPUT`: grayscale or RGB image.
- `CAMERA_MATRIX`: OpenCV `K` matrix, size `3x3`.
- `DISTORTION`: OpenCV distortion coefficients in the usual order `k1,k2,p1,p2,k3,k4,k5,k6,s1,s2,s3,s4,tauX,tauY`.

The module keeps the output size equal to the input size and assumes the new camera matrix is the same as `CAMERA_MATRIX`. It precomputes the inverse rectification map when image size or calibration values change, so each tick only performs bilinear remapping. Apple builds use Grand Central Dispatch for row-level parallelism; other systems use standard C++ threads.

The common OpenCV radial, tangential, rational, and thin-prism coefficients are supported. Tilt coefficients `tauX` and `tauY` are accepted but ignored.

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| border_value | Value used for rectified pixels that sample outside the source image | number | 0.0 |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| INPUT | Grayscale image [rows, cols] or RGB image [3, rows, cols] |  |
| CAMERA_MATRIX | OpenCV camera matrix K, size 3x3 |  |
| DISTORTION | OpenCV distortion coefficients k1,k2,p1,p2[,k3[,k4,k5,k6[,s1,s2,s3,s4[,tauX,tauY]]]] |  |

## Outputs

| Name | Description |
| --- | --- |
| OUTPUT | Rectified image with the same shape as INPUT |
