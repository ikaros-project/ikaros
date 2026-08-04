# Native learned template matching

This demo is implemented entirely in C++ at runtime. It uses ALIKED and LightGlue through ONNX
Runtime, native robust projective geometry, and native pyramidal Lucas-Kanade tracking. It does not
use Python or OpenCV, and the historical handcrafted elastic matcher is not retained as a fallback.

The current implementation targets macOS on Apple Silicon. ONNX Runtime is an external dependency
and must be installed separately; Ikaros never installs or downloads it. A Homebrew installation is
discovered by CMake without embedding a machine-specific path:

```sh
cmake -S . -B Build
cmake --build Build --parallel
```

If ONNX Runtime is absent, the rest of Ikaros can still build, but these learned-feature modules are
not included. The configure output reports whether the dependency was found.

## Model artifacts

Copy these two files to `UserData/models/ElasticTemplateMatcher`:

- `aliked-n16-320x240-512.onnx`
- `aliked-lightglue-512.onnx`

They are external weight artifacts and are intentionally not versioned. Before running the demo,
verify them from the repository root:

```sh
cd UserData/models/ElasticTemplateMatcher
shasum -a 256 -c ../../../Source/Modules/VisionModules/ElasticTemplateMatcher/models.sha256
```

The native inference boundary repeats the checksum validation at startup and rejects symlinks,
non-regular files, incorrect tensor contracts, and unsupported model shapes. See
`ModelArtifacts.md` for the conversion and network contracts. Python may be used for that one-time
offline conversion, but it is not part of the Ikaros runtime.

## Pipeline

`ALIKEDFeatureExtractor` emits runtime-sized keypoint, descriptor, and score matrices.
`TemplateFeatureBank` retains the features inside the central learning square whenever `LEARN`
receives a rising edge. Each press appends one template; `CLEAR` discards all templates.
The demo draws these retained reference features in yellow. Cyan boxes are reserved for current
geometrically matched or Lucas-Kanade-tracked features.

During detection, `LightGlueFeatureMatcher` produces dynamic correspondence rows and
`RobustTransformEstimator` verifies them with a RANSAC homography. `TemplatePolygonTransform`
validates the transformed quadrilateral and emits a closed, centered-coordinate path.

A verified detection seeds `PyramidalLucasKanadeTracker`. Forward-backward consistency and a robust
similarity transform stabilize the polygon between detections. `TemplateTrackingController`
requests learned inference after tracking failure and at the configured reacquisition interval.
ALIKED and LightGlue are gated off on ordinary tracking ticks.

On the Learn tick, `FeatureRegionFilter` restricts current LightGlue candidates to the same central
square used by the template bank. This prevents repeated structure elsewhere in the image from
seeding the first homography. The filter passes every feature during subsequent detection passes,
so reacquisition still searches the complete image.

The pipeline uses bounded setup-owned matrix capacities. Public feature, correspondence, template,
inlier, tracked-point, and path matrices resize only within those declared capacities; separate
count outputs are not used.

## Run

```sh
./Bin/ikaros Source/Modules/VisionModules/ElasticTemplateMatcher/ElasticTemplateMatcher_demo.ikg
```

The demo learns the initial square shown over the camera image. Its Path overlay follows projective
shape changes, and the labeled tables report controller status, selected template, and transform
quality. Templates are held in memory for the current run and are not persisted.
