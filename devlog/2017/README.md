# Ikaros 2017: Robotics, Pupil Models, and Kernel Refinement

**Year:** 2017  
**Status:** Draft for review  
<!-- **Suggested visual:** Pupil or robotics demo screenshot. -->

## Year in Brief

2017 continues the hardware and model direction from 2016. The weekly logs show repeated robotics, pupil, vision, math, kernel, build, and test work, with bursts of dense activity early in the year and lighter maintenance in the later active weeks. Compared with the surrounding years, 2017 is more consolidating than expansive, but it keeps several experimental lines alive. The weekly logs suggest attention to keeping experimental systems operational rather than launching a new architectural direction.

## Development Notes

The year begins with math, WebUI, logging, vision, and robotics updates. Work recorded under `ikaros-project` continues to refine the runtime while testing and examples keep pace. Birger Johansson and Christian Balkenius appear in several robotics and vision weeks, with the codebase receiving both module-specific fixes and infrastructure-level polish. The recurring combination of robotics and vision indicates that sensing and action were still being developed together rather than as separate module silos. This kept the project aligned with its original goal of connecting computational models to sensorimotor loops.

Pupil-model work is a recurring thread. The logs include pupil sizing, robotics and vision updates, and test work tied to those systems. This keeps Ikaros connected to embodied and sensor-driven modeling rather than only abstract module composition. The pupil work also gave the year a recognizable experimental center, tying low-level module updates to an embodied modeling task. It also helped keep the robotics thread visible during a year that otherwise contains many smaller support changes.

Kernel and math updates run alongside the application-level work. The year includes weeks focused on kernel changes, math support, build updates, and tests. These are not isolated chores; they are what let the growing module set stay buildable and testable. This background work is what kept the larger experimental modules usable as their dependencies and examples changed. That sort of maintenance does not create dramatic milestones, but it prevents module ecosystems from becoming brittle.

The later active weeks are quieter, but still active. Single-commit and small-week updates handle targeted merges, kernel changes, and maintenance. The project state at the end of 2017 is more stable around robotics/pupil experiments and the supporting kernel infrastructure. The quieter ending still matters because it preserves continuity after the heavy 2016 activity. In that sense 2017 functions as a consolidation year: it keeps the robotics and model work operational while the repository continues to refine its foundations.

## Milestones

- Continued robotics and vision module development.
- Pupil-model and pupil-size work.
- Kernel and math refinements.
- Test and build updates around active modules.
- Sustained WebUI and logging maintenance.
## Weekly Entries

| Week | Date range | Title |
| --- | --- | --- |
| 2017-W01 | 2017-01-02 to 2017-01-08 | [Math and WebUI Updates](2017-01-02-week-01.md) |
| 2017-W02 | 2017-01-09 to 2017-01-15 | [Logging Updates](2017-01-09-week-02.md) |
| 2017-W03 | 2017-01-16 to 2017-01-22 | [Vision and Robotics Updates](2017-01-16-week-03.md) |
| 2017-W05 | 2017-01-30 to 2017-02-05 | [Kernel and Math Updates](2017-01-30-week-04.md) |
| 2017-W06 | 2017-02-06 to 2017-02-12 | [Vision and Robotics Updates](2017-02-06-week-05.md) |
| 2017-W09 | 2017-02-27 to 2017-03-05 | [Robotics and Vision Updates](2017-02-27-week-06.md) |
| 2017-W10 | 2017-03-06 to 2017-03-12 | [Robotics and Tests Updates](2017-03-06-week-07.md) |
| 2017-W11 | 2017-03-13 to 2017-03-19 | [Setting pupil sizes](2017-03-13-week-08.md) |
| 2017-W12 | 2017-03-20 to 2017-03-26 | [Tests and Build Updates](2017-03-20-week-09.md) |
| 2017-W13 | 2017-03-27 to 2017-04-02 | [Robotics and Tests Updates](2017-03-27-week-10.md) |
| 2017-W14 | 2017-04-03 to 2017-04-09 | [Kernel Updates](2017-04-03-week-11.md) |
| 2017-W15 | 2017-04-10 to 2017-04-16 | [Math and Kernel Updates](2017-04-10-week-12.md) |
| 2017-W16 | 2017-04-17 to 2017-04-23 | [WebUI and Tests Updates](2017-04-17-week-13.md) |
| 2017-W17 | 2017-04-24 to 2017-04-30 | [WebUI Updates](2017-04-24-week-14.md) |
| 2017-W18 | 2017-05-01 to 2017-05-07 | [WebUI and Tests Updates](2017-05-01-week-15.md) |
| 2017-W19 | 2017-05-08 to 2017-05-14 | [Cleaning up](2017-05-08-week-16.md) |
| 2017-W20 | 2017-05-15 to 2017-05-21 | [Starting communication](2017-05-15-week-17.md) |
| 2017-W21 | 2017-05-22 to 2017-05-28 | [WebUI and Tests Updates](2017-05-22-week-18.md) |
| 2017-W22 | 2017-05-29 to 2017-06-04 | [Stauffer-Grimson preliminary version](2017-05-29-week-19.md) |
| 2017-W31 | 2017-07-31 to 2017-08-06 | [Merge branch 'master' into c_ANN](2017-07-31-week-20.md) |
| 2017-W35 | 2017-08-28 to 2017-09-03 | [Memory Model](2017-08-28-week-21.md) |
| 2017-W36 | 2017-09-04 to 2017-09-10 | [AA progress](2017-09-04-week-22.md) |
| 2017-W37 | 2017-09-11 to 2017-09-17 | [Visual Flow modules](2017-09-11-week-23.md) |
| 2017-W38 | 2017-09-18 to 2017-09-24 | [Merge branch 'master' into c_ANN](2017-09-18-week-24.md) |
| 2017-W40 | 2017-10-02 to 2017-10-08 | [Kernel Updates](2017-10-02-week-25.md) |
| 2017-W46 | 2017-11-13 to 2017-11-19 | [Kernel Updates](2017-11-13-week-26.md) |
