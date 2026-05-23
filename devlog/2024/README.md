# Ikaros 2024: Sequence Tooling, Tests, and Documentation

**Year:** 2024  
**Status:** Draft for review  
<!-- **Suggested visual:** Sequence, crop, or time-series visualization. -->

## Year in Brief

2024 rebuilds momentum through FFMPEG build updates, WebUI/tests, data changes, sequence timing/crop fixes, documentation, robotics tests, audio tests, math tests, logging, and utility functions. The year is more active than 2023 and shows renewed attention to the workflows around recorded data, tests, and examples. The year’s activity suggests a project strengthening its data and test workflows.

## Development Notes

The year opens with FFMPEG CMake updates and then moves into WebUI/tests and data work. These changes keep the media and runtime data paths current while preserving test coverage around user-facing behavior. This mattered because video, audio, sequence data, and runtime values all depend on consistent timing and reliable file/module behavior. The FFMPEG and sequence-related work connects media-input themes with runtime analysis needs visible in the 2024 entries.

Sequence and timing work becomes a clear theme. The end-time fix for `crop()` and later sequence/documentation/data updates point to careful handling of time-based runs and recorded data. A sequence or time-series visual would capture the year's practical concern with temporal structure. The sequence work points to real use cases where a run is not just a momentary computation but a structured recording that must be cropped, replayed, or analyzed. That makes a timeline or time-series visual more appropriate than a generic screenshot.

Autumn and winter bring repeated test, audio, math, WebUI, logging, and data weeks. The addition of `GetTimeOfDay` and subsequent logging/math and math/data updates show incremental strengthening of runtime utilities. These updates gave the project a stronger base for the sequence, data, and test work visible within 2024 itself. The year’s contribution is therefore enabling work: it improves the surfaces where current changes can be verified and explained.

By the end of 2024, the repository had stronger support around sequence handling, examples, documentation, and test-backed module behavior.

## Milestones

- FFMPEG CMake update.
- WebUI and test updates.
- Sequence/crop timing fix.
- Documentation and data updates.
- Robotics, audio, and math test work.
- `GetTimeOfDay` and runtime utility updates.
## Weekly Entries

| Week | Date range | Title |
| --- | --- | --- |
| 2024-W04 | 2024-01-22 to 2024-01-28 | [Updated FindFFMPEG.cmake](2024-01-22-week-01.md) |
| 2024-W07 | 2024-02-12 to 2024-02-18 | [WebUI and Tests Updates](2024-02-12-week-02.md) |
| 2024-W09 | 2024-02-26 to 2024-03-03 | [Data Updates](2024-02-26-week-03.md) |
| 2024-W10 | 2024-03-04 to 2024-03-10 | [Fixed end_time for sequence if using crop()](2024-03-04-week-04.md) |
| 2024-W31 | 2024-07-29 to 2024-08-04 | [Docs Updates](2024-07-29-week-05.md) |
| 2024-W36 | 2024-09-02 to 2024-09-08 | [Docs and Data Updates](2024-09-02-week-06.md) |
| 2024-W37 | 2024-09-09 to 2024-09-15 | [Tests and Robotics Updates](2024-09-09-week-07.md) |
| 2024-W38 | 2024-09-16 to 2024-09-22 | [Tests and Audio Updates](2024-09-16-week-08.md) |
| 2024-W39 | 2024-09-23 to 2024-09-29 | [Tests and Math Updates](2024-09-23-week-09.md) |
| 2024-W40 | 2024-09-30 to 2024-10-06 | [WebUI and Logging Updates](2024-09-30-week-10.md) |
| 2024-W41 | 2024-10-07 to 2024-10-13 | [Added GetTimeOfDay](2024-10-07-week-11.md) |
| 2024-W42 | 2024-10-14 to 2024-10-20 | [Logging and Math Updates](2024-10-14-week-12.md) |
| 2024-W43 | 2024-10-21 to 2024-10-27 | [Math and Kernel Updates](2024-10-21-week-13.md) |
| 2024-W44 | 2024-10-28 to 2024-11-03 | [Kernel and WebUI Updates](2024-10-28-week-14.md) |
| 2024-W46 | 2024-11-11 to 2024-11-17 | [Kernel and Tests Updates](2024-11-11-week-15.md) |
| 2024-W48 | 2024-11-25 to 2024-12-01 | [Math and Logging Updates](2024-11-25-week-16.md) |
| 2024-W49 | 2024-12-02 to 2024-12-08 | [Math and Data Updates](2024-12-02-week-17.md) |
| 2024-W50 | 2024-12-09 to 2024-12-15 | [Math and Build Updates](2024-12-09-week-18.md) |
| 2024-W51 | 2024-12-16 to 2024-12-22 | [Math and WebUI Updates](2024-12-16-week-19.md) |
