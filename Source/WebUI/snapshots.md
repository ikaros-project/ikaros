# WebUI Snapshot Cache

The WebUI update path now uses a server-side snapshot cache to keep `/update` responses fast even when the simulation tick rate is much higher than the browser polling rate.

## What It Does

- During steady-state execution with active clients, the kernel builds a shared UI snapshot no more often than the configured WebUI request interval.
- `/update` responses read from that snapshot instead of serializing everything on demand.
- If a requested value is missing from the current snapshot, it is computed once as a fallback and then included in future snapshots.
- The first active client and subscription changes trigger an immediate snapshot instead of waiting for the next interval.

## Multi-Browser Behavior

- Subscriptions and log delivery cursors are tracked per WebUI client.
- The snapshot includes the union of values requested by all active clients.
- Inactive clients expire automatically after a short timeout.

This means one browser can change view without disturbing another browser that is showing different data.

## Snapshot And Image Throttling

Scalar values and status data are refreshed at the WebUI request cadence. Image entries can be refreshed less often because encoding them is substantially more expensive.

These parameters are built into the top group and can be set directly as attributes on the top-level `<group>` element:

- `webui_req_int`
  Minimum wall-clock time in seconds between complete WebUI snapshots and the browser polling interval for `/update`.
  Default: `0.1`

- `snapshot_interval`
  Minimum wall-clock time in seconds between image snapshot refreshes.
  Default: `0.1`

- `rgb_quality`
  JPEG quality used for RGB images in WebUI snapshots.
  Default: `75`

- `gray_quality`
  JPEG quality used for grayscale and pseudocolor images in WebUI snapshots.
  Default: `70`

These settings affect snapshot-backed `/update` image data. They do not change the ordinary image endpoint behavior.

Related top-group parameter:

- `webui_log_buffer_limit`
  Maximum number of recent log messages retained for delivery to WebUI clients.
  Default: `500`

Log delivery is independent of snapshot replacement. Each client advances its own cursor through the retained history, so messages emitted between snapshots remain available and another client's request cannot consume them. If a client falls behind far enough for messages to leave the bounded history, its next response includes a truncation warning.

## Why This Helps

- `/update` latency stays very low.
- Expensive image encoding is moved off the request path.
- Snapshot serialization follows the browser polling cadence instead of the simulation tick rate.
- If the browser polls slower than the simulation ticks, scalar serialization and image work are no longer repeated every tick unnecessarily.

## Tradeoff

Status and scalar data in `/update` can be up to one `webui_req_int` interval old. With a nonzero `snapshot_interval`, image data may be older than scalar values from the same response. Both bounds are intentional and reduce snapshot and encoding load while matching the browser's configured refresh cadence.
