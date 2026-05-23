# Ikaros 2012: Public Repository Launch and Build-System Groundwork

**Year:** 2012  
**Status:** Draft for review  
<!-- **Suggested visual:** Screenshot of the early HTML WebUI showing a simple module network or viewer. -->

## Year in Brief

2012 is the year Ikaros became a public Git repository with a broad existing codebase already in place. The initial import contained the kernel, examples, WebUI assets, platform support, brain models, utility modules, vision modules, learning modules, robot modules, I/O modules, and user-module templates. The year is not the beginning of Ikaros as a system; it is the beginning of Ikaros as a public, versioned development effort. That distinction matters because the work reads less like invention from scratch and more like the careful opening of a mature research platform to repeatable collaboration, build automation, and external review. It also establishes the baseline for the public history: Ikaros enters Git as a large, multi-platform research system whose immediate work is organization, exposure, and buildability.

## Development Notes

The first phase of the year established repository hygiene and a cross-platform build direction. Work recorded under `ikaros-project` brought in the initial source tree and then refined README content, naming, compilation casts, and startup wording. Birger Johansson handled much of the early repository cleanup and CMake work, including `.gitignore` rules, `DelayOne` fixes, temporary-file cleanup, Linux CMake support, and removal of older project and make-script files. The week-by-week pattern shows that the first public task was not adding one marquee feature, but making the imported tree legible and buildable enough for public collaboration. This kind of foundational cleanup is easy to underestimate, but it determines whether new module work can proceed as normal development rather than repository archaeology.

The next phase turned module registration and initialization into a more maintainable mechanism. Static initialization entered the kernel, the old central module headers and explicit initialization functions were removed, and the build system gained module-level include and link support. YARP support moved from testing into the `YARPPort` module, while QT-related code was migrated into the newer module build structure. This reduced friction for module authors, because adding or moving modules no longer required the same level of manual central coordination. The change also made the module tree easier to reason about because registration behavior moved closer to the module code itself.

The strongest product-facing change was the new HTML WebUI. The interface was reorganized, icons and stylesheets were moved into cleaner paths, controls were fixed, canvas objects were introduced, image loading and profiling were added, and path/inspector behavior began to settle. In parallel, `VideoInputFile` based on FFMpeg gave the system a practical video-file input path. The WebUI changes are especially important because they made Ikaros visible as an interactive system: networks, data, images, and controls could increasingly be inspected in the browser rather than inferred from logs alone. That shift made the browser a central part of the developer workflow from the first public year, not an optional display layer added after the core system was complete.

Late 2012 expanded the hardware and sensing surface. `SaliencePoints`, Kinect, Dynamixel replacement work, the LUCS robotic arm, Phidgets, `MPIFaceDetector`, MarkerTracker updates, and ARToolKitPlus-related build support all entered the history. Christian Balkenius also moved and extended trainer work, while Birger Johansson refined servo configuration and YARP behavior. These additions gave the young public repository a practical personality: Ikaros was not only a kernel and module library, but also a platform for connecting models to cameras, robots, servos, sensors, and learning experiments. The range of modules also shows the project’s ambition early: perception, action, learning, and interface code were all expected to coexist in one extensible system.

## Milestones

- Public source import with license and README.
- CMake build structure across the source tree.
- Static module registration and new initialization flow.
- HTML WebUI and canvas-based visual objects.
- FFMpeg-based video input.
- Early Kinect, Dynamixel, Phidgets, robotic arm, and face-detection work.
## Weekly Entries

| Week | Date range | Title |
| --- | --- | --- |
| 2012-W38 | 2012-09-17 to 2012-09-23 | [Repository import and build-system groundwork](2012-09-17-week-01.md) |
| 2012-W39 | 2012-09-24 to 2012-09-30 | [Static module registration](2012-09-24-week-02.md) |
| 2012-W41 | 2012-10-08 to 2012-10-14 | [Module initialization and YARP build integration](2012-10-08-week-03.md) |
| 2012-W42 | 2012-10-15 to 2012-10-21 | [HTML WebUI and video input](2012-10-15-week-04.md) |
| 2012-W43 | 2012-10-22 to 2012-10-28 | [Salience, CMake, and Kinect](2012-10-22-week-05.md) |
| 2012-W44 | 2012-10-29 to 2012-11-04 | [Robotics, face detection, and Phidgets](2012-10-29-week-06.md) |
| 2012-W45 | 2012-11-05 to 2012-11-11 | [Trainer work and servo configuration](2012-11-05-week-07.md) |
| 2012-W48 | 2012-11-26 to 2012-12-02 | [YARP configuration tightening](2012-11-26-week-08.md) |
| 2012-W49 | 2012-12-03 to 2012-12-09 | [PolarPlot and interaction updates](2012-12-03-week-09.md) |
| 2012-W51 | 2012-12-17 to 2012-12-23 | [Robotic arm documentation](2012-12-17-week-10.md) |
