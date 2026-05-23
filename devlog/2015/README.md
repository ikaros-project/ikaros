# Ikaros 2015: Parameter Binding, WebUI Work, and Issue Cleanup

**Year:** 2015  
**Status:** Draft for review  
<!-- **Suggested visual:** WebUI screenshot showing Marker, Histogram, or Select-style modules. -->

## Year in Brief

2015 concentrates a large amount of early work into January and February, then returns for issue fixes and marker/camera work later in the year. The main story is the tightening of WebUI binding, kernel parameter/default handling, optional inputs, examples, and issue-driven cleanup. The year is less about adding a single flagship subsystem and more about making the existing system more coherent for users building networks from many smaller parts. The steady emphasis on defaults, inheritance, and optional inputs suggests that network configuration was becoming more ambitious and needed better language support.

## Development Notes

The first weeks of 2015 were dense. Work recorded under `ikaros-project` updated WebUI objects, gallery behavior, paths, realtime controls, binding mechanisms, hierarchy examples, error/warning counting, and parameter binding. Kernel behavior improved around cycle detection, loop handling during `SetSize`, matrix size inference, serial exceptions, and optional inputs. This concentration of kernel and WebUI work made the system more predictable when networks became larger, nested, or configured from the command line. These changes also made WebUI behavior less special-case, because more of the interface could depend on consistent kernel-side binding and size rules.

Christian Balkenius worked on StepSlider, LinearAssociator, Histogram, Sum, and related examples. The LinearAssociator branch added LMS learning and default views, while old Sum code was removed and new Summation/Histogram examples were introduced. These changes made the examples and modules better aligned with the evolving defaults and binding system. Those example changes matter because they turned implementation work into runnable demonstrations that could be used for regression checks and teaching. They also helped preserve institutional knowledge: a working example records not only that a module exists, but how it is meant to be wired and observed.

The WebUI and kernel became more consistent around inheritance and command-line parameters. Full inheritance, command-line parameter support, name/title inheritance, group-list behavior, and attribute inheritance all landed. The marker and histogram work continued through February with `Marker.js`, marker experiments, size inputs, CMake changes, face-detector examples, and test updates. The inheritance work also made configuration less repetitive, allowing common attributes to flow through groups and command-line entry points more naturally. That work made Ikaros configurations more compositional, which is essential when models are assembled from nested groups and reusable modules.

Later in the year, the repository received issue-focused fixes around default handling and public issues, Dynamixel torque fixes from Birger Johansson, and an ARToolKit camera-location module. The later fixes gave the year a practical close: bugs were addressed, examples were adjusted, and marker/camera work remained connected to real use cases. The year closes as a pragmatic cleanup cycle that made the public issue tracker, examples, and module behavior better match one another.

## Milestones

- New parameter binding and WebUI binding mechanisms.
- Full inheritance and command-line parameter support.
- Optional input support and trainer/perceptron updates.
- StepSlider, Histogram, Sum, and Select module work.
- Marker widget and face-detector example updates.
- Public issue fixes and default-handling updates.
- ARToolKit camera-location module.
## Weekly Entries

| Week | Date range | Title |
| --- | --- | --- |
| 2015-W01 | 2014-12-29 to 2015-01-04 | [Vision and Robotics Updates](2014-12-29-week-01.md) |
| 2015-W02 | 2015-01-05 to 2015-01-11 | [WebUI and Math Updates](2015-01-05-week-02.md) |
| 2015-W03 | 2015-01-12 to 2015-01-18 | [WebUI and Logging Updates](2015-01-12-week-03.md) |
| 2015-W04 | 2015-01-19 to 2015-01-25 | [Math and Kernel Updates](2015-01-19-week-04.md) |
| 2015-W05 | 2015-01-26 to 2015-02-01 | [Math and Vision Updates](2015-01-26-week-05.md) |
| 2015-W06 | 2015-02-02 to 2015-02-08 | [WebUI and Tests Updates](2015-02-02-week-06.md) |
| 2015-W07 | 2015-02-09 to 2015-02-15 | [Kernel connect bug](2015-02-09-week-07.md) |
| 2015-W08 | 2015-02-16 to 2015-02-22 | [Vision and Build Updates](2015-02-16-week-08.md) |
| 2015-W09 | 2015-02-23 to 2015-03-01 | [Vision and Tests Updates](2015-02-23-week-09.md) |
| 2015-W17 | 2015-04-20 to 2015-04-26 | [FaceDetector test change](2015-04-20-week-10.md) |
| 2015-W20 | 2015-05-11 to 2015-05-17 | [Weekly Development Updates](2015-05-11-week-11.md) |
| 2015-W26 | 2015-06-22 to 2015-06-28 | [Tests Updates](2015-06-22-week-12.md) |
| 2015-W38 | 2015-09-14 to 2015-09-20 | [Weekly Development Updates](2015-09-14-week-13.md) |
| 2015-W41 | 2015-10-05 to 2015-10-11 | [Vision Updates](2015-10-05-week-14.md) |
