import os
from pathlib import Path

import cv2
import numpy as np
import torch
from lightglue import ALIKED, LightGlue
from lightglue.utils import rbd


REPOSITORY_ROOT = Path(__file__).resolve().parents[4]
os.environ.setdefault("TORCH_HOME", str(REPOSITORY_ROOT / ".model-cache"))

extractor = None
matcher = None
device = None
templates = []
previous_gray = None
tracked_reference = None
tracked_current = None
tracked_template_id = -1
tracked_score = 0.0
tracked_correspondence_count = 0
last_learn = False
last_clear = False


def parameter(ctx, name, default):
    value = ctx.parameters.get(name, default)
    if value is None:
        return default
    while isinstance(value, (list, tuple)) and value:
        value = value[0]
    return value


def input_array(ctx, name):
    return np.asarray(ctx.inputs[name], dtype=np.float32)


def output_array(ctx, name):
    return np.asarray(ctx.outputs[name], dtype=np.float32)


def choose_device(requested):
    if requested == "mps":
        if not torch.backends.mps.is_available():
            raise RuntimeError("device=mps was requested but PyTorch MPS is unavailable")
        return torch.device("mps")
    if requested == "auto" and torch.backends.mps.is_available():
        return torch.device("mps")
    return torch.device("cpu")


def init(ctx):
    global extractor, matcher, device
    device = choose_device(str(parameter(ctx, "device", "auto")))
    keypoint_count = int(parameter(ctx, "max_num_keypoints", 512))
    extractor = ALIKED(max_num_keypoints=keypoint_count).eval().to(device)
    # The learned template contains only the central-region keypoints. Disable
    # adaptive pruning: its confidence calibration assumes two full feature
    # sets and can otherwise prune the smaller learned set before assignment.
    matcher = LightGlue(
        features="aliked",
        depth_confidence=-1,
        width_confidence=-1,
    ).eval().to(device)
    torch.set_grad_enabled(False)
    ctx.log.print(f"ALIKED and LightGlue initialized on {device}.")


def reset_outputs(ctx):
    for name in (
        "LOCATION",
        "MATCH",
        "VALID",
        "CORRESPONDENCE_COUNT",
        "INLIER_COUNT",
        "TRACKING",
        "MATCH_CORNERS",
        "MATCHED_FEATURES",
    ):
        output_array(ctx, name).fill(0)
    output_array(ctx, "TEMPLATE_ID").fill(-1)
    output_array(ctx, "TEMPLATE_COUNT")[0] = len(templates)


def update_learning_box(ctx, height, width):
    region = max(1.0, min(height, width) * float(parameter(ctx, "learning_region", 0.25)))
    box = output_array(ctx, "LEARNING_BOX")
    box.fill(0)
    box[0, 0] = -region / width
    box[0, 1] = -region / height
    box[0, 2] = 2.0 * region / width
    box[0, 3] = 2.0 * region / height


def image_tensor(gray):
    tensor = torch.from_numpy(np.ascontiguousarray(gray)).to(device=device, dtype=torch.float32)
    return tensor.unsqueeze(0).unsqueeze(0).clamp(0.0, 1.0)


def extract_features(gray):
    with torch.inference_mode():
        return extractor.extract(image_tensor(gray))


def subset_features(features, mask):
    result = {"image_size": features["image_size"]}
    for name in ("keypoints", "descriptors", "keypoint_scores"):
        result[name] = features[name][:, mask]
    return result


def learn_template(ctx, gray):
    height, width = gray.shape
    if float(np.std(gray)) < 1e-4:
        ctx.log.warning("Learning region has no usable image contrast; template was not learned.")
        return False
    features = extract_features(gray)
    keypoints = features["keypoints"][0]
    region = max(1.0, min(height, width) * float(parameter(ctx, "learning_region", 0.25)))
    center = keypoints.new_tensor([(width - 1) * 0.5, (height - 1) * 0.5])
    mask = torch.logical_and(
        torch.abs(keypoints[:, 0] - center[0]) <= region * 0.5,
        torch.abs(keypoints[:, 1] - center[1]) <= region * 0.5,
    )
    selected = int(mask.sum().item())
    minimum = int(parameter(ctx, "min_matches", 8))
    if selected < minimum:
        ctx.log.warning(
            f"Learning region contains only {selected} ALIKED keypoints; at least {minimum} are required."
        )
        return False

    half = region * 0.5
    corners = np.array(
        [
            [(width - 1) * 0.5 - half, (height - 1) * 0.5 - half],
            [(width - 1) * 0.5 + half, (height - 1) * 0.5 - half],
            [(width - 1) * 0.5 + half, (height - 1) * 0.5 + half],
            [(width - 1) * 0.5 - half, (height - 1) * 0.5 + half],
        ],
        dtype=np.float32,
    )
    templates.append({"features": subset_features(features, mask), "corners": corners})
    ctx.log.print(f"Learned template {len(templates) - 1} with {selected} ALIKED keypoints.")
    return True


def estimate_homography(reference_points, current_points, ctx):
    if len(reference_points) < 4:
        return None, None
    method = getattr(cv2, "USAC_MAGSAC", cv2.RANSAC)
    homography, mask = cv2.findHomography(
        np.asarray(reference_points, dtype=np.float32),
        np.asarray(current_points, dtype=np.float32),
        method,
        float(parameter(ctx, "ransac_threshold", 3.0)),
    )
    if homography is None or mask is None:
        return None, None
    return homography.astype(np.float64), mask.reshape(-1).astype(bool)


def transform_corners(template, homography):
    return cv2.perspectiveTransform(template["corners"][None], homography)[0]


def valid_geometry(corners, image_shape, ctx):
    if corners is None or not np.isfinite(corners).all():
        return False
    contour = corners.astype(np.float32).reshape(-1, 1, 2)
    if not cv2.isContourConvex(contour):
        return False
    area_fraction = abs(cv2.contourArea(contour)) / float(image_shape[0] * image_shape[1])
    return (
        float(parameter(ctx, "min_area_fraction", 0.002))
        <= area_fraction
        <= float(parameter(ctx, "max_area_fraction", 0.95))
    )


def detect_best(ctx, gray):
    current_features = extract_features(gray)
    current_unbatched = rbd(current_features)
    best = None
    diagnostic = {"correspondences": 0, "inliers": 0}

    for template_id, template in enumerate(templates):
        with torch.inference_mode():
            match_result = rbd(matcher({"image0": template["features"], "image1": current_features}))
        matches = match_result["matches"].detach().cpu().numpy()
        scores = match_result["scores"].detach().cpu().numpy()
        correspondence_count = len(matches)
        diagnostic["correspondences"] = max(diagnostic["correspondences"], correspondence_count)
        if correspondence_count < 4:
            continue

        reference = rbd(template["features"])["keypoints"][matches[:, 0]].detach().cpu().numpy()
        current = current_unbatched["keypoints"][matches[:, 1]].detach().cpu().numpy()
        homography, inlier_mask = estimate_homography(reference, current, ctx)
        if homography is None:
            continue
        inlier_count = int(inlier_mask.sum())
        diagnostic["inliers"] = max(diagnostic["inliers"], inlier_count)
        corners = transform_corners(template, homography)
        if inlier_count < int(parameter(ctx, "min_matches", 8)) or not valid_geometry(corners, gray.shape, ctx):
            continue

        confidence = float(scores[inlier_mask].mean()) if inlier_count else 0.0
        candidate = {
            "template_id": template_id,
            "homography": homography,
            "corners": corners,
            "reference": reference[inlier_mask].astype(np.float32),
            "current": current[inlier_mask].astype(np.float32),
            "score": confidence,
            "correspondences": correspondence_count,
            "inliers": inlier_count,
        }
        if best is None or (candidate["inliers"], candidate["score"]) > (best["inliers"], best["score"]):
            best = candidate

    return best, diagnostic


def track_previous(ctx, gray_u8):
    global tracked_current
    if previous_gray is None or tracked_current is None or len(tracked_current) < 4:
        return None

    source = tracked_current.reshape(-1, 1, 2).astype(np.float32)
    forward, status_forward, errors = cv2.calcOpticalFlowPyrLK(
        previous_gray,
        gray_u8,
        source,
        None,
        winSize=(21, 21),
        maxLevel=3,
        criteria=(cv2.TERM_CRITERIA_EPS | cv2.TERM_CRITERIA_COUNT, 30, 0.01),
    )
    if forward is None:
        return None
    backward, status_backward, _ = cv2.calcOpticalFlowPyrLK(
        gray_u8,
        previous_gray,
        forward,
        None,
        winSize=(21, 21),
        maxLevel=3,
        criteria=(cv2.TERM_CRITERIA_EPS | cv2.TERM_CRITERIA_COUNT, 30, 0.01),
    )
    if backward is None:
        return None

    forward_points = forward.reshape(-1, 2)
    backward_points = backward.reshape(-1, 2)
    fb_error = np.linalg.norm(backward_points - tracked_current, axis=1)
    good = (
        status_forward.reshape(-1).astype(bool)
        & status_backward.reshape(-1).astype(bool)
        & (errors.reshape(-1) <= float(parameter(ctx, "track_max_error", 20.0)))
        & (fb_error <= float(parameter(ctx, "track_fb_error", 1.5)))
    )
    reference = tracked_reference[good]
    current = forward_points[good]
    homography, inliers = estimate_homography(reference, current, ctx)
    if homography is None:
        return None
    reference = reference[inliers]
    current = current[inliers]
    template = templates[tracked_template_id]
    corners = transform_corners(template, homography)
    if len(current) < int(parameter(ctx, "min_matches", 8)) or not valid_geometry(corners, gray_u8.shape, ctx):
        return None
    return {
        "template_id": tracked_template_id,
        "homography": homography,
        "corners": corners,
        "reference": reference.astype(np.float32),
        "current": current.astype(np.float32),
        "score": tracked_score,
        "correspondences": tracked_correspondence_count,
        "inliers": len(current),
    }


def centered_coordinates(points, height, width):
    result = np.asarray(points, dtype=np.float32).copy()
    result[:, 0] = 2.0 * result[:, 0] / max(1, width - 1) - 1.0
    result[:, 1] = 2.0 * result[:, 1] / max(1, height - 1) - 1.0
    return result


def publish_result(ctx, result, height, width, tracking):
    corners = centered_coordinates(result["corners"], height, width)
    output_array(ctx, "LOCATION")[:] = corners.mean(axis=0)
    output_array(ctx, "MATCH")[0] = result["score"]
    output_array(ctx, "TEMPLATE_ID")[0] = result["template_id"]
    output_array(ctx, "VALID")[0] = 1
    output_array(ctx, "CORRESPONDENCE_COUNT")[0] = result["correspondences"]
    output_array(ctx, "INLIER_COUNT")[0] = result["inliers"]
    output_array(ctx, "TRACKING")[0] = 1 if tracking else 0
    output_array(ctx, "MATCH_CORNERS")[0, :] = corners.reshape(-1)

    feature_boxes = output_array(ctx, "MATCHED_FEATURES")
    feature_boxes.fill(0)
    points = centered_coordinates(result["current"], height, width)
    count = min(len(points), feature_boxes.shape[0])
    feature_width = 6.0 / width
    feature_height = 6.0 / height
    feature_boxes[:count, 0] = points[:count, 0] - 0.5 * feature_width
    feature_boxes[:count, 1] = points[:count, 1] - 0.5 * feature_height
    feature_boxes[:count, 2] = feature_width
    feature_boxes[:count, 3] = feature_height
    feature_boxes[:count, 4] = result["score"]


def tick(ctx):
    global previous_gray, tracked_reference, tracked_current, tracked_template_id
    global tracked_score, tracked_correspondence_count, last_learn, last_clear

    gray = input_array(ctx, "INPUT")
    if gray.ndim != 2:
        raise ValueError("INPUT must be a rank-2 grayscale image")
    if float(np.max(gray)) > 1.0:
        gray = gray / 255.0
    height, width = gray.shape
    gray_u8 = np.clip(gray * 255.0, 0, 255).astype(np.uint8)
    reset_outputs(ctx)
    update_learning_box(ctx, height, width)

    learn = bool(input_array(ctx, "LEARN").reshape(-1)[0] > 0.5)
    clear = bool(input_array(ctx, "CLEAR").reshape(-1)[0] > 0.5)
    if clear and not last_clear:
        templates.clear()
        tracked_reference = None
        tracked_current = None
        tracked_template_id = -1
        ctx.log.print("Cleared all learned templates.")
    if learn and not last_learn:
        last_learn = learn_template(ctx, gray)
    elif not learn:
        last_learn = False
    last_clear = clear
    output_array(ctx, "TEMPLATE_COUNT")[0] = len(templates)

    if not templates:
        previous_gray = gray_u8
        return

    interval = max(1, int(parameter(ctx, "detection_interval", 10)))
    should_detect = tracked_current is None or ctx.tick % interval == 0
    result = None
    tracking = False
    diagnostic = {"correspondences": 0, "inliers": 0}
    if should_detect:
        result, diagnostic = detect_best(ctx, gray)
    if result is None:
        result = track_previous(ctx, gray_u8)
        tracking = result is not None
    if result is None and not should_detect:
        result, diagnostic = detect_best(ctx, gray)

    if result is not None:
        tracked_template_id = result["template_id"]
        tracked_reference = result["reference"]
        tracked_current = result["current"]
        tracked_score = result["score"]
        tracked_correspondence_count = result["correspondences"]
        publish_result(ctx, result, height, width, tracking)
    else:
        tracked_reference = None
        tracked_current = None
        tracked_template_id = -1
        output_array(ctx, "CORRESPONDENCE_COUNT")[0] = diagnostic["correspondences"]
        output_array(ctx, "INLIER_COUNT")[0] = diagnostic["inliers"]

    previous_gray = gray_u8
