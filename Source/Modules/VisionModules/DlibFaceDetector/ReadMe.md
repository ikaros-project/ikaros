# DlibFaceDetector

## Description

`DlibFaceDetector` detects frontal faces with dlib's built-in HOG detector.

It accepts either a 2D grayscale image or an RGB image shaped as `3,height,width`. The output
`BOXES` is dynamic, with one centered coordinate row per detected face:

```text
x, y, width, height, score
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

The current dlib detector does not expose calibrated confidence scores through this simple API, so
`score` is currently `1`.

## Parameters

| Name | Description |
| --- | --- |
| `max_faces` | Maximum number of detections emitted per tick. |
| `upsample` | Number of dlib pyramid-up passes before detection. Higher values find smaller faces but cost more. |
