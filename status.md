# Kernel Review Status

## BrainStudio heat-map visualization

The tasks below will be completed sequentially, with one focused commit per task.

| # | Task | Status | Verification | Commit |
|---:|---|---|---|---|
| 1 | Add a selectable Heat Map widget derived from Grid with scalar color maps, fixed or automatic value ranges, optional zero inclusion, and a reusable vertical color legend. | Completed | JavaScript syntax for all widgets; focused automatic-range, zero-inclusion, default-legend, and shared legend-format checks; XML validation and four-tick Ikaros smoke; browser verification of an animated 8x8 scalar matrix, automatic diverging range, reserved legend geometry, unclipped endpoint labels, normalized zero label, edge-to-edge no-scale layout, high-DPI canvas sizing, and no warnings or errors; `git diff --check`. | `Heat Map now renders scalar matrices with legends` |
| 2 | Add an animated `.ikg` example showing fixed and automatic ranges, multiple color maps, labels, scale visibility, and optional legends. | Completed | XML validation; four-tick Ikaros smoke; JavaScript syntax for all widgets; browser verification of four non-overlapping animated variants with fire, spectrum, gray, and custom maps, fixed and automatic ranges, rectangular and circular cells, row labels, row/column orders, visible/invisible/disabled scales, optional legends, and no warnings or errors; `git diff --check`. | `Added an animated Heat Map gallery` |
| 3 | Make inherited Grid pointer interaction account for the horizontal space reserved by the Heat Map legend. | Completed | Focused legend-enabled and legend-disabled pointer-metric checks confirm reserved legend width is excluded from cell hit testing while ordinary Grid geometry remains unchanged; JavaScript syntax; `git diff --check`. | `Heat Map interaction now excludes legend space` |

### Constraints

- Preserve Grid matrix orientation, labels, cell shapes, and interaction behavior.
- Reserve legend space through the shared graph and vertical-legend infrastructure.
- Exclude RGB grids from scalar legends and automatic scalar ranges.
- Keep widgets and source modules non-overlapping in the example view.
- Complete, verify, and commit the widget before adding the example.

### Outstanding issues and questions

None.

## ElasticTemplateMatcher learning-region correction

| # | Task | Status | Verification | Commit |
|---:|---|---|---|---|
| 1 | Visualize the features actually retained in learned templates, separately from current matched and tracked features. | Completed | Full Debug build; XML validation; deterministic point-to-box test verified three pixel points map to the expected centered target boxes; demo now overlays retained template points in yellow separately from cyan matched/tracked points. | `Learned template features now have a distinct overlay` |
| 2 | Restrict current match candidates to the central learning square during Learn while retaining full-image matching for later reacquisition. | In progress |  |  |

### Constraints

- Keep the reusable filtering and visualization conversion in separate native C++ modules.
- Use bounded dynamic `ikaros::matrix` outputs and do not modify the kernel.
- Preserve full-image learned-feature extraction and later global reacquisition.
- Complete, verify, and commit task 1 before task 2.

### Outstanding issues and questions

None.

## Modern learned template matching demo

The old handcrafted elastic matcher will be removed rather than retained as a fallback. The tasks
below will be completed sequentially with one focused commit per task.

| # | Task | Status | Verification | Commit |
|---:|---|---|---|---|
| 1 | Replace the handcrafted matcher with ALIKED feature extraction, LightGlue correspondence matching, robust homography verification, and Lucas-Kanade tracking/reacquisition while preserving multi-template learning and centered output coordinates. | Completed | Full CMake build; Python compilation; XML and shell validation; five-tick CPU smoke learned 101 ALIKED points, accepted 101 LightGlue correspondences and 101 homography inliers, and reported VALID=1. | `Template matching now uses learned features and tracking` |
| 2 | Replace the axis-aligned match-box overlay with a closed Path driven by the four homography-transformed template corners. | Completed | XML validation; Python compilation; five-tick CPU smoke retained VALID=1 with 101 correspondences/inliers after removing MATCH_BOX; `git diff --check`. | `Matched templates now use projective path overlays` |

### Constraints

- Use a Python-backed Ikaros class and shared-memory transport for learned-model inference.
- Keep Learn and Clear as single-button interactions through explicit trigger inputs.
- Detect/reacquire globally, track verified points between detection passes, and fall back immediately when tracking quality fails.
- Keep model/runtime dependencies outside version control and provide a reproducible setup command.
- Keep widgets non-overlapping except for the intentional image/path/feature overlays.
- Complete, verify, and commit task 1 before starting task 2.

### Outstanding issues and questions

- The pretrained runtime remains an explicit local setup step and is intentionally excluded from version control.
- Live camera behavior and the WebUI overlay still require an interactive hardware smoke test; the deterministic image smoke covers learning, matching, homography output, and output-shape setup.


## BrainStudio event-raster visualization 

The tasks below will be completed sequentially, with one focused commit per task.

| # | Task | Status | Verification | Commit |
|---:|---|---|---|---|
| 1 | Add a selectable Event Raster widget with dense-vector and sparse event-list inputs, fixed-capacity sparse history, level or rising-edge detection, scrolling or fixed time, channel labels, tick/dot/square and magnitude styles, grids, and optional color legend. | Completed | JavaScript syntax for all widgets; focused rising-edge, level, sparse-row, and fixed-capacity ring checks; XML validation and five-tick Ikaros smoke; browser verification of live scrolling rows, channel labels, time grid and scale, alternating backgrounds, tick events, now line, magnitude coloring, correctly sized shared legend, and high-DPI canvas sizing; `git diff --check`. | `Event Raster now displays sparse channel activity` |
| 2 | Add an animated `.ikg` demo showing varied firing rates, bursts, edge and level detection, marker styles, labels, magnitude coloring, scrolling, and a color legend. | Completed | XML validation; five-tick Ikaros smoke; JavaScript syntax; browser verification of four non-overlapping live variants with isolated edge-triggered spikes, sustained level-triggered bursts, graded magnitude-colored dots and legend, dense flipped-channel activity, three marker styles, labels, dark and light themes, and varied firing rates; `git diff --check`. | `Added an animated Event Raster gallery` |
| 3 | Freeze event sampling and the scrolling time window when BrainStudio enters the stopped state and reports an invalid tick sentinel. | Completed | Focused stopped and invalid-controller clock checks; JavaScript syntax; browser Stop-control regression in the animated gallery with screenshots after a 2.2-second interval verified byte-for-byte identical; `git diff --check`. | `Stopped simulations now freeze Event Raster time` |

### Constraints

- Store only detected events in fixed-capacity typed circular buffers.
- Synchronize dense sampling with simulation ticks and avoid per-tick history allocation.
- Treat channel rows as discrete coordinates and time as the horizontal coordinate.
- Reuse the shared color-map and vertical color-legend infrastructure.
- Preserve normal BrainStudio Edit-mode interaction.
- Complete, verify, and commit the widget before adding the demo.

### Outstanding issues and questions

None.


## BrainStudio trace visualization

The tasks below will be completed sequentially, with one focused commit per task.

| # | Task | Status | Verification | Commit |
|---:|---|---|---|---|
| 1 | Add a selectable Trace widget that records live 2D positions into fixed-size per-series ring buffers with packed and separate-coordinate inputs, fading/tapering styles, current/start markers, direction arrows, discontinuities, and graph-coordinate controls. | Completed | JavaScript syntax for all widgets; focused discontinuity, invalid-value, and fixed-capacity ring checks; XML validation and live Ikaros smoke; browser verification of packed and separate multi-trace inputs, tick-synchronized histories, trail styles, markers, labels, direction arrows, visible and disabled scales, and no browser warnings or errors; `git diff --check`. | `Live positions now retain spatial Trace histories` |
| 2 | Add an animated `.ikg` demo showing single and multiple traces, packed and separate-coordinate sources, trail styles, markers, labels, sampling intervals, and jump breaks. | Completed | XML validation; four-tick Ikaros smoke; JavaScript syntax; non-overlapping layout audit; four variants cover packed and separate inputs, single and multiple traces, solid/fade/taper/fade-taper styles, sampling intervals, automatic ranges, visible/invisible/disabled scales, labels, markers, direction arrows, and jump breaks; `git diff --check`. | `Added an animated Trace widget gallery` |

### Constraints

- Keep Trace distinct from Path: Trace owns temporal history while Path renders a supplied trajectory.
- Reuse Graph coordinate, scale-visibility, color, and axis conventions.
- Use fixed-capacity ring buffers and avoid per-tick history reallocation.
- Treat non-finite positions and configured large jumps as segment breaks.
- Preserve normal BrainStudio Edit-mode interaction.
- Complete, verify, and commit the widget before adding the demo.

### Outstanding issues and questions

None.


## BrainStudio surface-plot visualization

The tasks below will be completed sequentially, with one focused commit per task.

| # | Task | Status | Verification | Commit |
|---:|---|---|---|---|
| 1 | Add a selectable Three.js Surface Plot widget for rank-2 height data with efficient geometry updates, surface/wireframe/points modes, height or source coloring, perspective/orthographic cameras, orbit controls, axes, grid, bounding box, and a reusable vertical color legend. | Completed | JavaScript syntax for all widgets; XML validation and two-tick Ikaros smoke; focused browser verification of masked source-colored surface with legend, wireframe orthographic mode, fixed-color points, separate overlay/WebGL sizing, registration, and clean browser console; `git diff --check`. | `Matrix data now has an interactive Surface Plot` |
| 2 | Add an animated `.ikg` demo showing waves, peaks, saddle-like surfaces, multiple display modes, camera projections, color modes, and legend options. | Completed | XML validation; three-tick Ikaros smoke; JavaScript syntax for all widgets; browser verification of four non-overlapping variants, perspective and orthographic cameras, surface/wireframe/points modes, fixed/height/source colors, separate legends, live wave and peak deformation, and no warnings or errors; `git diff --check`. | `Added an animated Surface Plot gallery` |

### Constraints

- Treat rank-2 input as a regular height field; accept optional X, Y, color, and mask sources with compatible shapes.
- Rebuild geometry only when topology changes and update typed vertex/color buffers in place during normal ticks.
- Keep height and color ranges independent, with fixed and automatic modes.
- Preserve BrainStudio Edit-mode interaction and widget lifecycle conventions.
- Reuse existing Three.js, OrbitControls, color-map, and vertical-legend infrastructure.
- Complete, verify, and commit the widget before adding the demo.

### Outstanding issues and questions

None.


## BrainStudio widget state-selector consistency

The tasks below will be completed sequentially, with one focused commit per task.

| # | Task | Status | Verification | Commit |
|---:|---|---|---|---|
| 1 | Correct scale-visibility selectors and spacing behavior for Path, Marker, Grid, and Vector Field so disabled scales render edge to edge while visible and invisible modes retain their intended geometry. | Completed | CSS selector audit; XML validation; JavaScript syntax; browser verification of zero-space Grid and Vector Field no-scale states and reserved Vector Field visible-scale geometry with no warnings or errors; invisible-state tick/grid geometry and transparent decoration audit; `git diff --check`. | `Scale-disabled widgets now render edge to edge` |
| 2 | Correct Bar Graph and Plot orientation selectors so their generated `orientation-*` classes activate the intended layout rules. | Completed | CSS selector audit; JavaScript syntax; browser verification that runtime `orientation-vertical` classes activate Bar Graph and Plot vertical direction/layout rules with no warnings or errors; horizontal selector correspondence audit; `git diff --check`. | `Graph orientation classes now activate layout styles` |
| 3 | Update the repository multiple-task workflow so implementation proceeds without a permission checkpoint unless outstanding issues require user direction. | Completed | Instruction review confirms task recording remains mandatory and permission is requested only when detected outstanding issues or questions require user direction; `git diff --check`. | `Multi-task work now proceeds when unblocked` |

### Constraints

- Use zero plot spacing when `scale_visibility="no"`.
- Preserve scale geometry but hide its decorations when `scale_visibility="invisible"`.
- Preserve existing serialized parameter names and values.
- Complete, verify, and commit each task separately.
- Ask for permission before implementation only when detected outstanding issues require user direction.

### Outstanding issues and questions

None.


## BrainStudio navigation and pan-tilt HUD overlays

The tasks below will be completed sequentially, with one focused commit per task.

| # | Task | Status | Verification | Commit |
|---:|---|---|---|---|
| 1 | Add framework-only angular HUD drawing infrastructure for wrapping and bounded tapes, mirrored ticks and labels, targets, readouts, and transparent overlay styling. | Completed | JavaScript syntax; focused angle normalization, wrap-safe target difference, cardinal labels, radians conversion, signed formatting, and framework-only registration audit; `git diff --check`. | `Angular HUD widgets now share a rendering base` |
| 2 | Add a selectable Navigation HUD with the upper half of the mirrored bottom heading band and optional planar movement readouts. | Completed | JavaScript syntax; focused source binding, wrapping upper-tape mode, shared bottom-band geometry, safe margins, target input, and registration checks; `git diff --check`. | `Navigation HUD now overlays planar movement data` |
| 3 | Add a selectable Pan-Tilt HUD with the lower mirrored pan band, a right-side vertical tilt scale, current and target indicators, and optional central reticle. | Completed | JavaScript syntax; focused lower mirrored-tape geometry, bounded pan, right-side tilt extent, target values, reticle errors, safe margins, and registration checks; `git diff --check`. | `Pan-Tilt HUD now mirrors navigation overlays` |
| 4 | Add an intentionally overlaid `.ikg` demo using slow sinusoidal heading, pan, tilt, and target inputs so both HUD layers can be evaluated over the same background. | Completed | XML validation; two-tick Ikaros smoke; JavaScript syntax for all widgets; browser verification of identical overlay geometry, animated canvas output, background image, separated readout lanes, and no warnings or errors; `git diff --check`. | `Added a live mirrored HUD overlay demo` |

### Constraints

- Keep Navigation HUD and Pan-Tilt HUD as separate selectable widgets with a shared framework-only base.
- Align their default bottom-band geometry so the navigation ticks point down and pan ticks point up across one shared centerline.
- Keep both HUD canvases transparent, frameless, titleless, and pointer-transparent by default.
- Keep the tilt scale on the right and terminate it above the mirrored bottom band.
- Treat overlap among the background and two HUD widgets in the demo as intentional; keep modules and unrelated view elements outside that shared rectangle.
- Complete, verify, and commit each task before starting the next.

### Outstanding issues and questions

None.


## BrainStudio polar-plot visualization

The tasks below will be completed sequentially, with one focused commit per task.

| # | Task | Status | Verification | Commit |
|---:|---|---|---|---|
| 1 | Add a dedicated Polar Plot widget supporting labeled vector axes, multiple matrix-row series, radial scaling, configurable grid geometry, angular layout, and series styling. | Completed | JavaScript syntax for all widgets; focused registration, rank-1/rank-2 series, invalid-series rejection, fixed/automatic range, label fallback, and angular direction checks; `git diff --check`. | `Polar data now has a dedicated BrainStudio widget` |
| 2 | Add an example `.ikg` demonstrating explicit and source labels, multiple series, grid shapes, direction and offset controls, and fixed and automatic ranges. | Completed | XML validation; two-tick Ikaros smoke test; focused label-precedence and angle-aware geometry checks; browser visual verification of four non-overlapping variants with no warnings or errors; `git diff --check`. | `Added a Polar Plot widget example` |

### Constraints

- Treat rank-1 input as one series and rank-2 input as one series per row.
- Resolve axis labels from `label_source`, source metadata, explicit `labels`, then numeric indices.
- Keep the initial widget read-only and focused on radar/spider plots rather than explicit angle-radius curves.
- Complete, verify, and commit the widget before adding the example model.

### Outstanding issues and questions

None.


## BrainStudio vector-field visualization

The tasks below will be completed sequentially, with one focused commit per task.

| # | Task | Status | Verification | Commit |
|---:|---|---|---|---|
| 1 | Add a dedicated Vector Field widget supporting channel-first and separate-component inputs, configurable vector scaling and anchoring, arrow rendering, and graph coordinates. | Completed | JavaScript syntax; focused registration, shape validation, and relative/coordinate/normalized scaling checks; `git diff --check`. | `Vector fields now have a dedicated BrainStudio widget` |
| 2 | Add an optional reusable vertical color legend and integrate it with Vector Field while keeping the component suitable for other color-mapped widgets. | Completed | JavaScript syntax for all widgets; focused legend spacing, color clamping, automatic magnitude range, and color-source shape checks; `git diff --check`. | `Color-mapped widgets now share a vertical legend` |
| 3 | Add an example `.ikg` demonstrating the Vector Field input forms, scaling, anchoring, coloring, and legend options. | Completed | XML validation; two-tick Ikaros smoke test; browser visual verification of four non-overlapping variants and separate legends; no browser warnings or errors; `git diff --check`. | `Added a Vector Field widget example` |

### Constraints

- Keep Vector Field as a separate selectable widget rather than a Path mode.
- Use the Ikaros channel-first convention for packed vector-field input.
- Build the color legend as shared WebUI infrastructure rather than Vector Field-specific drawing code.
- Complete, verify, and commit task 1 before starting task 2.
- Complete and commit the reusable legend before adding the example model.

### Outstanding issues and questions

None.


## WebUI superclass consolidation

The refactor will reduce repeated widget code while preserving public widget registrations, parameters, serialized models, runtime behavior, and framework-only base-class visibility.

| # | Task | Status | Verification | Commit |
|---:|---|---|---|---|
| 1 | Expand `WebUIWidgetControl` with shared enabled-state, Edit-mode, index, selected-source, and indexed-write behavior; migrate applicable controls. | Completed | JavaScript syntax for the base and six migrated controls; focused selection, indexed-write, enabled, and Edit-mode checks; `git diff --check`. Net 129 lines removed. | `Control widgets now share indexed source behavior` |
| 2 | Add a framework-only slider superclass and reduce Horizontal and Vertical Slider to shared behavior plus orientation-specific structure/layout. | Completed | JavaScript syntax; focused registration, template, indexed-write, command, numeric-list, and control-count checks; `git diff --check`. Net 416 lines removed. | `Sliders now share a framework base` |
| 3 | Add listener lifecycle helpers to `WebUIWidget` and migrate widgets with document/window or retained element handlers. | Completed | JavaScript syntax for the base and ten migrated widgets; focused registration/removal, idempotent early cleanup, option preservation, and disconnect cleanup checks; direct-listener inventory; `git diff --check`. | `Widget listeners now share lifecycle management` |
| 4 | Add shared source normalization and numeric access helpers to `WebUIWidget` and migrate repeated widget implementations. | Completed | JavaScript syntax for the base and nine migrated widgets; focused nested flattening, scalar/fallback, finite/positive number, source-number, and matrix-row checks; `git diff --check`. Net 20 lines removed. | `Widgets now share source normalization` |
| 5 | Add canvas clearing/begin-draw helpers to `WebUIWidgetCanvas` and migrate repeated canvas setup sequences. | Completed | JavaScript syntax for all widget files; focused scaled-transform, default/explicit offset, clear bounds, and margin-translation checks; `git diff --check`. Net 7 lines removed. | `Canvas widgets now share drawing setup` |

### Superclass consolidation constraints

- Keep all new base classes framework-only and absent from the widget selector.
- Preserve public parameters, yes/no values, widget tag names, serialized models, and runtime behavior.
- Keep World 2D's intentional Edit-mode interaction and Text's inline editing behavior unchanged.
- Complete, verify, and commit each task independently in the listed order.

### Superclass consolidation outstanding issues and questions

- Browser-level visual and interaction regression testing remains manual because the repository has no automated browser widget suite.
- World 2D's specialized editor redraw path and Epi Head's custom oversized clear remain intentionally separate from the generic canvas helpers.

## WebUI Edit-mode interaction fixes

World 2D View remains intentionally interactive in Edit mode. All other runtime controls should yield mouse interaction to the component editor and must not issue runtime commands or parameter changes.

| # | Task | Status | Verification | Commit |
|---:|---|---|---|---|
| 1 | Finalize and commit the already implemented Canvas 3D cancellable model loading change before further edits to that file. | Completed | JavaScript syntax; focused fetch-abort and stale-parse disposal checks; `git diff --check`. | `Canvas 3D model loading is now cancellable` |
| 2 | Add central Edit-mode command suppression while preserving the direct World 2D editor command path. | Completed | JavaScript syntax; focused Edit/runtime command-queue checks; confirmed World 2D uses its separate direct command path; `git diff --check`. | `Widget commands are now suppressed in Edit mode` |
| 3 | Disable Drop-down Menu and Table slice controls in Edit mode and allow their events to reach component dragging. | Completed | JavaScript syntax; focused Edit/runtime disabled-state checks; reviewed event propagation and change guards; `git diff --check`. | `Menu controls now yield to Edit-mode dragging` |
| 4 | Disable Switch, Color Picker, Horizontal Slider, and Vertical Slider interaction in Edit mode without blocking component dragging. | Completed | JavaScript syntax for all four widgets; focused Edit/runtime disabled and pointer-transparency checks; event-propagation review; `git diff --check`. | `Control inputs now yield to Edit-mode dragging` |
| 5 | Stop Canvas 3D input forwarding and wheel interception from interfering with component editing. | Completed | JavaScript syntax; reviewed all forwarding/wheel Edit-mode guards and per-frame canvas pointer transparency; `git diff --check`. | `Canvas 3D now yields input while editing` |
| 6 | Make disabled Drop-down Menu and Table slice selectors pointer-transparent in Edit mode for consistent cross-browser dragging. | Completed | JavaScript syntax; focused Edit/runtime disabled and pointer-transparency checks for both selectors; `git diff --check`. | `Select controls now yield consistently while editing` |

### Edit-mode constraints

- Preserve World 2D View as the explicit interactive Edit-mode exception.
- Preserve Text's intentional inline editing behavior.
- Keep runtime behavior unchanged outside Edit mode.
- Complete, verify, and commit each task independently in the listed order.

### Edit-mode outstanding issues and questions

- Interactive dragging across all supported browsers was not automated; focused event-state checks verify the widget logic, while the repository still lacks a browser-level widget regression suite.

## WebUI widget bug review

Each widget implementation will be reviewed sequentially for concrete defects. Straightforward, narrowly scoped fixes will be implemented and verified as part of that widget's task. Framework-only base classes are included in the review but remain unavailable in the widget selector. Each task will be committed independently before the next begins.

| # | Widget or framework class | Status | Verification | Commit |
|---:|---|---|---|---|
| 1 | `WebUIWidget` framework base | Completed | JavaScript syntax; focused source, metadata, indexed CSS, Boolean, and zero-color behavior check; `git diff --check`. | `WebUIWidget base values are now handled safely` |
| 2 | `WebUIWidgetControl` framework base | Completed | JavaScript syntax; subclass inheritance and non-registration audit. No defect required a source change. | `Reviewed WebUIWidgetControl framework base` |
| 3 | `WebUIWidgetCanvas` framework base | Completed | JavaScript syntax; focused layout-guide context and margin-position check; `git diff --check`. | `Canvas layout guides now use the drawing context` |
| 4 | `WebUIWidgetGraph` framework base | Completed | JavaScript syntax; focused single-tick, non-zero flipped range, decimal clamping, and finite-coordinate check; `git diff --check`. | `Graph axes now handle edge-case tick counts` |
| 5 | Image | Completed | JavaScript syntax; focused empty-source and zero-opacity behavior check; static image load-path review; `git diff --check`. | `Image widget opacity and loading are now reliable` |
| 6 | Target Boxes | Completed | JavaScript syntax; focused score precision, font fallback, and canvas-bound label check; `git diff --check`. | `Target Boxes scores now render safely` |
| 7 | Bar Graph | Completed | JavaScript syntax; focused negative auto-range, missing-source clearing, and line-cap option check; `git diff --check`. | `Bar Graph now handles negative and missing data correctly` |
| 8 | Histogram | Completed | JavaScript syntax; focused negative auto-range, missing-source clearing, single-tick centering, and line-cap review; `git diff --check`. | `Histogram ranges and axes now handle edge cases` |
| 9 | Box Plot | Completed | JavaScript syntax; focused negative auto-range and missing-source clearing check; narrow-slot and line-width review; `git diff --check`. | `Box Plot now handles ranges and narrow layouts safely` |
| 10 | Scatter Plot | Completed | JavaScript syntax; focused negative range, plot-margin, single-tick, and precision checks; absent-data clearing review; `git diff --check`. | `Scatter Plot ranges and layout now handle edge cases` |
| 11 | Plot | Completed | JavaScript syntax; focused ring-buffer resize/order, invalid capacity, and negative auto-range checks; missing/non-finite data review; `git diff --check`. | `Plot history now resizes and renders safely` |
| 12 | Path | Completed | JavaScript syntax; focused open-path fill, single-point arrow, selection normalization, parameter redraw, and missing-source clearing checks; `git diff --check`. | `Path rendering now respects open paths and valid segments` |
| 13 | Marker | Completed | JavaScript syntax; focused selection, label precision/value fallback, parameter redraw, and missing-source clearing checks; non-finite coordinate review; `git diff --check`. | `Marker rendering now handles invalid values safely` |
| 14 | World 2D View | Completed | JavaScript syntax; focused scaling, color fallback, nested command path, and parameter-redraw checks; two-tick World2D model smoke test; `git diff --check`. | `World 2D View interaction paths are now robust` |
| 15 | World 3D View | Completed | JavaScript syntax; focused camera-distance, opacity, color fallback, vertical placement, invalid-coordinate, and resource-cleanup checks; `git diff --check`. | `World 3D View resources and camera are now reliable` |
| 16 | Grid | Completed | JavaScript syntax; focused RGB source-name, current-label, RGB metrics, invalid label width, and missing-source clearing checks; color-range review; `git diff --check`. | `Grid sources and rendering now handle edge cases` |
| 17 | Text | Completed | JavaScript syntax; focused zero-value, missing-source placeholder, selection formatting/clearing, and decimal-bound checks; `git diff --check`. | `Text values now update without stale or lost content` |
| 18 | Rectangle | Completed | JavaScript syntax; focused horizontal alignment, null-label, and negative dimension checks; `git diff --check`. | `Rectangle labels now align correctly` |
| 19 | Table | Completed | JavaScript syntax; focused first-update rendering, numeric-string/non-finite formatting, decimal bounds, yes/no colorization, and stale-table clearing checks; `git diff --check`. | `Table data now renders immediately and clears safely` |
| 20 | Button | Completed | JavaScript syntax; focused edit-mode suppression, push/release, index normalization, scalar state, and stale enabled/file-source checks; multi-button recursion and icon reuse review; `git diff --check`. | `Button interactions now release and update safely` |
| 21 | Joystick | Completed | JavaScript syntax; focused non-finite position/value, invalid index, and indexed source checks; drag-handler lifecycle review; `git diff --check`. | `Joystick values and indices are now normalized safely` |
| 22 | Switch | Completed | JavaScript syntax; focused scalar zero/one, missing-source clearing, and invalid-index checks; edit-mode input and control-count review; `git diff --check`. | `Switch values now synchronize without stale state` |
| 23 | Horizontal Slider | Completed | JavaScript syntax; focused invalid-index, empty-target, and yes/no value-label checks; keyboard/edit interaction and numeric configuration review; `git diff --check`. | `Horizontal Slider interaction state now recovers safely` |
| 24 | Vertical Slider | Completed | JavaScript syntax; focused invalid-index and yes/no value-label checks; keyboard/edit interaction and numeric configuration review; `git diff --check`. | `Vertical Slider interaction state now recovers safely` |
| 25 | Color Picker | Completed | JavaScript syntax; focused row-index and yes/no value-label checks; keyboard/edit interaction and step validation review; `git diff --check`. | `Color Picker interaction and labels now update safely` |
| 26 | Drop-down Menu | Completed | JavaScript syntax; focused numeric zero, missing-source clearing, and indexed string selection checks; option caching and label-width review; `git diff --check`. | `Drop-down Menu options and values now stay synchronized` |
| 27 | Canvas 3D | Completed | JavaScript syntax; focused camera-distance update and line-parameter checks; point-buffer resizing/first-frame, missing-data clearing, invalid-matrix, model-count, and resource-lifecycle review; `git diff --check`. | `Canvas 3D data and resources now update reliably` |
| 28 | Key Points | Completed | JavaScript syntax; focused zero-range and invalid-sequence cache checks; parameter-only redraw, mark-line stroke, scalar position/target, and drag-listener lifecycle review; `git diff --check`. | `Key Points drawing now handles incomplete data safely` |
| 29 | Sequence Grid | Completed | JavaScript syntax; focused array-name normalization and valid/invalid RGB checks; edit-mode event propagation and gap validation review; `git diff --check`. | `Sequence Grid cells now handle edit mode and malformed data safely` |
| 30 | Epi Head | Completed | JavaScript syntax; focused nested-vector, color-channel clamp, gaze/pupil/head normalization checks; malformed RGB matrix and non-legacy source review; `git diff --check`. | `Epi Head inputs now render with safe values` |

### Review constraints

- Review tasks in the listed order with only one task in progress at a time.
- Include framework-only base classes in the review without making them selectable.
- Implement only straightforward, narrowly scoped bug fixes discovered during each review.
- Record architectural improvements, ambiguous behavior, and nontrivial changes as outstanding issues instead of implementing them implicitly.
- Preserve the standardized public parameter names and `yes`/`no` Boolean values unless correcting an unambiguous defect.
- Verify and commit every task independently, including its `status.md` update.

### Outstanding issues and questions

- Key Points still contains unresolved product-level choices about vertical-axis direction and how input/output/active traces should be presented. These were left unchanged because the intended visualization semantics are ambiguous.
- Verification was primarily syntax checks and focused headless JavaScript checks. World 2D View also received an Ikaros model smoke test; a full interactive browser regression suite for all widgets does not currently exist.

## WebUI image encoding follow-ups

The tasks below will be completed sequentially, with one focused commit per task.

| # | Task | Status | Verification | Commit |
|---:|---|---|---|---|
| 1 | Add a documented CMake preset for running ThreadSanitizer with Homebrew LLVM on macOS. | Completed | Preset listing and configuration; full Homebrew Clang 22.1.8 TSan build; `test_363_webui_image_encoder.ikg` passed without sanitizer reports. | `Homebrew LLVM TSan now has a CMake preset` |
| 2 | Add a repeatable Release benchmark for WebUI image capture, JPEG encoding, response latency, and image-active tick throughput. | Completed | Debug and Release builds; Python syntax and short HTTP/CSV smoke probes; default five-repeat 1024×1024 Release run measured 0.057 ms capture, 2.465 ms JPEG/Base64, 1.193 ms first response, 10.939 ms completion, 0.931 ms cached response, and 99.99% tick-throughput retention; all 267 tests passed. | `WebUI image performance now has a repeatable benchmark` |

### Outstanding issues and questions

- None.

## Post-refactoring consistency pass

The tasks below will be completed sequentially, with one focused commit per task.

| # | Task | Status | Verification | Commit |
|---:|---|---|---|---|
| 1 | Bring historical `status.md` findings up to date with the completed removals, moves, and retained APIs. | Completed | Historical findings reconciled against the current source tree and completed commits; Markdown diff check passed. | `Kernel review status now reflects completed work` |
| 2 | Audit `kernel_shapes.cc` and `kernel_scheduling.cc` for self-contained, minimal direct includes and declarations. | Completed | Both units require the private `Kernel` definition and already use only `ikaros.h`; CMake target build passed. No source change required. | `New kernel units now have verified include boundaries` |
| 3 | Re-run the unused-function audit across the split kernel implementation and resolve confirmed internal dead code. | Completed | Removed five private diagnostics functions referenced only by comments; member-pointer handlers and retained/public APIs excluded; CMake Debug build and all 266 tests passed. | `Removed unused kernel diagnostics functions` |
| 4 | Review the remaining responsibilities in `kernel_setup.cc` and extract another unit only if a clear cohesive boundary remains. | Completed | Startup-step analysis and JSON reporting moved intact to `kernel_startup_steps.cc`; stale local snapshot declarations removed; CMake Debug build and all 266 tests passed. | `Startup-step analysis now has its own implementation unit` |
| 5 | Review repository-unused public APIs (`Kernel::HasOption()`, `Kernel::GetOptionLong()`, `Component::BindParameter()`, and `Component::info()`) for external compatibility and document or implement the appropriate disposition. | Completed | `HasOption()` and `GetOptionLong()` retained as coherent module-facing option queries; obsolete duplicate `BindParameter()` and malformed debugging `info()` removed; CMake Debug build and all 266 tests passed. | `Obsolete Component APIs were removed` |

### Outstanding issues and questions

None.

## BrainStudio multi-stream image stability

The tasks below address the synchronized blinking seen with eight image streams in split panes and the separate inability of `InputVideo` to resume after Stop followed by Play. They will be completed sequentially and committed independently.

| # | Task | Status | Verification | Commit |
|---:|---|---|---|---|
| 1 | Make WebUI image updates generation-safe, retain the last successfully decoded frame until its replacement is ready, and prevent stale callbacks or timeouts from triggering synchronized blank redraws. | Completed | JavaScript syntax; focused generation dispatch, superseded decode, failed decode, latest-frame swap, and redraw checks; live six-pane `AppleVisionFaceDetector_video_widget_test.ikg` browser stress run with sustained updates and no browser warnings/errors; `git diff --check`. | `Image streams now retain frames during decoding` |
| 2 | Make Stop followed by Play restart systems containing destructively stopped modules such as `InputVideo`, without changing Pause/Play behavior. | Completed | Focused Stop/Play reload and Pause/Play continuation tests; real `AppleVisionFaceDetector_video_widget_test.ikg` Stop/Play smoke test resumed ticks and returned `Camera.OUTPUT:rgb`; Debug build; all 269 kernel and WebUI tests passed; `git diff --check`. | `Stopped systems now reload before playing` |

### Constraints

- Preserve the last valid image under decode or encoder pressure; overload may drop frames but must not blank the view.
- Keep image-update work generation-scoped and avoid polling loops or per-refresh timer accumulation.
- Keep the WebUI image fix independent from the kernel/module lifecycle fix.
- Preserve Pause/Play as a non-destructive continuation path.
- Complete, verify, and commit task 1 before starting task 2.

### Outstanding issues and questions

None.


## WebUI widget parameter standardization

This is an intentionally breaking migration. Old parameter names will not be retained as aliases. Each task includes migration of affected repository models and focused verification, and will be committed independently before the next task begins.

| # | Task | Status | Verification | Commit |
|---:|---|---|---|---|
| 1 | Resolve parameters whose current names have conflicting meanings: separate frame and scene backgrounds, rename Sequence Grid's color source, rename Grid's color-map selector, and distinguish control counts from point counts. | Completed | JavaScript syntax checks; XML validation for every changed `.ikg`; stale-name audit; `git diff --check`. | `WebUI widget parameters now avoid conflicting meanings` |
| 2 | Standardize source-binding names with snake_case and `_source` suffixes across graph, Key Points, Sequence Grid, Drop-down, Color Picker, and control widgets. | Completed | JavaScript syntax checks; XML validation for every changed `.ikg`; repository-wide stale-name audit; `git diff --check`. | `WebUI source bindings now use consistent names` |
| 3 | Standardize writable target, command, selection-index, and enabled-state names and declared binding types across Button, Switch, sliders, Joystick, Drop-down, Color Picker, Grid, Text, Image, and Sequence Grid. | Completed | JavaScript syntax checks; XML validation for changed models except the pre-existing trailing quote in `InputVideoStream_test.ikg`; repository-wide stale-name audit; `git diff --check`. | `WebUI control targets and commands now use consistent names` |
| 4 | Standardize common drawing-style names, including line, marker, label, color, fill, and legend parameters, using semantic snake_case names. | Completed | JavaScript syntax checks; XML validation for changed models except the documented pre-existing invalid model; Canvas API property audit; stale-name audit; `git diff --check`. | `WebUI drawing styles now use semantic snake case` |
| 5 | Standardize coordinate ranges, axes, scales, grid lines, margins, canvas flips, graph orientation, and Table transposition names and Boolean types. | Completed | JavaScript syntax checks; XML validation except the documented pre-existing invalid model; duplicate-attribute and stale-name audits; `git diff --check`. | `WebUI axes and layout now use consistent parameters` |
| 6 | Define and apply a consistent common graph parameter set across Bar Graph, Histogram, Box Plot, Scatter Plot, and Plot, adding meaningful missing axis, range, legend, precision, and layout controls. | Completed | JavaScript syntax checks; common graph-template comparison; `git diff --check`. | `WebUI graphs now expose consistent axis controls` |
| 7 | Standardize the general control-family API and add missing parity features, including Joystick enabled state and Vertical Slider command support. | Completed | JavaScript syntax checks; control-path review; `git diff --check`. | `WebUI controls now share enabled and command capabilities` |
| 8 | Standardize Canvas 3D parameters and add title, scene background, line width, and explicit camera controls. | Completed | JavaScript syntax check; affected model XML validation; source-request and camera-path review; `git diff --check`. | `Canvas 3D now has a consistent configurable interface` |
| 9 | Align World 2D View and World 3D View shared parameters and defaults, separate scene styling from frame styling, and add appropriate 3D visibility, camera, and opacity controls. | Completed | JavaScript syntax checks; shared-template and rendering-path review; `git diff --check`. | `World views now share consistent display controls` |
| 10 | Expand Target Boxes presentation parameters and normalize its score visibility type and defaults. | Completed | JavaScript syntax check; affected model XML validation; rendering-path review; `git diff --check`. | `Target Boxes now has complete presentation controls` |
| 11 | Align Rectangle and Text presentation parameters for text color, font, alignment, padding, formatting, editability, and placeholder behavior where applicable. | Completed | JavaScript syntax checks; presentation and formatting-path review; `git diff --check`. | `Rectangle and Text now share presentation controls` |
| 12 | Perform a final repository-wide parameter audit, update documentation and gallery/example models, run WebUI tests and relevant `.ikg` smoke tests, and record outstanding issues. | Completed | All widget JavaScript syntax checks; changed-model XML validation except one documented pre-existing malformed model; WebUI HTTP smoke test; two-tick World2D model smoke test; public camelCase and Boolean-value audits; `git diff --check`. | `WebUI widget parameters are now standardized` |

### Constraints

- Do not provide backward-compatible aliases for renamed parameters.
- Prefer `yes`/`no` over `true`/`false` for serialized and user-facing Boolean values.
- Update affected repository `.ikg` files in the same commit as each breaking rename.
- Keep one task in progress at a time and make one focused commit per task.
- Preserve widget behavior except where a task explicitly adds a missing control.

### Outstanding issues and questions

- WebGL implementations may ignore `LineBasicMaterial.linewidth`; the Canvas 3D parameter is exposed consistently but platform rendering support varies.


## Connection encapsulation completion

| Task | Status | Verification | Commit |
|---|---|---|---|
| Privatize all `Connection` fields and provide read-only accessors where external inspection is required. | Completed | CMake Debug build; all 266 kernel and WebUI tests passed, including `OutputFile` connection metadata coverage. | `Connection metadata is now read-only outside the kernel` |

### Outstanding issues and questions

None.


## Remaining kernel refactoring

The tasks below will be completed sequentially, with one focused commit per task.

| # | Task | Status | Verification | Commit |
|---:|---|---|---|---|
| 1 | Encapsulate scalar-state capture, restore, and reset type dispatch in `ScalarState`. | Completed | CMake Debug build; all 266 kernel and WebUI tests passed. | `ScalarState now owns persistence operations` |
| 2 | Consolidate repeated numeric conversion logic in `parameter.cc` while preserving option-index and rate semantics. | Completed | CMake Debug build; all 266 kernel and WebUI tests passed. | `Parameter numeric conversions now share one path` |
| 3 | Split `Connection::Tick()` into named whole-buffer, flattened-delay, and indexed-delay propagation paths; preserve allocation-free execution and benchmark it. | Completed | CMake Debug and Release builds; all 266 kernel and WebUI tests passed; Release probe changed from 123.73 to 71.69 ns/connection (3 repeats, no regression). | `Connection propagation now uses named paths` |
| 4 | Separate model construction from shape convergence in `kernel_setup.cc` without changing startup behavior. | Completed | CMake Debug build; all 266 kernel and WebUI tests passed. | `Shape convergence now has its own implementation unit` |
| 5 | Extract graph scheduling from `kernel_execution.cc` into a private implementation unit with characterization coverage. | Completed | CMake Debug build; all 266 kernel and WebUI tests passed, including cycle and parallel zero-delay scheduling coverage. | `Graph scheduling now has its own implementation unit` |
| 6 | Replace session logging's small friend accessors with one immutable metadata snapshot and share common event construction. | Completed | CMake Debug build; all 266 kernel and WebUI tests passed, including asynchronous session-log delivery. | `Session logging now uses metadata snapshots` |
| 7 | Privatize appropriate `Connection` implementation fields after propagation restructuring, preserving the required kernel interface. | Completed | CMake Debug build; all 266 kernel and WebUI tests passed, including external module compilation. | `Connection runtime state is now private` |

### Outstanding issues and questions

None.


## Buffer diagnostics consolidation

| Task | Status | Verification | Commit |
|---|---|---|---|
| Replace the duplicate, unfiltered input/output buffer listings with one truthful buffer listing. | Completed | CMake Debug build; all 266 kernel and WebUI tests passed, including both `info="true"` models. | `Buffer diagnostics now use one truthful listing` |

### Outstanding issues and questions

None.


## Startup-step formatting cleanup

| Task | Status | Verification | Commit |
|---|---|---|---|
| Share startup-step formatting through one private `Component` helper. | Completed | CMake Debug build; all 266 kernel and WebUI tests passed. | `Startup step formatting now shares one helper` |

### Outstanding issues and questions

None.


## Module class implementation ownership

| Task | Status | Verification | Commit |
|---|---|---|---|
| Move `Class` implementation from `request.cc` to `module_class.cc`. | Completed | CMake Debug build; all 266 kernel and WebUI tests passed. | `Module class implementation now has its own file` |

### Outstanding issues and questions

None.


## Confirmed unused kernel cleanup

| Task | Status | Verification | Commit |
|---|---|---|---|
| Keep `Kernel::ListClasses()` and remove only `Kernel::AllocateInputs()`, `Kernel::AuthEnabled()`, and `Kernel::DoSendLog()`. | Completed | Reference check confirms only `ListClasses()` remains; Debug build; all 266 kernel and WebUI tests passed. | `Removed unused kernel functions` |

### Outstanding issues and questions

None.


## Split kernel file review

This is a read-only review. Each split implementation and focused declaration file is checked for unused functions, unnecessary exposure, simplification opportunities, and potential follow-up refactoring. No implementation changes are included.

| # | Review area | Status | Verification | Commit |
|---:|---|---|---|---|
| 1 | Core declarations and lifecycle: `ikaros.h`, `ikaros.cc`, `kernel_types.h` | Completed | Declaration/definition and repository-wide reference audit | `Reviewed split kernel files` |
| 2 | Parameters: `parameter.h`, `parameter.cc`, `kernel_parsing.h`, `kernel_parsing.cc` | Completed | Declaration/definition and helper-use audit | `Reviewed split kernel files` |
| 3 | Components and modules: `component.h`, `component.cc`, `module.h`, `module.cc`, `component_runtime.h` | Completed | Override, registration, and repository-wide call-site audit | `Reviewed split kernel files` |
| 4 | Setup and class metadata: `kernel_setup.cc`, `module_class.h` | Completed | Helper-use, ownership, and diagnostic-path audit | `Reviewed split kernel files` |
| 5 | Execution and buffering: `kernel_execution.cc`, `circular_buffer.h`, `circular_buffer.cc`, `connection.h`, `connection.cc` | Completed | Scheduling, callback, and propagation-path audit | `Reviewed split kernel files` |
| 6 | State persistence: `kernel_state.cc` | Completed | Capture/restore/reset call-chain and duplication audit | `Reviewed split kernel files` |
| 7 | WebUI data and HTTP: `kernel_webui.cc`, `kernel_http.cc`, `request.h`, `request.cc` | Completed | Route, endpoint, serialization, and reference audit | `Reviewed split kernel files` |
| 8 | Diagnostics, paths, and session logging: `kernel_diagnostics.cc`, `kernel_paths.cc`, `session_logging.h`, `session_logging.cc` | Completed | Diagnostic option, path-policy, queue, and helper-use audit | `Reviewed split kernel files` |
| 9 | Cross-file synthesis: confirm unused candidates, rank simplifications, and propose staged refactoring | Completed | Low-reference candidates manually validated against callbacks, overrides, friends, registration, and public API exposure | `Reviewed split kernel files` |

### Review constraints

- Keep `ikaros.h` as the overarching include for all modules.
- Do not introduce PImpl.
- Do not change code during this review.
- Treat apparent unused private functions conservatively when callbacks, registration, or external API use may apply.
- Separate straightforward cleanup from architectural proposals.

### Disposition of originally identified internal code

- `Kernel::AllocateInputs()`, `Kernel::AuthEnabled()`, and `Kernel::DoSendLog()` were removed in `Removed unused kernel functions`.
- `Kernel::ListClasses()` was deliberately retained for class diagnostics.

### Disposition of repository-unused public API candidates

- `Kernel::HasOption()` and `Kernel::GetOptionLong()` are retained as coherent module-facing option queries alongside `GetOption()` and `IsOptionExplicitlySet()`.
- Obsolete `Component::BindParameter()` was removed; parameter binding remains part of the active `ResolveParameter()` and `LookupParameter()` path.
- Lowercase `Component::info()` was removed; it was an unused debugging printer with duplicated and mislabeled output.
- `msg_inherit`, `msg_quiet`, and `msg_exception` are not referenced in repository C++ code. They are public protocol constants and should not be removed as an internal cleanup.

### Disposition of proposed simplifications and refactorings

- `Class` implementation now resides in `module_class.cc`.
- Startup-step formatting now shares one private helper.
- Stale `WAS:` comments and commented profiling code were removed.
- Startup diagnostics now use one truthful `ListBuffers()` listing; the misleading input/output variants were removed.
- `ScalarState` now owns capture, restore, and reset operations.
- Parameter numeric conversions now share one conversion path while retaining option and rate semantics.
- `Connection::Tick()` now delegates to named propagation paths and retained Release performance.
- Shape convergence now resides in `kernel_shapes.cc`.
- Graph scheduling now resides in `kernel_scheduling.cc`.
- Session logging now captures one metadata snapshot and shares event-path construction.
- All `Connection` fields are private; modules inspect connection metadata through const accessors.

### Files requiring no targeted cleanup

- `kernel_parsing.cc`, `circular_buffer.cc`, and `kernel_paths.cc` are small, cohesive, and showed no unused functions or compelling simplification.
- `module.cc` is cohesive; its forwarding time/profiling methods are intentional module API, not redundant dead code.

### Outstanding issues and questions

- Decide whether repository-unused public methods remain supported external module API before removing them.
- Decide whether identical input/output/buffer diagnostics are intentional aliases or should filter by buffer role.


## Kernel support declaration extraction

All modules continue to include `ikaros.h`; the focused headers establish internal ownership and are included by that umbrella. Each extraction is built, tested, and committed independently.

| # | Task | Status | Verification | Commit |
|---:|---|---|---|---|
| 1 | Move the `CircularBuffer` declaration into a self-contained `circular_buffer.h`. | Completed | Standalone header syntax check; Debug build; all 266 kernel and WebUI tests passed. | `Circular buffer declarations now have a focused header` |
| 2 | Move the `Connection` declaration into a self-contained `connection.h`. | Completed | Standalone header syntax check; Debug build; all 266 kernel and WebUI tests passed. | `Connection declarations now have a focused header` |
| 3 | Move the `Request` declaration into a self-contained `request.h`. | Completed | Standalone header syntax check; Debug build; all 266 kernel and WebUI tests passed. | `Request declarations now have a focused header` |
| 4 | Move the internal class-registration metadata declaration into a focused kernel header with an unambiguous name. | Completed | Standalone `module_class.h` syntax check; Debug build; all 266 kernel and WebUI tests passed. | `Module class declarations now have a focused header` |

### Constraints

- Keep `ikaros.h` as the overarching, documented include for all modules.
- Preserve source compatibility, behavior, diagnostics, and serialized formats.
- Do not migrate module include sites.
- Keep every extracted header self-contained.
- Verify and commit each extraction independently.

### Outstanding issues and questions

None.


## Kernel API encapsulation and header extraction

The work proceeds sequentially without introducing PImpl or changing public behavior. Each implementation change is verified and committed independently.

| # | Task | Status | Verification | Commit |
|---:|---|---|---|---|
| 1 | Audit direct access to public `Kernel` fields outside `Kernel` member functions and existing friends. | Completed | No production module directly accesses `Kernel` fields; direct access is limited to kernel implementation, startup control in `main.cc`, and the CPU accounting test. | `Audited kernel public field access` |
| 2 | Move implementation-only `Kernel` fields and nested WebUI structures under `private:`. | Completed | Debug build succeeded; kernel-owned startup and session-logging access is isolated through private friends, and the CPU test uses only public behavior. | `Kernel implementation state is now private` |
| 3 | Add only narrowly scoped accessors required by legitimate external callers found by the audit. | Completed | The audit found no legitimate external field caller; no public accessor was added. | `Confirmed kernel fields need no public accessors` |
| 4 | Build and run the complete kernel test suite for the `Kernel` privacy change. | Completed | Debug build succeeded; all 266 kernel and WebUI tests passed. | `Verified private kernel implementation state` |
| 5 | Commit the verified `Kernel` privacy change independently. | Completed | Implementation commit `5f9f0565`; full verification recorded in `b9e04533`. | `Kernel implementation state is now private` |
| 6 | Extract the `parameter` declarations into a focused, self-contained header and verify and commit the change independently. | Completed | Standalone header syntax check; Debug build; all 266 kernel and WebUI tests passed. | `Parameter declarations now have a focused header` |
| 7 | Extract the `Component`/`Group` and `Module`/registration declarations into focused, self-contained headers, verifying and committing each extraction independently. | Completed | Standalone syntax checks for both headers; Debug builds; all 266 kernel and WebUI tests passed after each extraction. | `Component declarations now have a focused header`; `Module declarations now have a focused header` |

### Constraints

- Do not introduce PImpl.
- Preserve the existing top-level `ikaros.h` as a source-compatible umbrella header.
- Preserve public behavior and avoid unrelated API changes.
- Keep headers self-contained and remove reliance on indirect includes where touched.
- Do not migrate module include sites as part of these declaration extractions; that remains a later, separately reviewed step.

### Outstanding issues and questions

None.


This file tracks the high-, medium-, and lower-priority findings from the joint review. Findings remain pending commit until implementation, focused tests, the full kernel test suite, and any relevant performance verification have completed.

## Thread and task review

| # | Priority | Finding | Status | Commit |
|---:|:---:|---|---|---|
| 1 | P1 | Parallel zero-delay connections between the same component pair collapse into one task. | Addressed | `Thread and task scheduling defects were corrected` |
| 2 | P2 | Partial `ThreadPool` construction can terminate the process. | Addressed | `Thread and task scheduling defects were corrected` |
| 3 | P2 | `TaskSequence` cannot safely be resubmitted or executed concurrently. | Addressed | `Thread and task scheduling defects were corrected` |
| 4 | P2 | Parallel console output can race. | Excluded by decision | — |
| 5 | P3 | Malformed thread counts are accepted. | Addressed | `Thread and task scheduling defects were corrected` |
| 6 | P3 | Timeout conversion loses sub-millisecond precision. | Addressed | `Thread and task scheduling defects were corrected` |
| 7 | P3 | Task errors are reported twice and out of order. | Addressed | `Thread and task scheduling defects were corrected` |
| 8 | P3 | `Task` lacks a virtual destructor. | Addressed | `Thread and task scheduling defects were corrected` |
| 9 | P3 | Public `ThreadPool` inputs are insufficiently validated. | Addressed | `Thread and task scheduling defects were corrected` |
| 10 | P3 | Recursive task-graph algorithms have an avoidable depth limit. | Addressed | `Thread and task scheduling defects were corrected` |

### Thread and task verification

- Debug build completed successfully.
- Six focused scheduler, watchdog, exception, and public-API tests passed.
- The complete kernel suite passed all 163 tests.
- The public `ThreadPool` API test passed under Homebrew Clang ThreadSanitizer.
- An iterative task-graph stress model completed setup with 4,000 modules and 8,001 graph nodes.
- The Release delayed-propagation benchmark measured 42.56 ns/connection before and 42.55 ns/connection after the changes.

## Socket and server review

| # | Priority | Finding | Status | Commit |
|---:|:---:|---|---|---|
| 1 | P1 | HTTP shutdown races with the request thread while closing descriptors and clearing connection state. | Addressed | `HTTP server shutdown was synchronized` |
| 2 | P1 | Partial TCP requests are rejected or can block indefinitely depending on accepted-socket behavior. | Addressed | `Partial HTTP requests were handled incrementally` |
| 3 | P1 | Outbound `Socket` DNS, connection, write, and read operations have no deadlines. | Addressed | `Outbound socket operations received deadlines` |
| 4 | P1 | The Linux build uses the macOS-only `SO_NOSIGPIPE` socket option unconditionally. | Addressed | `Socket SIGPIPE handling became portable` |
| 5 | P2 | Complete pipelined requests already present in a connection buffer are not processed. | Addressed | `Buffered HTTP requests were processed promptly` |
| 6 | P2 | The keep-alive idle timeout is not enforced while `select()` waits indefinitely. | Addressed | `Keep-alive idle timeouts were enforced` |
| 7 | P2 | HTTP message framing accepts ambiguous lengths and does not consistently consume request bodies. | Addressed | `HTTP request framing was validated strictly` |
| 8 | P2 | Malformed and unsupported requests can remain open without an HTTP error response. | Addressed | `Invalid HTTP requests received explicit errors` |
| 9 | P2 | File transfer requires write access, buffers whole files, ignores short reads and send failures, and can expose uninitialized bytes. | Addressed | `File responses were streamed safely` |
| 10 | P2 | Client `Socket` reuse, failed connection attempts, and short writes can corrupt descriptor ownership or truncate requests. | Addressed | `Client socket ownership was made reliable` |
| 11 | P2 | Unbounded connections can pass descriptors outside the valid `select()` `fd_set` range. | Addressed | `Select descriptor limits were enforced` |
| 12 | P3 | `ServerSocket` construction and temporary listener flag changes are not exception-safe. | Addressed | `Server socket construction became exception-safe` |
| 13 | P3 | The socket API exposes fragile state and obsolete declarations, while an unused experimental server remains in the tree. | Addressed | `Socket server state was encapsulated` |

### Socket and server verification

- The Debug build completed successfully.
- Ten focused socket and HTTP server tests passed as part of the kernel suite.
- The complete kernel suite passed all 173 tests.
- The split-request and HTTP-thread shutdown path passed under Homebrew Clang ThreadSanitizer.

## Component, group, module, and class review

| # | Priority | Finding | Status | Commit |
|---:|:---:|---|---|---|
| 1 | P1 | Async modules race when reading kernel time and statistics. | Addressed | `Async runtime snapshots stabilized module time reads` |
| 2 | P1 | WebUI state save, load, and reset operations race with active async modules. | Addressed | `State operations waited for active async modules` |
| 3 | P1 | A failing deferred action permanently wedges async component state. | Addressed | `Deferred action failures finalized async state` |
| 4 | P1 | Failed setup can leave a partially initialized model executable. | Addressed | `Failed setup cleanup removed partial networks` |
| 5 | P2 | Groups can accidentally run in module-only async mode. | Addressed | `Async execution was restricted to modules` |
| 6 | P2 | Custom component JSON serialization bypasses async protection. | Addressed | `Custom JSON reads respected async updates` |
| 7 | P2 | Resetting a selected group does not reset its child components. | Addressed | `Scoped resets included child components` |
| 8 | P2 | Input-dependent dynamic output capacities fail instead of deferring resolution. | Addressed | `Dynamic capacities deferred unresolved input shapes` |
| 9 | P2 | A whole-output alias renames its source matrix metadata. | Addressed | `Whole-output aliases preserved source metadata` |
| 10 | P2 | Scalar-state defaults and loaded Boolean values are not parsed strictly. | Addressed | `Scalar state parsing rejected malformed values` |
| 11 | P2 | Flattened input size accumulation can overflow. | Addressed | `Flattened input sizing prevented cumulative overflow` |

## Lower-priority defects and hardening

| # | Priority | Finding | Status | Commit |
|---:|:---:|---|---|---|
| 12 | P3 | Group outputs reject `size` but silently ignore the equally forbidden `shape` attribute. | Addressed | `Group outputs rejected explicit shapes` |
| 13 | P3 | `GetIntValue()` accepts trailing characters in integer attributes. | Addressed | `Integer attributes rejected trailing characters` |
| 14 | P3 | Binding the same scalar state twice silently leaves the first binding stale. | Addressed | `Scalar states rejected conflicting bindings` |
| 15 | P3 | `ClearOutputs()` clears component metadata but leaves registered kernel buffers behind. | Addressed | `Unsafe output clearing API was removed` |
| 16 | Hardening | `Component` exposes scheduling, async, identity, and profiler internals publicly. | Addressed | `Component runtime internals were encapsulated` |
| 17 | Hardening | `Module` overrides are not declared explicitly and its destructor is not defaulted as an override. | Addressed | `Module overrides were declared explicitly` |
| 18 | P3 | Class scanning trusts the `.ikc` filename without validating the declared class name. | Addressed | `Class metadata names were validated during scanning` |
| 19 | P3 | Class validation tests use a host-specific absolute fixture path. | Addressed | `Duplicate class files were rejected during scanning` |
| 20 | P3 | Duplicate `.ikc` filenames silently replace previously scanned class metadata. | Addressed | `Duplicate class files were rejected during scanning` |
| 21 | Hardening | Unused public `Class` constructors bypass scanner validation. | Addressed | `Duplicate class files were rejected during scanning` |
| 22 | Cleanup | `ScanClasses()` retains an obsolete error-handling `FIXME`. | Addressed | `Duplicate class files were rejected during scanning` |

## XML parser review

| # | Priority | Finding | Status | Commit |
|---:|:---:|---|---|---|
| 1 | P2 | XML document buffers, included documents, duplicate attributes, and parser exception paths leak memory. | Addressed | `XML parser ownership was made exception-safe` |
| 2 | P2 | Standalone XML includes have no effective cycle or depth protection. | Addressed | `Standalone XML includes were bounded and cycle-checked` |
| 3 | P2 | Top-level XML parsing silently accepts additional roots and trailing non-whitespace content. | Addressed | `Top-level XML structure was validated strictly` |
| 4 | P2 | Single-quoted XML attributes are rejected because character matching consumes mismatches. | Addressed | `Single-quoted XML attributes were parsed correctly` |
| 5 | P2 | XML entity decoding is incomplete and accepts malformed or corrupting numeric entities. | Addressed | `XML entities were decoded and validated correctly` |
| 6 | P3 | Duplicate XML attributes are accepted instead of rejected. | Addressed | `Duplicate XML attributes were rejected` |
| 7 | P2 | XML parser recursion and element nesting are unbounded. | Addressed | `XML parser recursion and nesting were bounded` |
| 8 | P3 | Owning XML objects permit unsafe shallow copying. | Addressed | `Owning XML objects rejected shallow copying` |
| 9 | P3 | `XMLNode::Disconnect()` corrupts sibling or element list invariants. | Addressed | `XML node disconnection preserved list invariants` |
| 10 | P3 | XML errors print directly and lose filenames or include-chain context. | Addressed | `XML errors retained file and include context` |

## Utilities review

| # | Priority | Finding | Status | Commit |
|---:|:---:|---|---|---|
| 1 | P1 | Floating-point parsing accepts malformed doubled signs and can reverse their meaning. | Addressed | `Malformed floating-point signs were rejected` |
| 2 | P2 | Fixed-decimal number formatting corrupts zero-decimal output and can access an empty string. | Addressed | `Fixed-decimal number formatting preserved integer zeros` |
| 3 | P2 | Base64 encoding exposes unsafe raw ownership and unchecked allocation and size failures. | Addressed | `Base64 encoding used safe string ownership` |
| 4 | P2 | The vector stream operator ignores its destination stream and writes to standard output. | Addressed | `Vector formatting respected the destination stream` |
| 5 | P3 | `cut_head()` is publicly declared but has no linkable implementation. | Addressed | `The cut-head utility received its missing implementation` |
| 6 | P3 | Diagnostic attribute printing ignores indentation and mishandles item limits. | Addressed | `Diagnostic attribute printing honored layout limits` |
| 7 | P3 | String delimiter helpers narrow positions to `int` and handle empty delimiters inconsistently. | Addressed | `Delimiter utilities used safe positions and rejected empty delimiters` |
| 8 | P3 | Prime and character-sum checksum helpers can overflow or vary with platform `char` signedness. | Addressed | `Checksum helpers avoided overflow and signed-character drift` |
| 9 | P3 | Obsolete utility APIs are unused, inefficient, or have misleading semantics. | Addressed | `Obsolete utility APIs were removed and string helpers streamlined` |
| 10 | P2/P3 | Utility tests are not integrated with the normal checksum-based kernel suite and cover little behavior. | Addressed | `Utility coverage joined the checksum-based kernel suite` |

## File output improvements

| # | Priority | Task | Status | Verification |
|---:|:---:|---|---|---|
| 1 | P2 | Rate-limit persistent `OutputImage` failures so a bad destination does not repeat expensive encoding and warnings every simulation tick. | Implemented and verified | Debug build; 100-tick persistent-error smoke test produced one warning |
| 2 | P2 | Prevent accidental image-sequence overwrites by adding `OutputFile`-style numbered output directories to `OutputImage`. | Implemented and verified | Debug build; two-run smoke test selected directories `000` and `001` |
| 3 | P2 | Publish completed images atomically so readers cannot observe partially written files. | Implemented and verified | Debug build; JPEG sequence smoke test left only complete destination files and no temporary files |
| 4 | Performance | Buffer complete `OutputFile` records and benchmark the change in Release mode. | Implemented and verified | All 232 kernel tests; Release benchmark improved from 170.68 to 59.47 ns/value (65.2%) |
| 5 | P2/P3 | Avoid loading and parsing an entire existing JSON array when `OutputFile` appends records. | Implemented and verified | Debug build; checksum-backed append test validates and appends one JSON object at a time |
| 6 | Testing | Add `OutputImage` regression coverage for a disabled `WRITE` gate, continuous writes, invalid `start_index`, and transient-error recovery. | Implemented and verified | Debug build; four behaviors covered by checksum-backed kernel tests where setup completes; all 232 kernel tests pass |

## Scalar maths review

| # | Priority | Task | Status | Verification | Commit |
|---:|:---:|---|---|---|---|
| 1 | P1 | Validate Gaussian sampling parameters and handle zero deviation without invoking `std::normal_distribution`. | Addressed | Debug build; focused checksum test; all 243 kernel tests passed | `Gaussian sampling now handles zero and invalid deviations` |
| 2 | P1 | Prevent `OneHotVector` from producing out-of-range or invalid matrix indices. | Addressed | Debug build; finite, non-finite, and empty-output regressions; all 245 kernel tests passed | `OneHotVector indices now remain within output bounds` |
| 3 | P2 | Make `exgaussian()` numerically stable and validate its finite parameter domain. | Addressed | Debug build; central, small-K, tail, amplitude, and invalid-domain tests; all 245 kernel tests passed | `Ex-Gaussian evaluation now remains stable across valid parameters` |
| 4 | P2 | Make angle conversion direct, identity-preserving, and strict about invalid units. | Addressed | Debug build; identity, direct-conversion, and invalid-unit tests; all 245 kernel tests passed | `Angle conversion now preserves identities and validates units` |
| 5 | P2 | Add a reproducible, caller-owned random-generator path for Gaussian sampling. | Addressed | Debug build; generator-state and seeded-module regressions; all 246 kernel tests passed | `Gaussian noise modules now support reproducible seeds` |
| 6 | P3 | Define safe scalar edge-case semantics and remove or replace the unused custom `min()` and `max()` APIs. | Addressed | Debug build; scalar sign and clipping edge-case tests; all 246 kernel tests passed | `Scalar maths helpers now have defined edge-case semantics` |
| 7 | Performance | Cache the standard-normal distribution and benchmark Gaussian sampling in Release mode. | Addressed | Release median improved from 15.309 ns to 11.728 ns per sample (23.4%); Debug build; focused seeded regressions; all 246 kernel tests passed | `Gaussian sampling now reuses distribution state` |
| 8 | Modernization and testing | Optimize `short_angle()`, modernize the public maths API, and add focused checksum-based kernel coverage for the library. | Addressed | Release median improved from 11.638 ns to 1.595 ns per ordinary `short_angle()` call (86.3%); Debug build; focused angle-edge regressions; all 246 kernel tests passed | `Scalar maths API now uses scoped units and faster angle wrapping` |

### Outstanding issues and questions

None.

## Kernel CMake source-list consolidation

| Task | Status | Verification | Commit |
|---|---|---|---|
| Replace the duplicated macOS and Linux kernel source lists with one shared platform-gated list. | Completed | CMake Debug configuration and full build succeeded; all 266 kernel tests passed. | `Kernel platforms now share one source list` |

## Parameter implementation split

| Task | Status | Verification | Commit |
|---|---|---|---|
| Move the `parameter` implementation and its private conversion helpers from `ikaros.cc` into `parameter.cc`. | Completed | Debug build; all 266 kernel tests passed | `Parameter implementation now has its own translation unit` |

### Outstanding issues and questions

None.

## Component implementation split

| Task | Status | Verification | Commit |
|---|---|---|---|
| Move the `Component` implementation and its private helpers from `ikaros.cc` into `component.cc`, retaining the asynchronous runtime snapshot boundary. | Completed | Debug build; all 266 kernel tests passed | `Component implementation now has its own translation unit` |

### Outstanding issues and questions

None.

## Module implementation split

| Task | Status | Verification | Commit |
|---|---|---|---|
| Move every `Module` implementation, including module-specific setup and shape resolution, into `module.cc`. | Completed | Debug build; all 266 kernel tests passed | `Module implementation now has its own translation unit` |

### Outstanding issues and questions

None.

## Final kernel core ownership cleanup

| # | Task | Status | Verification | Commit |
|---:|---|---|---|---|
| 1 | Remove the unused private `Kernel::Save()` implementation and declaration. | Completed | Debug build; all 266 kernel tests passed | `Removed obsolete kernel save stub` |
| 2 | Move kernel session-logging wrappers into `session_logging.cc`. | Completed | Debug build; all 266 kernel tests passed | `Session logging now owns kernel logging wrappers` |
| 3 | Move listing, log-printing, and profiling diagnostics into `kernel_diagnostics.cc`. | Completed | Debug build; all 266 kernel tests passed | `Kernel diagnostics now have their own translation unit` |
| 4 | Move general module-facing read/write path policy into `kernel_paths.cc`. | Completed | Debug build; all 266 kernel tests passed | `Kernel path policy now has its own translation unit` |
| 5 | Move `validate_identifier()` to utilities and replace `new_session_id()` with private `Kernel::NewSessionID()`. | Completed | Debug build; all 266 kernel tests passed | `Kernel helper ownership is now explicit` |

### Constraints

- Preserve behavior, diagnostics, serialized formats, and synchronization semantics.
- Keep each task isolated, fully verified, and independently committed.
- Leave lifecycle, construction, runtime queries, options, notification forwarding, serialization, `Message`, and `kernel()` in `ikaros.cc`.

### Outstanding issues and questions

None.

## Random source follow-ups

| # | Priority | Task | Status | Verification | Commit |
|---:|:---:|---|---|---|---|
| 1 | P2 | Make uniform `Noise` use its module-owned generator so the existing `seed` parameter controls both distributions. | Addressed | Debug build; focused checksum-backed reproducibility regression; all 247 kernel tests passed | `Uniform Noise now uses module-owned random state` |
| 2 | P2 | Replace `Randomizer`'s process-global POSIX random state with a module-owned generator and add a `seed` parameter. | Addressed | Debug build; focused checksum-backed reproducibility regression; updated dependent structural checksum; all 248 kernel tests passed | `Randomizer now uses reproducible module-owned random state` |

### Random-source outstanding issues and questions

- Fixed seeds reproduce sequences within the same C++ standard-library implementation. `std::uniform_real_distribution` does not guarantee bit-identical floating-point sequences across different standard-library implementations.

## Numerical utility modules

| # | Priority | Task | Status | Verification | Commit |
|---:|:---:|---|---|---|---|
| 1 | P1 | Prevent `Softmax` from modifying its shared input buffer. | Addressed | Debug build; checksum-backed input-integrity and probability regression; all 256 kernel tests passed | `Softmax now preserves its input buffer` |
| 2 | P1 | Make zero-denominator `Normalize` modes produce a zero matrix instead of attempting scalar assignment. | Addressed | Debug build; checksum-backed coverage for all four zero-denominator modes; all 257 kernel tests passed | `Normalize now zeroes outputs for zero denominators` |
| 3 | P2 | Make range, Euclidean, and city-block normalization stable for large finite values. | Addressed | Debug build; checksum-backed finite-extreme coverage for all affected modes; all 258 kernel tests passed | `Normalize now remains stable for finite extremes` |
| 4 | P2 | Keep `RegressionStatistics` sample capacity consistent with its fixed startup output shapes. | Addressed | Debug build; checksum-backed runtime shrink/growth attempts and rate-limited warning; all 259 kernel tests passed | `RegressionStatistics now keeps its startup sample capacity` |
| 5 | P2 | Replace numerically fragile normal-equation model comparison with a scale-robust calculation. | Addressed | Debug build; checksum-backed small-scale model comparison with finite statistics and validated degrees of freedom; all 260 kernel tests passed | `Regression model comparisons now remain stable across scales` |
| 6 | P2 | Ensure non-finite `RegressionStatistics` sampling-mask values do not enable sampling. | Addressed | Debug build; checksum-backed NaN and positive/negative infinity mask coverage; all 261 kernel tests passed | `Non-finite regression masks now suppress sampling` |
| 7 | P2 | Define and implement safe `Softmax` behavior for non-finite inputs. | Addressed | Debug build; checksum-backed positive/negative infinity, NaN, warning-rate, and recovery coverage; all 262 kernel tests passed | `Softmax now defines non-finite input behavior` |
| 8 | P2/P3 | Validate `RegressionStatistics` topology during startup and remove runtime shape adaptation. | Addressed | Debug build; startup-failure regressions for incompatible X and SAMPLE inputs; fixed output-shape validation; all 264 kernel tests passed | `RegressionStatistics now validates fixed topology at startup` |

### Numerical utility outstanding issues and questions

- No known high- or medium-priority correctness defect remains from this review.
- `RegressionStatistics` still erases the oldest vector element and recomputes scatter and regression results from retained samples every tick. Benchmark before replacing this with circular storage and incremental sufficient statistics.
- Runtime changes to `RegressionStatistics.labels` remain ignored. A later API decision should either make labels explicitly startup-only or refresh output labels when the parameter changes.
- Non-finite X/Y samples continue to occupy retained-sample capacity while being excluded from regression fits, following the policy chosen for this work.
- The double-precision `Normalize` accumulation path was functionally verified but not benchmarked in Release mode.

## Status meanings

- **Not addressed**: no corrective implementation has been completed.
- **In progress**: implementation or verification is currently underway.
- **Implemented and verified**: implementation and required verification have completed, but the change has not yet been committed.
- **Addressed**: implementation and required verification have completed and the change has been committed.
- **Excluded by decision**: the finding was deliberately left unchanged.

## WebUI dialog follow-ups

| # | Priority | Task | Status | Verification | Commit |
|---:|:---:|---|---|---|---|
| 1 | P2 | Preserve and report the underlying Open-dialog confirmation and callback errors. | Addressed | Dialog regression test; JavaScript syntax check | `Open dialog errors now expose their underlying cause` |
| 2 | P2 | Extend dialog regression coverage to stale Save-to-Open and list-selection races. | Addressed | Three dependency-free dialog race regressions pass | `Dialog tests now cover stale request ordering` |
| 3 | Testing | Register the WebUI dialog regression test with the standard automated test workflow. | Addressed | Standard runner passed all 253 tests, including WebUI dialog regressions | `WebUI dialog regressions now run with kernel tests` |
| 4 | Testing | Run the complete Open-to-Save-As browser scenario when localhost browser access is available. | Addressed | Browser opened the saved user model and saved a copy; server confirmed both operations; no WebUI alert, warning, error, or temporary file remained | `Browser workflow verified Open and Save As dialogs` |
| 5 | Testing | Stress delayed and failed WebUI file-list and save requests, including recovery without reloading. | Addressed | Delayed transport retries, queued saves, `/files` recovery, and 50 HTTP failure/recovery cycles pass; all 254 standard tests pass | `WebUI save failures now have automated recovery coverage` |
| 6 | Testing | Add a self-running live-browser Open-to-Save-As recovery regression with deterministic failure injection. | Addressed | Live BrainStudio test passed with delayed `/files` and Save As failures; both complete files were written and no temporary files remained | `Live WebUI save recovery is now reproducible` |

### WebUI dialog outstanding issues and questions

None.

## Linux CI and portability follow-ups

| # | Priority | Task | Status | Verification | Commit |
|---:|:---:|---|---|---|---|
| 1 | P1 | Diagnose and fix the remaining Linux kernel-test failures. | Implemented and verified | Linux GCC Debug build; Linux kernel tests pass except the three Node.js tests assigned to task 3; all 264 macOS tests pass | `Linux kernel behavior now passes the portable test suite` |
| 2 | Testing | Expand Linux CI from focused smoke tests to the complete kernel test suite. | Implemented and verified | Exact Ubuntu CI command passed all 261 C++/HTTP kernel tests in Debug mode | `Linux CI now runs the complete kernel test suite` |
| 3 | Testing | Install Node.js in Linux CI and run the WebUI JavaScript regression tests. | Implemented and verified | Ubuntu with Node.js passed all 264 kernel and WebUI JavaScript tests | `Linux CI now includes WebUI JavaScript regressions` |
| 4 | Testing | Add a Linux CI WebUI smoke test that starts Ikaros and verifies the main page and logo over HTTP. | Implemented and verified | Ubuntu smoke test launched Ikaros and validated `index.html` plus the PNG signature of `Images/logo.png` | `Linux CI now verifies the WebUI and logo` |
| 5 | Testing | Test Linux builds with both GCC and Clang. | Implemented and verified | Ubuntu Clang 18 build passed all 264 tests and the WebUI smoke test; GCC verification retained | `Linux CI now tests GCC and Clang` |
| 6 | Testing | Add Linux CI coverage for optional dependencies and the modules they enable. | Implemented and verified | Ubuntu 24.04 Release build passed with Dlib, FFmpeg, libusb, PNG, TIFF, and WebP support enabled | `Linux CI now builds optional modules` |
| 7 | Documentation | Update the Linux installation documentation to match the CI-verified dependencies and optional-module support. | Implemented and verified | Instructions match the Ubuntu 24.04 GCC/Clang and optional-module CI package sets and commands | `Linux installation documentation now matches CI` |

### Outstanding issues and questions

- Dynamixel support and hardware-dependent module behavior are not exercised by hosted Linux CI.
- Optional modules are compile-tested, but their hardware and runtime behavior is not covered.
- The GitHub wiki is stored separately from this repository. Its Linux section should be replaced with or linked to `docs/LINUX.md` so it does not drift from the CI-tested instructions.

## WebUI Chrome compatibility fixes

| # | Priority | Task | Status | Verification | Commit |
|---:|:---:|---|---|---|---|
| 1 | P1 | Let Three.js select and own the Canvas 3D WebGL context so Chromium does not reject a conflicting context type. | Implemented and verified | JavaScript syntax check; Chromium gallery rendered without WebGL context or shader errors | `Canvas 3D now initializes with Chromium WebGL` |
| 2 | P2 | Replace the obsolete, unclosed loading-screen `blink` element with valid HTML. | Implemented and verified | Source markup check; Chromium DOM contains no `blink` and preserves loading-panel siblings | `WebUI loading status now uses valid markup` |
| 3 | P2 | Correct the malformed component-inspector toolbar closing tag. | Implemented and verified | Source markup check; Chromium preserves component and system inspector controls as sibling `div` elements | `WebUI inspector toolbar now uses valid markup` |
| 4 | P2 | Correct the WebUI button template's malformed `type` and `class` attributes. | Implemented and verified | JavaScript syntax check; Chromium parses widget buttons with `type="button"` and no stray attributes | `WebUI widget buttons now use valid attributes` |

### WebUI Chrome compatibility outstanding issues and questions

None.

## Major code cleanup

| # | Task | Status | Verification | Commit |
|---:|---|---|---|---|
| 1 | Check that no locale-sensitive code remains and replace it with locale-free code. | Addressed | Debug build; hostile-global-locale regression; WebUI syntax check; all 264 standard tests passed | `Ikaros text handling is now locale-independent` |
| 2 | Audit error handling so startup and `Init()` failures use exceptions, `Tick()` problems use `Notify()` or related functions, exceptions propagate as high as suitable, and WebUI messages identify the involved module and buffer where applicable while including enough call-chain context to locate the failure. | Addressed | Debug build; focused startup, synchronous, asynchronous, and delayed-propagation lifecycle regressions; all 265 standard tests passed; remaining `EpiServos` and `ServoControlTuning` console diagnostics explicitly deferred | `Runtime diagnostics now preserve lifecycle and component context` |
| 3 | Audit header dependencies: remove unnecessary includes while making files include what they directly use instead of relying on transitive includes in most cases. | Addressed | All project headers compile standalone; strict clangd missing/unused-include diagnostics enabled; Debug build; all 265 standard tests passed | `Header dependencies are now explicit and leaner` |
| 4 | Find C-style idioms that can be replaced with C++ idioms; implement straightforward replacements and retain a discussion list of cases requiring judgment for review after all tasks are complete. | Addressed | Debug build; mechanical cast, null, constant, buffer-length, initialization, and type-alias cleanup; all 265 standard tests passed | `Straightforward C idioms now use modern C++ forms` |
| 5 | Audit path and include-name capitalization for code that works on commonly case-insensitive macOS filesystems but can fail on case-sensitive Linux filesystems. | Addressed | Exact-case audit found no mismatched quoted includes or case-colliding tracked paths; reusable checker added; Debug build passed | `Path capitalization now has a regression check` |
| 6 | Review all Markdown files for current and accurate content, and place each document in the appropriate repository location. | Addressed | All 547 Markdown files inventoried; maintained docs checked for content type, portable paths, module titles, and `.ikc` interface agreement; stale pages corrected; misplaced files removed or relocated | `Markdown documentation now matches the current tree` |
| 7 | Audit string escaping and UTF-8 handling across parsing, serialization, messages, paths, and WebUI boundaries; correct unsafe or inconsistent behavior and verify whether handling is sound throughout. | Addressed | JSON, XML, URL, filesystem/request, notification, DOM, and Markdown-rendering boundaries audited; malformed UTF-8, unsafe URL bytes/schemes, XML names/control bytes, and HTML insertion covered; Debug build; all 266 standard tests passed | `Text boundaries now validate UTF-8 and escape by context` |

### Major code cleanup deferred items

- Task 2: Remaining direct console diagnostics in the legacy `EpiServos` and `ServoControlTuning` modules were explicitly deferred for later review.

### C++ modernization cases for discussion

- The XML implementation still owns a linked object tree and copied C strings manually. Converting it to `std::string` and smart pointers would improve ownership clarity, but changes parser object identity and requires a focused API redesign.
- Codec, FFmpeg, CoreAudio, POSIX spawn, socket, and shared-memory boundaries retain C buffers, casts, and memory operations where those types are imposed by the external API or preserve contiguous fast paths.
- The `INSTALL_CLASS` registration macro and build-feature macros remain because they perform preprocessing or conditional compilation that `constexpr` cannot replace directly.
- The deferred `EpiServos` and `ServoControlTuning` modules contain many macro constants and a C-style aggregate that should be modernized together with their later diagnostic cleanup.

### Major code cleanup outstanding issues and questions

- `EpiServos` and `ServoControlTuning` remain deferred by request. Their direct console diagnostics and C-style constants/aggregate should be handled together in a later task.
- The XML parser's manual linked-tree and C-string ownership, external C API buffer boundaries, and class-registration macro are the judgment-heavy modernization cases listed above; no change was made without an API or architecture decision.
- No other known locale, lifecycle-diagnostic, include, capitalization, Markdown, escaping, or UTF-8 correctness issue remains from this audit.

## Kernel refactoring plan

Each task is executed sequentially. Before implementation, its status changes to **In progress**; after focused and full verification, it is committed independently before the next task begins.

| # | Task | Status | Verification | Commit |
|---:|---|---|---|---|
| 1 | Split the self-contained circular-buffer, connection, class, and request implementations out of `ikaros.cc`; later subsystem moves remain paired with their corresponding refactoring steps. | Implemented and verified | Debug build; all 266 kernel and WebUI tests passed | `Kernel support types now have cohesive implementation units` |
| 2 | Consolidate the five scalar-state `Component::Bind()` implementations behind one type-safe internal helper while retaining the public overloads. | Implemented and verified | Debug build; duplicate-binding diagnostic regression; all 266 kernel and WebUI tests passed | `Scalar state binding now shares type-safe validation` |
| 3 | Decompose `Component::ResolveParameter()` into value-source, expression/type resolution, and matrix-shaping helpers without changing diagnostic context. | Implemented and verified | Debug build; inherited/default/matrix parameter coverage; all 266 kernel and WebUI tests passed | `Parameter resolution now has explicit processing stages` |
| 4 | Separate state capture and restoration from state-file I/O while preserving the `ikaros-state-v1` format and scoped remapping. | Implemented and verified | Debug build; complete matrix/scalar/scoped/WebUI state coverage; all 266 kernel and WebUI tests passed | `State persistence now separates representation from file I/O` |
| 5 | Decompose flattened, stacked, simple, dynamic, and indexed input-shape resolution after adding characterization coverage for existing behavior. | Implemented and verified | Debug build; fixed/flattened/stacked/delayed/dynamic/labelled/reverse-step coverage; all 266 tests passed; Release 100-process setup benchmark improved from 9.50 s to 9.46 s | `Input shape resolution now uses mode-specific helpers` |
| 6 | Extract buffer-size convergence and startup-step propagation into independently testable setup algorithms without changing setup order or semantics. | Implemented and verified | Debug build; checksum-backed cascaded, looped, delayed, dynamic, and startup-step coverage; all 266 tests passed; Release setup benchmark remained 9.46 s over 100 runs | `Kernel setup calculations are now independently testable` |
| 7 | Separate task submission, watchdog waiting, completion barriers, and failure collection while preserving exception and notification behavior. | Implemented and verified | Debug build; watchdog, barrier, task-error, and thread-pool coverage; all 266 tests passed; Release delayed propagation improved from 65.18 to 62.85 ns/connection | `Task execution now separates dispatch, barriers, and failures` |
| 8 | Extract realtime waiting, lag, catch-up, resynchronization, and warning policy from the kernel run-state loop. | Implemented and verified | Debug build; realtime/play/pause and timing coverage; all 266 tests passed; Release delayed propagation improved from 62.85 to 60.35 ns/connection | `Realtime timing policy is now separate from the run loop` |
| 9 | Decompose WebUI subscription management, snapshot policy, value serialization, publication, and data-response construction. | Implemented and verified | Debug build; subscription, rate-limit, image-refresh, and first-client snapshot coverage; all 266 tests passed; Release five-test WebUI snapshot median improved from 7.01 s to 6.19 s | `WebUI data snapshots now have explicit processing stages` |
| 10 | Replace the authenticated endpoint dispatch chain with a small explicit route table while keeping authentication, public-file handling, aliases, and static-file fallback clear. | Implemented and verified | Debug build; authentication, public-file, command alias, data endpoint, and static-file coverage; all 266 tests passed | `WebUI endpoint dispatch now uses explicit routes` |

### Refactoring-wide constraints

- Preserve public behavior and file formats unless a separately approved defect is discovered.
- Keep setup and `Init()` exception behavior distinct from runtime `Tick()` notification behavior.
- Preserve component and value paths, plus useful call-chain context, in errors sent to the WebUI.
- Add characterization tests before restructuring compatibility-sensitive algorithms.
- Run the Debug build and complete kernel suite for every step; run the smallest focused tests first.
- Run Release benchmarks before and after changes to matrix shape resolution, scheduling, runtime timing, or WebUI snapshot work where the affected path is performance-sensitive.
- Do not begin a later task until the preceding task is verified and committed.

### Planned commit boundaries

1. `Kernel support types now have cohesive implementation units`
2. `Scalar state binding now shares type-safe validation`
3. `Parameter resolution now has explicit processing stages`
4. `State persistence now separates representation from file I/O`
5. `Input shape resolution now uses mode-specific helpers`
6. `Kernel setup calculations are now independently testable`
7. `Task execution now separates dispatch, barriers, and failures`
8. `Realtime timing policy is now separate from the run loop`
9. `WebUI data snapshots now have explicit processing stages`
10. `WebUI endpoint dispatch now uses explicit routes`

### Kernel refactoring outstanding issues and questions

None.

## Physical kernel implementation split

Each task moves existing definitions without intentional behavioral changes. Tasks are executed sequentially, verified with a Debug build and the complete kernel test suite, and committed independently.

| # | Task | Status | Verification | Commit |
|---:|---|---|---|---|
| 1 | Move model construction, buffer sizing, input-shape resolution, and startup-step implementation into `kernel_setup.cc`. | Implemented and verified | Debug build; complete setup, shape, startup-step, and WebUI recovery coverage; all 266 tests passed | `Kernel setup implementation now has its own translation unit` |
| 2 | Move task execution, propagation, the run loop, and realtime timing implementation into `kernel_execution.cc`. | Implemented and verified | Debug build; task, watchdog, propagation, async, run-mode, and realtime coverage; all 266 tests passed | `Kernel execution now has its own translation unit` |
| 3 | Move state capture, restoration, save, load, and reset implementation into `kernel_state.cc`. | Implemented and verified | Debug build; matrix, scalar, scoped, reset, file-format, and WebUI state coverage; all 266 tests passed | `Kernel state persistence now has its own translation unit` |
| 4 | Move WebUI subscriptions, snapshots, value serialization, and data-response construction into `kernel_webui.cc`. | Implemented and verified | Debug build; subscription, snapshot timing, image refresh, logging, serialization, and response coverage; all 266 tests passed | `Kernel WebUI data handling now has its own translation unit` |
| 5 | Move HTTP parsing and dispatch, authentication, file serving, and endpoint handlers into `kernel_http.cc`. | Implemented and verified | Debug build; authentication, public/static files, save/load endpoints, routing aliases, controls, and HTTP lifecycle coverage; all 266 tests passed | `Kernel HTTP handling now has its own translation unit` |
| 6 | Review the remaining `ikaros.cc`, move only clearly misplaced cohesive definitions, update build registration, and document the resulting implementation boundaries. | Implemented and verified | Debug build; class discovery, setup orchestration, delayed buffers, run modes, checksums, startup reporting, and image serialization coverage; all 266 tests passed; implementation map added to `Source/Kernel/README.md` | `Kernel implementation boundaries are now documented and complete` |

### Physical split constraints

- Preserve behavior, public interfaces, file formats, diagnostics, and synchronization semantics.
- Do not introduce subsystem classes or redesign shared `Kernel` state as part of this split.
- Keep small helpers with the subsystem that owns them; avoid one-function files.
- Keep headers self-contained and register every new implementation unit explicitly in CMake.
- Do not begin a later task until the preceding task is verified and committed.

### Physical kernel split outstanding issues and questions

- A few small file-local policy helpers are duplicated where setup, execution, and HTTP paths require the same legacy behavior. Consolidating them would require introducing a private shared kernel-support interface; no such abstraction was added during this behavior-preserving split.
- `parameter`, `Component`, and `Module` implementations remain together in `ikaros.cc` because they share its core conversion, binding, and runtime-context helpers. Splitting those types cleanly would be a separate internal-API refactoring rather than a physical move.

## Kernel helper ownership cleanup

These tasks follow the preferred helper restructuring identified after the physical split. They are executed sequentially, verified with a Debug build and all kernel tests, and committed independently.

| # | Task | Status | Verification | Commit |
|---:|---|---|---|---|
| 1 | Replace duplicated `resolve_state_filename` helpers with a private `Kernel::ResolveStateFilename()` member. | Completed | Debug build; all 266 kernel tests passed | `Kernel now owns state filename resolution` |
| 2 | Consolidate built-in `log_level`, `module_start`, `start_tick`, and `async` parameter metadata construction under `Component`. | Completed | Debug build; all 266 kernel tests passed | `Component now owns built-in parameter metadata` |
| 3 | Add `dictionary::ensure_list()` and replace the duplicated kernel helpers. | Completed | Debug build; all 266 kernel tests passed | `Dictionary now normalizes list members` |
| 4 | Add matrix-owned shape formatting and replace duplicated `format_shape` helpers. | Completed | Debug build; all 266 kernel tests passed | `Matrix now owns diagnostic shape formatting` |
| 5 | Consolidate strict parameter and scalar-state numeric parsing in a shared private kernel utility, and remove obsolete scalar/setup parser copies from `ikaros.cc`. | Completed | Debug build; all 266 kernel tests passed | `Kernel parsing helpers are now shared internally` |
| 6 | Move `.size` to `.shape` alias canonicalization into the XML/model-serialization layer and remove the duplicated kernel helpers. | Completed | Debug build; all 266 kernel tests passed | `Model XML serialization now canonicalizes shape aliases` |

### Helper ownership constraints

- Preserve public behavior, diagnostics, serialized formats, and checksum results.
- Prefer an existing owning class over a new abstraction; keep shared utilities private when no public API is justified.
- Do not expose implementation-only helpers through the public API solely to share code between translation units.
- Do not begin a later task until the preceding task is verified and committed.

### Outstanding issues and questions

None.
## Secure Python interpreter selection

| # | Task | Status | Verification | Commit |
|---:|---|---|---|---|
| 1 | Remove model-controlled `python_executable` selection and restore command-line-only interpreter selection in `PythonModule`. | Completed | Full build; XML validation; explicit `-p` matcher startup; all 272 kernel tests; source inspection confirms only the command-line option remains. | `Python interpreter selection is trusted again` |
| 2 | Configure the local VS Code launch command to pass the trusted matcher Python interpreter with `-p`. | Completed | JSON validation; interpreter executable check; VS Code-equivalent argument sequence initialized ALIKED and LightGlue on MPS. | Local `.vscode/launch.json` is git-ignored. |

### Outstanding issues and questions

- The VS Code launch configuration is intentionally local because `.vscode` is git-ignored.
## Native ElasticTemplateMatcher pipeline

The implementation is C++ only at runtime, targets macOS Apple Silicon, uses the already-installed
ONNX Runtime, does not use OpenCV, and does not modify the kernel. Existing `ikaros::matrix`
functionality must be evaluated before adding any new data structure or numerical helper.

| # | Task | Status | Verification | Commit |
|---:|---|---|---|---|
| 1 | Validate Homebrew ONNX Runtime C++ linkage and externally convert checksum-pinned ALIKED and static LightGlue models; prove C++ inference parity on frozen fixtures. | Completed | Homebrew CMake target compiled and ran with ONNX Runtime 1.28.0; ONNX checker passed; native/PyTorch parity passed for ALIKED and dynamic LightGlue with identical match indices and maximum float error `1.53e-5`. | `Native matcher models now have a verified artifact contract` |
| 2 | Add optional Apple-Silicon ONNX Runtime CMake discovery and a module-local, security-hardened inference helper. | Completed | CMake discovered Homebrew ONNX Runtime 1.28.0 without a fixed installation path; full Ikaros build passed; helper enforces regular `.onnx` files, SHA-256, exact tensor names/types/ranks, and fixed inference thread limits. | `Native matcher inference now uses a verified ONNX boundary` |
| 3 | Implement the C++ ALIKED module with dynamic feature matrices and no count outputs. | Completed | Full Ikaros build passed; native three-tick image smoke emitted 273 thresholded features with leading scores matching PyTorch; outputs retained setup-owned capacities of 512x2, 512x128, and 512x1. | `ALIKED features now flow through native dynamic matrices` |
| 4 | Implement the C++ TemplateFeatureBank module with flattened dynamic template matrices. | Completed | Full Ikaros build passed; live ALIKED pipeline smoke learned 61 central features at range 0,61 and appended a second template at 61,61 while retaining predeclared capacities; no count output used. | `Learned templates now persist in bounded matrix banks` |
| 5 | Implement the C++ LightGlue module with dynamic correspondence rows. | Completed | Full Ikaros build passed; live same-image pipeline smoke recovered all 61 learned features as correspondence rows with valid template/current indices and high scores; optional execution gate verified by code path. | `LightGlue correspondences now run in native C++` |
| 6 | Implement reusable native similarity and homography estimation using `ikaros::matrix`/LAPACK facilities before adding local numerical helpers. | Completed | Full build passed; end-to-end deterministic RANSAC retained 61/61 inliers and refined a same-image homography to `1.59e-5` pixel mean error; normalized DLT uses matrix transpose, matmul, SVD, and inverse. | `Native geometry now robustly verifies feature matches` |
| 7 | Implement reusable native polygon geometry and projective transformation. | Completed | Full build and end-to-end smoke passed; verified identity homography produced a finite convex closed five-point centered path and matched-feature boxes, with bounds and area validation. | `Verified transforms now produce validated polygon paths` |
| 8 | Implement native pyramidal Lucas–Kanade tracking, keeping image pyramids internal to avoid artificial module boundaries. | Completed | Full build and end-to-end repeated-image smoke passed; all 61 seeded points remained tracked with zero forward-backward error and a valid similarity transform. | `Verified features now continue through native LK tracking` |
| 9 | Implement the C++ tracking controller and assemble the dynamic-matrix `.ikg` pipeline. | Completed | Full build, XML validation, deterministic 20-tick pipeline smoke, and one-tick native camera demo smoke passed; status transitioned detection-to-tracking with 61 supports and confidence 1. | `Native modules now form a tracked template pipeline` |
| 10 | Add security, deterministic, dynamic-shape, failure/reacquisition, WebUI, and Release performance verification. | Completed | Release build and 20-tick gated learned-feature/tracking smoke passed in 2.86 seconds; deterministic geometry and controller tests passed; corrupt model checksum was rejected; live WebUI camera, controls, labeled tables, and non-overlapping layout were verified without browser warnings; kernel regression suite passed. | `Native matcher verification now covers security and reacquisition` |
| 11 | Remove the Python prototype/runtime setup after native parity is established and complete documentation and migration. | Completed | Removed the Python class, descriptor, installer, downloader, requirements, bytecode cache, and obsolete manual model; native-only documentation and checksum instructions added; Release configure found ONNX Runtime and full build passed; XML, source-reference audit, deterministic geometry/controller tests, and `git diff --check` passed. | `Removed the Python template-matching runtime` |

### Approved decisions

- ONNX Runtime is approved and already installed.
- One-time Python model conversion outside the Ikaros runtime is approved.
- Initial capacities: 320x240 input, 512 current keypoints, 16 templates, 8192 stored features, and 512 matches per template.
- No OpenCV, no Python runtime, no kernel changes, and no automatic downloads or installations.

### Outstanding issues and questions

None.
