# Ikaros 2016: Hardware Integrations, Audio Modules, and Runtime Expansion

**Year:** 2016  
**Status:** Draft for review  
<!-- **Suggested visual:** FadeCandy or audio-module setup in the WebUI. -->

## Year in Brief

2016 expands the runtime and module ecosystem through hardware protocols, audio, recorder/model work, pupil models, and sustained test and kernel changes. The year begins with compatibility and hardware updates, then moves into a long middle period of module and runtime development. The year reads as a widening of Ikaros into more devices, more protocols, and more runtime behaviors, with tests and examples trying to keep pace. The frequency of active weeks also shows that development had moved into a more continuous cadence after the issue-driven work of 2015.

## Development Notes

The year opened with Birger Johansson replacing deprecated FFMpeg calls and rewriting the Dynamixel module to support both protocol 1 and 2. Christian Balkenius tested AVKit, continued FadeCandy work, and added socket-send changes. FadeCandy then moved from near-complete work into a server-starting module path, with `GetModulePath` and `GetClassPath` updates supporting module discovery. This hardware work strengthened Ikaros as a bridge between computational models and physical or visual output devices. Protocol support and external-device output made Ikaros more useful for demonstrations where models drive behavior outside the process itself.

Audio and output work became more visible through SoundOutput/FadeCandy-related additions, OutputFile tests, unique directory creation, and write/newfile inputs. The repository also gained recorder and model work in the later active weeks of the year, including many tests and kernel-support changes. The output work is important because it made generated data and external effects easier to manage as first-class results of a run. Managing files and generated outputs more carefully also improved reproducibility, since experimental runs need predictable artifacts.

The middle and late parts of 2016 are broad and dense. Commit themes include WebUI updates, logging, tests, pupil and robotics models, CMake/build fixes, matrix/math changes, kernel behavior, and module examples. Christian Balkenius, Birger Johansson, and `ikaros-project` appear throughout the year, with contributions spanning both feature work and maintenance. Many of these commits are small in isolation, but together they show a system absorbing practical needs from experiments, demos, and module testing. The year’s breadth makes it hard to summarize as one feature, but easy to understand as platform growth.

By the end of the year, the repository had absorbed a significant amount of hardware-facing, audio-facing, and runtime-support work. The weekly logs show a project moving from isolated module additions toward broader infrastructure for modules, examples, tests, and live runtime behavior. The result is a repository that looks less like a set of isolated research modules and more like a runtime environment with hardware, files, sockets, tests, and user-visible feedback all under active development. That wider runtime character made interface behavior, runtime boundaries, and test coverage important concerns inside the year itself.

## Milestones

- FFMpeg compatibility updates.
- Dynamixel protocol 1 and 2 support.
- FadeCandy module work and server startup.
- Module/class path discovery improvements.
- OutputFile tests and unique output directory support.
- Recorder, model, pupil, and test expansions.
- Sustained WebUI, logging, and kernel updates.
## Weekly Entries

| Week | Date range | Title |
| --- | --- | --- |
| 2016-W14 | 2016-04-04 to 2016-04-10 | [Replacing a few deprecated function dropped in FFMPEG 3](2016-04-04-week-01.md) |
| 2016-W15 | 2016-04-11 to 2016-04-17 | [Robotics and Tests Updates](2016-04-11-week-02.md) |
| 2016-W18 | 2016-05-02 to 2016-05-08 | [Audio and Security Updates](2016-05-02-week-03.md) |
| 2016-W19 | 2016-05-09 to 2016-05-15 | [Tests Updates](2016-05-09-week-04.md) |
| 2016-W20 | 2016-05-16 to 2016-05-22 | [Logging and WebUI Updates](2016-05-16-week-05.md) |
| 2016-W21 | 2016-05-23 to 2016-05-29 | [WebUI and Tests Updates](2016-05-23-week-06.md) |
| 2016-W22 | 2016-05-30 to 2016-06-05 | [Build Updates](2016-05-30-week-07.md) |
| 2016-W24 | 2016-06-13 to 2016-06-19 | [Vision and Kernel Updates](2016-06-13-week-08.md) |
| 2016-W25 | 2016-06-20 to 2016-06-26 | [Recorder module](2016-06-20-week-09.md) |
| 2016-W26 | 2016-06-27 to 2016-07-03 | [Pupil model working](2016-06-27-week-10.md) |
| 2016-W29 | 2016-07-18 to 2016-07-24 | [Updated model](2016-07-18-week-11.md) |
| 2016-W31 | 2016-08-01 to 2016-08-07 | [Audio Updates](2016-08-01-week-12.md) |
| 2016-W33 | 2016-08-15 to 2016-08-21 | [Build and Tests Updates](2016-08-15-week-13.md) |
| 2016-W35 | 2016-08-29 to 2016-09-04 | [Vision and Tests Updates](2016-08-29-week-14.md) |
| 2016-W36 | 2016-09-05 to 2016-09-11 | [WebUI and Robotics Updates](2016-09-05-week-15.md) |
| 2016-W37 | 2016-09-12 to 2016-09-18 | [Tests and WebUI Updates](2016-09-12-week-16.md) |
| 2016-W38 | 2016-09-19 to 2016-09-25 | [Build and WebUI Updates](2016-09-19-week-17.md) |
| 2016-W39 | 2016-09-26 to 2016-10-02 | [Robotics and Kernel Updates](2016-09-26-week-18.md) |
| 2016-W40 | 2016-10-03 to 2016-10-09 | [Robotics and WebUI Updates](2016-10-03-week-19.md) |
| 2016-W41 | 2016-10-10 to 2016-10-16 | [Weekly Development Updates](2016-10-10-week-20.md) |
| 2016-W42 | 2016-10-17 to 2016-10-23 | [Robotics and Logging Updates](2016-10-17-week-21.md) |
| 2016-W43 | 2016-10-24 to 2016-10-30 | [WebUI and Robotics Updates](2016-10-24-week-22.md) |
| 2016-W45 | 2016-11-07 to 2016-11-13 | [Logging Updates](2016-11-07-week-23.md) |
| 2016-W46 | 2016-11-14 to 2016-11-20 | [Robotics and Kernel Updates](2016-11-14-week-24.md) |
| 2016-W47 | 2016-11-21 to 2016-11-27 | [Robotics and WebUI Updates](2016-11-21-week-25.md) |
| 2016-W48 | 2016-11-28 to 2016-12-04 | [Build and Logging Updates](2016-11-28-week-26.md) |
| 2016-W49 | 2016-12-05 to 2016-12-11 | [Build and Logging Updates](2016-12-05-week-27.md) |
| 2016-W50 | 2016-12-12 to 2016-12-18 | [Kernel and Tests Updates](2016-12-12-week-28.md) |
| 2016-W51 | 2016-12-19 to 2016-12-25 | [Tests and Math Updates](2016-12-19-week-29.md) |
| 2016-W52 | 2016-12-26 to 2017-01-01 | [Tests and Kernel Updates](2016-12-26-week-30.md) |
