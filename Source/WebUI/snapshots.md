# WebUI Snapshot Cache

The WebUI update path now uses a server-side snapshot cache to keep `/update` responses fast even when the simulation tick rate is much higher than the browser polling rate.

## What It Does

- The kernel builds a shared UI snapshot between simulation ticks.
- `/update` responses read from that snapshot instead of serializing everything on demand.
- If a requested value is missing from the current snapshot, it is computed once as a fallback and then included in future snapshots.

## Multi-Browser Behavior

- Subscriptions are tracked per session.
- The snapshot includes the union of values requested by all active sessions.
- Inactive sessions expire automatically after a short timeout.

This means one browser can change view without disturbing another browser that is showing different data.

## Image Throttling

Scalar values and status data are refreshed every tick, but image entries in the snapshot are refreshed at a lower rate.

The top group can define these parameters:

- `snapshot_interval`
  Minimum time in seconds between image snapshot refreshes.
  Default: `0.1`

- `rgb_quality`
  JPEG quality used for RGB images in WebUI snapshots.
  Default: `75`

- `gray_quality`
  JPEG quality used for grayscale and pseudocolor images in WebUI snapshots.
  Default: `70`

These settings affect snapshot-backed `/update` image data. They do not change the ordinary image endpoint behavior.

## Why This Helps

- `/update` latency stays very low.
- Expensive image encoding is moved off the request path.
- If the browser polls slower than the simulation ticks, image work is no longer repeated every tick unnecessarily.

## Tradeoff

With a nonzero `snapshot_interval`, image data in `/update` may be slightly older than scalar values from the same response. This is intentional and reduces encoding load substantially for image-heavy views.
