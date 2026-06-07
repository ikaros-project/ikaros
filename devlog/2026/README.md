# Ikaros 2026: New UI, Safety Hardening, Statistics, and Vision Modules

**Year:** 2026  
**Status:** Draft for review  
<!-- **Suggested visual:** Screenshot of the new UI. -->

## Year in Brief

The 2026 entries through May form one of the most active periods in the repository history. The work spans WebUI editing, logging, error recovery, generated documentation, library views, Epi/robotics updates, Python modules, C++17 cleanup, matrix work, safer parsing and sockets, audio modules, security restrictions, statistics, face detection and recognition, salience integration, camera rectification, and a new UI. Within the period covered so far, the volume and variety of commits make 2026 one of the most consequential spans in the devlog. The suggested new-UI screenshot is the natural visual anchor for the summary, because it represents the most visible result of many deeper changes recorded in the current 2026 entries.

## Development Notes

The year starts with Epi sequence recorder updates and quickly moves into WebUI, tests, logging, and data changes. March is especially dense: widget editing, selection mechanics, network editing, auto-routing, inspector behavior, group editing, error recovery, module library views, generated module descriptions, and documentation all advance in rapid succession. This work changes the day-to-day feel of the system, making editing, selecting, arranging, inspecting, and recovering from errors central design concerns. The generated documentation and library-view work also make the system more navigable as the module library grows during this period.

Late March and early April add deeper runtime changes. Python modules enter with a minimal implementation and then module-code support. Batch mode, headless behavior, test cleanup, logging, dictionary migration, JSON/XML parsing, parameter resolution, and C++17 cleanup follow. Matrix code becomes a major focus through multidimensional literals, optimized loading, const correctness, unit tests, implementation moves, and safer size handling. The runtime modernization is broad rather than cosmetic: parsing, types, sockets, logging, batch behavior, and matrix operations are all being revisited. Those changes reduce technical debt in the parts of the system that everything else depends on.

Security and robustness become explicit. Socket handling, signal-handler safety, file path sanitization, limited file access, limited spawn behavior, optional authentication, safer Python-backed modules, safer external group import, and IP bind/agent settings all land in the same broader period. This cluster gives 2026 a clear engineering theme: Ikaros is becoming more capable while also becoming more careful about what code and users are allowed to do. The safety work is especially significant because Python modules, external files, sockets, and browser control all increase the need for clear boundaries.

The module surface also expands. AudioInput/Output moves are followed by ADSR, filters, delays, reverb, and output-file support. Statistics and regression modules arrive with widgets, box plots, matrix metadata, and multiple-comparison correction. Face detection and recognition, Apple and Dlib detectors, face classification, PointsToSalience, SalienceIntegrator, stacked inputs, matrix parameter functions, and CameraRectification all enter before the May new-UI work. The result is a period that connects modern interface work with new scientific and analytical modules, giving the platform both a cleaner surface and a broader toolkit. That combination makes 2026 feel like a platform renewal rather than just a feature sprint.

## Milestones

- Major WebUI editing and new UI work.
- Generated module README/descriptions and library view.
- Python module support.
- Logging, dictionary, JSON, XML, and socket modernization.
- C++17 and matrix-code cleanup.
- Safety hardening: file access, spawn, auth, signal handling, Python-backed modules.
- Audio module expansion.
- Statistics and regression modules with widgets.
- Face detection/recognition and salience modules.
- CameraRectification first commit.

## Weekly Entries

| Week | Date range | Title |
| --- | --- | --- |
| 2026-W04 | 2026-01-19 to 2026-01-25 | [Updated sequence recorder files for Epi](2026-01-19-week-01.md) |
| 2026-W05 | 2026-01-26 to 2026-02-01 | [WebUI and Tests Updates](2026-01-26-week-02.md) |
| 2026-W06 | 2026-02-02 to 2026-02-08 | [Logging and WebUI Updates](2026-02-02-week-03.md) |
| 2026-W07 | 2026-02-09 to 2026-02-15 | [Data and WebUI Updates](2026-02-09-week-04.md) |
| 2026-W08 | 2026-02-16 to 2026-02-22 | [Docs Updates](2026-02-16-week-05.md) |
| 2026-W09 | 2026-02-23 to 2026-03-01 | [WebUI and Tests Updates](2026-02-23-week-06.md) |
| 2026-W10 | 2026-03-02 to 2026-03-08 | [WebUI and Kernel Updates](2026-03-02-week-07.md) |
| 2026-W11 | 2026-03-09 to 2026-03-15 | [WebUI and Kernel Updates](2026-03-09-week-08.md) |
| 2026-W12 | 2026-03-16 to 2026-03-22 | [WebUI and Logging Updates](2026-03-16-week-09.md) |
| 2026-W13 | 2026-03-23 to 2026-03-29 | [Robotics and Logging Updates](2026-03-23-week-10.md) |
| 2026-W14 | 2026-03-30 to 2026-04-05 | [Logging and Tests Updates](2026-03-30-week-11.md) |
| 2026-W15 | 2026-04-06 to 2026-04-12 | [Math and Security Updates](2026-04-06-week-12.md) |
| 2026-W16 | 2026-04-13 to 2026-04-19 | [Audio and WebUI Updates](2026-04-13-week-13.md) |
| 2026-W17 | 2026-04-20 to 2026-04-26 | [WebUI and Math Updates](2026-04-20-week-14.md) |
| 2026-W18 | 2026-04-27 to 2026-05-03 | [Math and WebUI Updates](2026-04-27-week-15.md) |
| 2026-W19 | 2026-05-04 to 2026-05-10 | [Vision and Math Updates](2026-05-04-week-16.md) |
| 2026-W20 | 2026-05-11 to 2026-05-17 | [Docs Updates](2026-05-11-week-17.md) |
| 2026-W21 | 2026-05-18 to 2026-05-24 | [WebUI and Math Updates](2026-05-18-week-18.md) |
| 2026-W22 | 2026-05-25 to 2026-05-31 | [WebUI and World2D Updates](2026-05-25-week-19.md) |
| 2026-W23 | 2026-06-01 to 2026-06-07 | [CVAE, Locale Parsing, and Optional Dimensions](2026-06-01-week-20.md) |
