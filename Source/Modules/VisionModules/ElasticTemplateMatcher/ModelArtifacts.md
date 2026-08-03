# Native model artifacts

The native matcher uses two externally supplied ONNX files. They are data artifacts rather than
source dependencies and are not stored in the repository. Put them in
`UserData/models/ElasticTemplateMatcher` and verify them against `models.sha256` before use.

| File | Network contract |
|---|---|
| `aliked-n16-320x240-512.onnx` | ALIKED N16, one RGB image `[1,3,240,320]`; 512 keypoints `[1,512,2]`, descriptors `[1,512,128]`, and scores `[1,512]` |
| `aliked-lightglue-512.onnx` | ALIKED LightGlue, independent dynamic feature dimensions from 1 to 512; match index and score for every feature in the first set |

The artifacts were exported from the official ALIKED N16 and ALIKED LightGlue v0.1 arXiv
weights with PyTorch 2.13. ALIKED used `max_num_keypoints=512` and a zero detection threshold.
LightGlue used nine layers with FlashAttention, early stopping, and point pruning disabled so the
export has a deterministic standard-operator graph. ONNX opset 20 was used and weights were
embedded in each file.

The exported graphs pass the ONNX checker and native ONNX Runtime 1.28 inference. A deterministic
native-versus-PyTorch comparison produced identical LightGlue match indices. Maximum absolute
differences were `1.53e-5` for ALIKED keypoints, `3.46e-6` for ALIKED descriptors, `5.29e-7` for
ALIKED scores, and `5.61e-6` for LightGlue scores.

This conversion is an offline build step. Ikaros does not load Python, PyTorch, or the original
`.pth` files at runtime.
