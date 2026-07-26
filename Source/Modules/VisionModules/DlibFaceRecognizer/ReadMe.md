# DlibFaceRecognizer

## Description

`DlibFaceRecognizer` computes one 128D dlib face descriptor per detected face.

It accepts either a 2D grayscale image or an RGB image shaped as `3,height,width`, together with
`BOXES` rows in the detector format:

```text
x, y, width, height, optional score
```

The module uses dlib's 5-point landmark model to align each detected face before running the face
recognition network. `DESCRIPTORS` is dynamic, with one 128-value row per recognized face.
`BOXES_OUT` reports the boxes that successfully produced descriptors:

```text
x, y, width, height, source row
```

The box coordinates use image-relative centered coordinates. `x` and `y` are the top-left corner
with `-1,-1` at the upper-left image corner, `0,0` at the image center, and `1,1` at the lower-right
image corner. `width` and `height` use the same scale, so a full image width or height is `2`.

## Model Files

Set these parameters to readable files inside the project directory or `UserData`:

| Parameter | Typical dlib file |
| --- | --- |
| `shape_predictor` | `shape_predictor_5_face_landmarks.dat` |
| `recognition_model` | `dlib_face_recognition_resnet_model_v1.dat` |

## Notes

Descriptor matching should live in a later module. A common first threshold is Euclidean distance
below about `0.6`, but the right value depends on camera, lighting, and enrollment data.

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| max_faces | Maximum number of faces to recognize | number | 16 |
| shape_predictor | Path to dlib shape_predictor_5_face_landmarks.dat | string |  |
| recognition_model | Path to dlib_face_recognition_resnet_model_v1.dat | string |  |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| INPUT | Grayscale image, or RGB image with shape 3,height,width |  |
| BOXES | Centered coordinate face boxes as rows: x, y, width, height, optional score |  |

## Outputs

| Name | Description |
| --- | --- |
| DESCRIPTORS | One 128D face descriptor per emitted face |
| BOXES_OUT | Centered coordinate boxes that produced descriptors: x, y, width, height, source row |
| COUNT | Number of descriptors emitted |
