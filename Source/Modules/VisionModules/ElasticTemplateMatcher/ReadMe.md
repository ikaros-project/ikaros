# ElasticTemplateMatcher

`ElasticTemplateMatcher` is a Python-backed visual template learner and tracker using ALIKED,
LightGlue, robust homography estimation, and pyramidal Lucas-Kanade optical flow. The historical
handcrafted elastic matcher is not retained as a fallback.

## Runtime setup

From the repository root:

```sh
Source/Modules/VisionModules/ElasticTemplateMatcher/setup_runtime.sh
```

This creates `.venv-aliked-lightglue`, installs the pinned Python packages, and downloads the
official pretrained weights into `.model-cache`. Both directories are ignored by Git.

Run the demo with the generated interpreter:

```sh
./Bin/ikaros -p "$PWD/.venv-aliked-lightglue/bin/python" \
  Source/Modules/VisionModules/ElasticTemplateMatcher/ElasticTemplateMatcher_demo.ikg
```

## Learning and detection

A rising edge on `LEARN` extracts ALIKED features from the current frame and retains features in
the central learning square. Every learned template stores its reference features and four corner
points. `CLEAR` discards all templates on a rising edge.

During detection, ALIKED extracts current-frame features and LightGlue jointly matches each
learned feature set against them. OpenCV estimates a homography using USAC MAGSAC when available,
with RANSAC as a compatibility fallback. A result is accepted only when it has enough geometric
inliers and produces a finite, convex quadrilateral with a plausible image area.

## Tracking and reacquisition

Verified homography inliers seed pyramidal Lucas-Kanade tracking. Forward-backward consistency,
flow error, similarity-transform inlier count, convexity, and transformed area are checked every
tick. The lower-degree similarity transform prevents point noise from becoming perspective and
shear jitter, while temporal corner smoothing softens transitions at reacquisition. Full ALIKED and
LightGlue homography detection runs at `detection_interval`, immediately after tracking failure, and
whenever no template is currently tracked.

`MATCH_CORNERS` reports the homography-transformed quadrilateral as four centered-coordinate
points. `TRACKING` distinguishes Lucas-Kanade updates from full learned-feature detection.

The learned templates remain in memory for the current run and are not persisted.
