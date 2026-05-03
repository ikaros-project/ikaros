# AppleVisionFaceDetector

## Description

`AppleVisionFaceDetector` detects faces with Apple's Vision framework. It is available on macOS
builds only.

It accepts either a 2D grayscale image or an RGB image shaped as `3,height,width`. The outputs match
`DlibFaceDetector` so graphs can swap the detector module without changing downstream consumers.
The module reuses its `VNDetectFaceRectanglesRequest` and `VNSequenceRequestHandler` across ticks to
avoid per-frame Vision setup overhead.
Before calling Vision, it can also run one or more Ikaros 2x downsample passes. Since all detector
outputs are image-relative coordinates, downsampling does not change downstream coordinate handling.

`BOXES` is dynamic, with one centered coordinate row per detected face:

```text
x, y, width, height, confidence
```

The first four values use image-relative centered coordinates. `x` and `y` are the top-left corner
with `-1,-1` at the upper-left image corner, `0,0` at the image center, and `1,1` at the lower-right
image corner. `width` and `height` use the same scale, so a full image width or height is `2`.

`CENTER_BOX` uses the same row format, but contains only the detected face whose box center is
closest to the middle of the image. It is empty when no face is detected.

`FACE_CENTERS` contains one row per detected face:

```text
center_x, center_y, width
```

These values use the same centered coordinate scale. `center_x` and `center_y` describe the center
of the face box, and `width` describes the face-box width where a full image width is `2`.

## Parameters

| Name | Description |
| --- | --- |
| `max_faces` | Maximum number of detections emitted per tick. |
| `downsample` | Number of 2x Ikaros downsample passes before Vision detection. Default is `1`. |
