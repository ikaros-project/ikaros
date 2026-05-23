# Ikaros 2014: Depth Processing, WebUI Resilience, and Module Growth

**Year:** 2014  
**Status:** Draft for review  
<!-- **Suggested visual:** Kinect or `DepthBlobList` visualization. -->

## Year in Brief

2014 broadens Ikaros across depth processing, homogeneous matrix math, robot simulation, audio output, spline and converter modules, Kinect work, local approximation, and face detection. The year has several distinct arcs: early kernel/WebUI and math fixes, spring module additions, summer depth processing, autumn approximator and robotics cleanup, and a late-year face-detection and timing push. The year is more exploratory than 2013, with several new module families entering the tree and existing runtime pieces being adapted to support them. The result is a year that feels like a catalogue of capabilities being opened: visual simulation, audio output, geometry, depth, approximation, and face processing.

## Development Notes

The year opened with fixes to the Constant module, binding code, GL robot simulation, and WebUI persistence. Work recorded under `ikaros-project` added modules such as Maxima, Submatrix, FASTDetector, and direct robot control. Birger fixed CMake typos, while the GL simulator gained target visualization, sliding movement, HUD direction, and direct control. These changes continued the previous year’s GL work by turning simulation into something controllable and inspectable, not just renderable. The simulator work also made robotics development less dependent on immediate access to physical devices.

Math infrastructure grew through homogeneous matrix functions, rotation matrices, Euler-angle corrections, validity checks, and additional matrix operations. Birger Johansson and Christian Balkenius contributed to marker tracking and coordinate-system work, while WebUI inspection improved through data views and exception handling around incorrect objects. The math additions provided the coordinate and transformation vocabulary needed by marker tracking, robotics, depth sensing, and visualization modules. Those math functions are the connective tissue between perception and action, turning raw coordinates into transformations that modules can share.

Spring added `LinearSplines`, `SoundOutput`, sign functions, and bug fixes. Birger Johansson added `RotationConverter`, `ArrayToMatrix`, SoundOutput CMake support, and atan2 fixes. Christian Balkenius and `ikaros-project` continued SoundOutput and spline work into a usable state. This spring work broadened the range of signals Ikaros could represent, moving from geometry and matrices into sound, splines, and data conversion. Together these modules made Ikaros more useful for experiments that need continuous values, generated sound, or format conversion rather than only image-like matrices.

The summer and early autumn centered on depth and Kinect processing. `DepthBlobList`, `DepthPointList`, grid processing, Xtion/Kinect multi-sensor support, and a working depth-blob branch moved the vision stack deeper into sensor-derived data. Later, `DepthTransform`, `LocalApproximator`, Kinect calibration parameters, and restored linear algebra functions extended the same direction. The depth work gave the repository a stronger connection to embodied perception, where inputs are spatial, noisy, and tied to physical sensors rather than static examples. It also linked Ikaros to the broader RGB-D camera wave of the period, when Kinect-style sensors were becoming common experimental tools.

The year closed with module and documentation additions: Arbiter variable inputs, Fuse, message-splitting fixes, `TappedDelayLine`, `SpectralTiming`, MarkerTracker coordinate outputs, `CIFaceDetector`, `AttentionWindow`, rectangle drawing, SVD comments, and OutputJPEG fixes. This late-year cluster shows the project widening from infrastructure into reusable perceptual components and temporal processing modules. The year therefore ends with a much richer module shelf than it began with, especially for perception and time-based processing.

## Milestones

- GLRobotSim and direct robot-control refinements.
- Homogeneous matrix and rotation math expansion.
- SoundOutput, LinearSplines, RotationConverter, and ArrayToMatrix.
- Kinect/Xtion and depth-blob processing.
- DepthTransform and LocalApproximator.
- CIFaceDetector and AttentionWindow.
- TappedDelayLine and SpectralTiming.
## Weekly Entries

| Week | Date range | Title |
| --- | --- | --- |
| 2014-W06 | 2014-02-03 to 2014-02-09 | [Fixed Constant data bug](2014-02-03-week-01.md) |
| 2014-W08 | 2014-02-17 to 2014-02-23 | [Robotics and Build Updates](2014-02-17-week-02.md) |
| 2014-W09 | 2014-02-24 to 2014-03-02 | [WebUI and Math Updates](2014-02-24-week-03.md) |
| 2014-W10 | 2014-03-03 to 2014-03-09 | [Additional homogenous matrix functions added](2014-03-03-week-04.md) |
| 2014-W11 | 2014-03-10 to 2014-03-16 | [Math Updates](2014-03-10-week-05.md) |
| 2014-W12 | 2014-03-17 to 2014-03-23 | [Math and Docs Updates](2014-03-17-week-06.md) |
| 2014-W13 | 2014-03-24 to 2014-03-30 | [Math Updates](2014-03-24-week-07.md) |
| 2014-W15 | 2014-04-07 to 2014-04-13 | [H_multiply_v bug fix](2014-04-07-week-08.md) |
| 2014-W16 | 2014-04-14 to 2014-04-20 | [Math and Docs Updates](2014-04-14-week-09.md) |
| 2014-W17 | 2014-04-21 to 2014-04-27 | [Math and WebUI Updates](2014-04-21-week-10.md) |
| 2014-W18 | 2014-04-28 to 2014-05-04 | [Audio and Math Updates](2014-04-28-week-11.md) |
| 2014-W19 | 2014-05-05 to 2014-05-11 | [Audio Updates](2014-05-05-week-12.md) |
| 2014-W23 | 2014-06-02 to 2014-06-08 | [Math and Build Updates](2014-06-02-week-13.md) |
| 2014-W32 | 2014-08-04 to 2014-08-10 | [DepthBlobList](2014-08-04-week-14.md) |
| 2014-W33 | 2014-08-11 to 2014-08-17 | [Vision and Logging Updates](2014-08-11-week-15.md) |
| 2014-W34 | 2014-08-18 to 2014-08-24 | [Vision Updates](2014-08-18-week-16.md) |
| 2014-W35 | 2014-08-25 to 2014-08-31 | [Vision and Security Updates](2014-08-25-week-17.md) |
| 2014-W36 | 2014-09-01 to 2014-09-07 | [Vision Updates](2014-09-01-week-18.md) |
| 2014-W37 | 2014-09-08 to 2014-09-14 | [Vision and Tests Updates](2014-09-08-week-19.md) |
| 2014-W38 | 2014-09-15 to 2014-09-21 | [Build and Logging Updates](2014-09-15-week-20.md) |
| 2014-W39 | 2014-09-22 to 2014-09-28 | [Vision and Math Updates](2014-09-22-week-21.md) |
| 2014-W41 | 2014-10-06 to 2014-10-12 | [Docs and Vision Updates](2014-10-06-week-22.md) |
| 2014-W44 | 2014-10-27 to 2014-11-02 | [WebUI and Kernel Updates](2014-10-27-week-23.md) |
| 2014-W45 | 2014-11-03 to 2014-11-09 | [Arbiter - now with variable number of inputs](2014-11-03-week-24.md) |
| 2014-W46 | 2014-11-10 to 2014-11-16 | [Weekly Development Updates](2014-11-10-week-25.md) |
| 2014-W47 | 2014-11-17 to 2014-11-23 | [Fixed multiple message split](2014-11-17-week-26.md) |
| 2014-W51 | 2014-12-15 to 2014-12-21 | [Docs and Audio Updates](2014-12-15-week-27.md) |
| 2014-W52 | 2014-12-22 to 2014-12-28 | [Vision and Robotics Updates](2014-12-22-week-28.md) |
