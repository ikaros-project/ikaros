# Ikaros 2013: Filters, Windows Support, and WebUI Stability

**Year:** 2013  
**Status:** Draft for review  
<!-- **Suggested visual:** Screenshot of `GLRobotSimulator`. -->

## Year in Brief

2013 is a year of targeted maintenance followed by a concentrated autumn development run. The first half contains small but useful improvements to casts, WebUI polling, Dynamixel examples, depth processing, XML buffer size, and YARP cleanup. The second half adds Windows/Visual Studio support, GL robot simulation, XML hardening, image filters, ANN trainer completion, and WebUI reconnect behavior. The year therefore has a clear shape: small corrective work in winter and spring, platform expansion in summer, and a substantial technical consolidation in the autumn. That structure makes the year a bridge between the public-launch work of 2012 and the broader module growth visible by the end of 2013.

## Development Notes

The early months kept the young public repository moving through practical fixes. Birger added type-cast cleanup, reduced browser polling in fast-forward run mode, and refreshed Dynamixel examples so the hardware module was easier to run. Work recorded under `ikaros-project` began depth processing, extending the Kinect and sensor-processing line that started in 2012. Birger Johansson increased XML parser headroom and cleaned up YARP naming and CMake handling. Those early commits are modest individually, but together they show the project settling the rough edges left by the 2012 public launch. They also show a healthy habit in the young repository: examples, polling behavior, and parser limits were corrected before they became long-term obstacles.

Midyear, Stefan Winberg added Visual Studio 2012 support and updated ignore rules for the Windows toolchain. Christian Balkenius later cleaned up auxiliary Windows files, corrected the Windows version, and merged the Windows updates. This made the repository's stated cross-platform ambition more concrete. This work also reduced the gap between source availability and actual adoption, because Windows developers needed project files and ignore rules that matched their tools. The Windows work gave Ikaros another credible development route, which mattered for a project that already carried platform-specific kernel and I/O support.

The autumn work is the center of the year. `GLRobotSimulator` entered the tree, including GL CMake support and a namespace-collision fix. XML parsing became more robust through automatic buffer growth, duplicate-attribute handling, entity handling, and parser fixes. At the same time, image processing expanded with `IntegralImage`, `GaussianFilter`, `DoG`, `LoG`, `MedianFilter`, and `ZeroCrossings`. The autumn sequence is where the codebase becomes visibly broader: rendering, parsing, filtering, and learning all advance within a few active weeks. Because these changes landed close together, 2013 has a strong technical finish despite the quiet first half.

The ANN branch also matured. Trainer work moved from minimal completion to a finished and merged path, with row mean and expanded average support landing alongside it. The year closed with the WebUI becoming more resilient: automatic restart, reconnect work, restored views, persistent view state, and explicit `int` and `float` dictionary types. Persistent WebUI behavior is a turning point because it treats the browser state as part of the working environment rather than a disposable view. It also makes reconnects, saved views, and editing state part of the WebUI vocabulary by the end of the year.

## Milestones

- Visual Studio 2012 support and Windows cleanup.
- GL robot simulator added and integrated with CMake.
- XML parser hardening and entity handling.
- Major image-filter expansion.
- ANN trainer completion and RBF first commit.
- WebUI reconnect and persistent view state.
## Weekly Entries

| Week | Date range | Title |
| --- | --- | --- |
| 2013-W05 | 2013-01-28 to 2013-02-03 | [Static cast cleanup](2013-01-28-week-01.md) |
| 2013-W06 | 2013-02-04 to 2013-02-10 | [WebUI run-mode polling](2013-02-04-week-02.md) |
| 2013-W09 | 2013-02-25 to 2013-03-03 | [Dynamixel example refresh](2013-02-25-week-03.md) |
| 2013-W10 | 2013-03-04 to 2013-03-10 | [Depth processing begins](2013-03-04-week-04.md) |
| 2013-W12 | 2013-03-18 to 2013-03-24 | [XML buffer increase](2013-03-18-week-05.md) |
| 2013-W14 | 2013-04-01 to 2013-04-07 | [YARP and cleanup fixes](2013-04-01-week-06.md) |
| 2013-W25 | 2013-06-17 to 2013-06-23 | [Visual Studio 2012 support](2013-06-17-week-07.md) |
| 2013-W36 | 2013-09-02 to 2013-09-08 | [Windows tree cleanup](2013-09-02-week-08.md) |
| 2013-W38 | 2013-09-16 to 2013-09-22 | [GL robot simulator](2013-09-16-week-09.md) |
| 2013-W40 | 2013-09-30 to 2013-10-06 | [GLRobotSim CMake file](2013-09-30-week-10.md) |
| 2013-W42 | 2013-10-14 to 2013-10-20 | [XML hardening and image filters](2013-10-14-week-11.md) |
| 2013-W43 | 2013-10-21 to 2013-10-27 | [Filter suite and ANN trainer](2013-10-21-week-12.md) |
| 2013-W44 | 2013-10-28 to 2013-11-03 | [WebUI reconnect and persistent views](2013-10-28-week-13.md) |
