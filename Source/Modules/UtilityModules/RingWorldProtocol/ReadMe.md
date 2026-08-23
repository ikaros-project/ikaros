# RingWorldProtocol

## Protocol format manual

## Introduction

RingWorld Protocol describes classical-conditioning experiments in JSON. It specifies reusable
stimuli, trial timing, contextual stimuli, repeated blocks, probability, and the response
measurements collected during each trial.

No knowledge of Ikaros is needed to write a protocol. All times are expressed in seconds. Colors,
intensities, and probabilities use values from 0 to 1. Trials execute in the order in which they
appear.

The format is intended to make the experimental design visible in the file. A reader should be
able to identify the conditioned and unconditioned stimuli, see their temporal relationship, find
the reinforcement schedule, and understand which responses are measured without inspecting
program code. Reusable stimulus templates keep physical stimulus properties separate from the
trial schedule, while nested repeated blocks provide a direct representation of acquisition,
extinction, test, and renewal phases.

This manual describes the proposed format. Details may change before the RingWorldProtocol module
is implemented.

## File overview

```json
{
  "version": 1,
  "metadata": {},
  "seed": 12345,
  "defaults": {},
  "stimului": {},
  "trials": {},
  "responses": {},
  "context": {},
  "protocol": []
}
```

| Field | Purpose |
| --- | --- |
| `version` | Selects the protocol-format version used to interpret the file. |
| `metadata` | Describes the experiment, subject, session, and experimental condition. |
| `seed` | Makes random intervals and probability decisions reproducible. |
| `defaults` | Supplies values omitted from named stimuli. |
| `stimului` | Defines reusable, named stimulus templates. |
| `trials` | Defines reusable, named trial templates. |
| `responses` | Declares logical response signals that can be measured. |
| `context` | Defines stimuli active throughout the complete protocol. |
| `protocol` | Lists trials and repeated blocks in execution order. |

The spelling `stimului` is part of the proposed format.

## Protocol version

Every complete protocol file must declare its format version:

```json
{
  "version": 1
}
```

The version is a positive integer. Version 1 denotes the format described in this manual. A loader
must reject a file whose version it does not support instead of guessing how to interpret it.

Additions that do not change the meaning of existing fields may remain within the same version.
Removing a field, changing a field's meaning, or changing timing semantics requires a new version.
An implementation may support several versions, but it must validate each file according to the
rules of the declared version.

## Experiment metadata

The optional `metadata` object records information needed to identify and organize an experiment:

```json
{
  "metadata": {
    "name": "Context renewal study",
    "description": "Acquisition in A, extinction in B, and test in A",
    "subject": "animal_17",
    "session": 2,
    "condition": "ABA",
    "researcher": "Researcher name",
    "tags": ["conditioning", "renewal", "partial_reinforcement"]
  }
}
```

Metadata does not alter stimulus generation, timing, probability, or measurement. It is copied into
the experiment record so that data can be identified without relying on a filename. Recommended
fields are:

| Field | Meaning |
| --- | --- |
| `name` | Human-readable experiment or protocol name. |
| `description` | Brief explanation of the experimental purpose or design. |
| `subject` | Subject, animal, participant, or simulated-agent identifier. |
| `session` | Session identifier or sequence number. |
| `condition` | Between-subject or between-session experimental condition. |
| `researcher` | Person or group responsible for the session. |
| `tags` | Searchable labels used to organize protocols and result files. |

Additional metadata fields are allowed if their values are JSON strings, numbers, booleans, null,
or lists of those scalar types. Arbitrary nested objects are discouraged because they make tabular
result export difficult.

A reusable protocol may omit subject-specific fields. The experiment launcher may supply or
override `subject`, `session`, `condition`, and `researcher` before the protocol is resolved. The
final merged metadata must be stored with both the resolved schedule and the recorded results.

## A minimal protocol

This is the smallest useful conditioning example. It defines a blue vertical conditioned stimulus
(CS), a rewarding white unconditioned stimulus (US), and one trial in which the US begins 1.5
seconds after CS onset.

```json
{
  "version": 1,
  "defaults": {
    "angle": 0,
    "reward": 0,
    "punishment": 0,
    "intensity": 1,
    "rgb": [1, 1, 1]
  },
  "stimului": {
    "cs": {
      "rgb": [0, 0, 1],
      "components": ["vertical"]
    },
    "us": {
      "reward": 1
    }
  },
  "protocol": [
    {
      "trial": {
        "name": "acquisition",
        "duration": 3.0,
        "stimuli": [
          {
            "id": "cs_presentation",
            "stimulus": "cs",
            "onset": 0,
            "duration": 2.0
          },
          {
            "id": "us_presentation",
            "stimulus": "us",
            "inter_stimulus_interval": 1.5,
            "duration": 1.0
          }
        ],
        "inter_trial_interval": 10.0
      }
    }
  ]
}
```

The two presentations overlap for 0.5 seconds. After the 3-second trial, the world enters a
10-second inter-trial interval before the next protocol item.

## Conditioning terminology used in this manual

| Term | Representation in the protocol |
| --- | --- |
| Conditioned stimulus (CS) | A named stimulus template used by a trial presentation. |
| Unconditioned stimulus (US) | Usually a stimulus with non-zero `reward` or `punishment`. |
| Conditioned response (CR) | A named response measured in a sampling window. |
| Inter-stimulus interval (ISI) | Onset-to-onset time between successive presentations. |
| Inter-trial interval (ITI) | Time after a trial and before the next protocol item. |
| Context | A stimulus inherited by trials within an enclosing protocol scope. |
| Reinforcement schedule | A presentation probability, commonly applied to the US. |

The format does not require a stimulus to be labeled CS or US. Those roles follow from how the
stimuli are used. This permits compound conditioning, inhibition, extinction, and transfer designs
without adding special stimulus categories.

## Defining stimuli

RingWorld internally uses this stimulus-vector order:

```text
angle, reward, punishment, intensity,
red, green, blue,
horizontal, vertical, diagonal_minus_45, diagonal_plus_45,
arrow_left, arrow_right, dot
```

The JSON format uses names so that this vector never needs to be written manually.

### Defaults

```json
{
  "defaults": {
    "angle": 0,
    "reward": 0,
    "punishment": 0,
    "intensity": 1,
    "rgb": [1, 1, 1]
  }
}
```

| Field | Meaning |
| --- | --- |
| `angle` | Position around the ring in degrees. Zero is straight ahead. |
| `reward` | Reward associated with fixation, normally from 0 to 1. |
| `punishment` | Punishment associated with fixation, normally from 0 to 1. |
| `intensity` | Visual intensity and relative stimulus diameter, from 0 to 1. |
| `rgb` | Red, green, and blue color channels, each from 0 to 1. |

Components do not appear in `defaults`. Every component starts at zero.

### Named stimulus templates

```json
{
  "stimului": {
    "red_vertical": {
      "angle": -30,
      "rgb": [1, 0, 0],
      "components": ["vertical"]
    },
    "blue_dot": {
      "angle": 30,
      "intensity": 0.8,
      "rgb": [0, 0, 1],
      "components": ["dot"]
    },
    "food": {
      "reward": 1,
      "rgb": [1, 1, 1],
      "components": ["horizontal", "vertical"]
    }
  }
}
```

Valid component names are:

```text
horizontal
vertical
diagonal_minus_45
diagonal_plus_45
arrow_left
arrow_right
dot
```

Every listed component is set to one; all unlisted components are zero. A presentation-level
`components` list replaces the list in its named stimulus.

Omitted properties inherit from `defaults`. A property set to `null` must be supplied later by a
context or presentation override. Starting an experiment with an unresolved `null` is an error.

Stimuli are resolved in this order:

```text
defaults -> named stimulus -> context overrides -> presentation overrides
```

## Trial templates

The optional top-level `trials` object defines named trial templates. Templates reduce repetition
and make the trial types in an experiment easy to identify:

```json
{
  "trials": {
    "cs_plus": {
      "duration": 4.0,
      "stimuli": [
        {
          "id": "cs",
          "stimulus": "tone_cs",
          "onset": 0.5,
          "duration": 2.0
        },
        {
          "id": "us",
          "stimulus": "food_us",
          "inter_stimulus_interval": 1.5,
          "duration": 1.0
        }
      ],
      "inter_trial_interval": {
        "min": 10.0,
        "max": 15.0
      }
    }
  }
}
```

A protocol item invokes the template by name:

```json
{
  "trial": {
    "template": "cs_plus"
  }
}
```

The template name becomes the trial name unless the invocation supplies another `name`.

### Template overrides

An invocation may override trial fields without changing the reusable definition:

```json
{
  "trial": {
    "template": "cs_plus",
    "name": "weak_cs_plus",
    "overrides": {
      "duration": 4.5,
      "inter_trial_interval": 12.0,
      "stimuli": {
        "cs": {
          "intensity": 0.5,
          "angle": 30
        }
      }
    }
  }
}
```

Keys under `overrides.stimuli` refer to presentation `id` values in the template. Their properties
are merged into those presentations. `duration`, `inter_trial_interval`, `context`, and `sampling`
may also be overridden. An overridden `context` is merged by context-entry name; an overridden
`sampling` list replaces the template's complete sampling list.

Every template presentation must have a unique `id` if the template can be overridden or referenced
by a sampling window. Referring to an unknown template or presentation identifier is an error.
Templates cannot inherit from other templates in version 1; this avoids recursive definitions and
keeps startup resolution straightforward.

An inline trial and a trial template have identical execution semantics after resolution. The
resolved schedule and experiment record should retain both the template name and the final trial
name.

## Protocol items

The top-level `protocol` is an ordered list. Each item is either a trial or a repeated block:

```json
{
  "protocol": [
    {
      "trial": {
        "name": "single_trial",
        "duration": 3.0,
        "stimuli": []
      }
    },
    {
      "repeat": 5,
      "protocol": []
    }
  ]
}
```

## Trials

A trial has a fixed trial period and an optional interval before the next protocol item:

```json
{
  "trial": {
    "name": "conditioning_trial",
    "duration": 4.0,
    "stimuli": [],
    "sampling": [],
    "inter_trial_interval": 10.0
  }
}
```

`duration` runs from trial onset to trial offset. It is not the inter-trial interval:

```text
next item start = trial start + duration + inter_trial_interval
```

Trial stimuli are absent during the inter-trial interval. Contexts inherited from an outer scope
may remain active.

Conceptually, each execution follows this sequence:

1. Enter the trial and activate its inherited and trial-level contexts.
2. Present and remove scheduled stimuli as the trial clock advances.
3. Sample responses in every active sampling window.
4. End the trial at `duration` and remove its trial-level context.
5. Continue outer contexts during the inter-trial interval.
6. Proceed to the next trial or repeat iteration.

The trial duration should normally include all baseline, stimulus, trace, and post-stimulus periods
that belong analytically to the trial. The inter-trial interval is reserved for separation between
trials.

### Explicit onsets

Stimulus onsets are relative to trial onset:

```json
{
  "trial": {
    "name": "overlapping_stimuli",
    "duration": 4.0,
    "stimuli": [
      {
        "id": "cs",
        "stimulus": "red_vertical",
        "onset": 0.5,
        "duration": 2.0
      },
      {
        "id": "us",
        "stimulus": "food",
        "onset": 2.0,
        "duration": 1.0
      }
    ]
  }
}
```

The CS is active from 0.5 to 2.5 seconds. The US is active from 2.0 to 3.0 seconds, producing an
overlap of 0.5 seconds.

`id` names this presentation. `stimulus` refers to the reusable template. Presentation identifiers
are also used by stimulus-relative sampling windows.

### Inter-stimulus intervals

A presentation may use `inter_stimulus_interval` instead of an explicit onset:

```json
{
  "stimuli": [
    {
      "id": "cs",
      "stimulus": "red_vertical",
      "onset": 0.5,
      "duration": 2.0
    },
    {
      "id": "us",
      "stimulus": "food",
      "inter_stimulus_interval": 1.5,
      "duration": 1.0
    }
  ]
}
```

Here, inter-stimulus interval means onset-to-onset interval, also called stimulus-onset asynchrony:

```text
current onset = preceding presentation onset + inter_stimulus_interval
```

The US therefore begins at 2.0 seconds. A probabilistically omitted presentation remains a timing
anchor, so later presentations do not move when it is omitted.

The first presentation defaults to onset zero if neither timing field is present. Every later
presentation specifies either `onset` or `inter_stimulus_interval`, but not both.

### Overrides

A presentation can override its named stimulus:

```json
{
  "stimulus": "red_vertical",
  "onset": 0,
  "duration": 2.0,
  "angle": 45,
  "intensity": 0.5,
  "rgb": [1, 0.5, 0.5]
}
```

Possible overrides are `angle`, `reward`, `punishment`, `intensity`, `rgb`, and `components`.

### Probability

```json
{
  "id": "us",
  "stimulus": "food",
  "inter_stimulus_interval": 1.5,
  "duration": 1.0,
  "probability": 0.7
}
```

`probability` ranges from 0 to 1 and defaults to 1. It is sampled independently each time the trial
executes. An omitted presentation retains its timing but produces no stimulus.

### Fixed and random intervals

An interval can be fixed:

```json
"inter_trial_interval": 10.0
```

or sampled uniformly from a range:

```json
"inter_trial_interval": {
  "min": 10.0,
  "max": 15.0
}
```

The range form is also valid for `inter_stimulus_interval`. Bounds must be non-negative and `min`
must not exceed `max`. A new value is selected for every trial execution.

Every presentation must fit inside `trial.duration`.

## Repeated blocks

```json
{
  "repeat": 10,
  "protocol": [
    {
      "trial": {
        "name": "acquisition",
        "duration": 3.0,
        "stimuli": []
      }
    }
  ]
}
```

Repeated blocks may recursively contain trials and other repeated blocks:

```json
{
  "repeat": 2,
  "protocol": [
    {
      "repeat": 5,
      "protocol": [
        {
          "trial": {
            "name": "nested_trial",
            "duration": 2.0,
            "stimuli": []
          }
        }
      ]
    }
  ]
}
```

`repeat` is a non-negative integer. Zero skips the block.

## Contextual stimuli

`context` may occur at the top level, in a repeated block, or in a trial. Context entries are named
so that inner scopes can override them:

```json
{
  "context": {
    "room": {
      "stimulus": "room_light",
      "angle": -60
    }
  },
  "protocol": [
    {
      "repeat": 10,
      "context": {
        "tone": {
          "stimulus": "background_tone",
          "angle": 60
        }
      },
      "protocol": [
        {
          "trial": {
            "name": "contextual_trial",
            "duration": 3.0,
            "context": {
              "room": {
                "intensity": 0.7
              }
            },
            "stimuli": []
          }
        }
      ]
    }
  ]
}
```

Contexts accumulate from outer to inner scopes:

```text
top-level context -> repeat context -> nested repeat context -> trial context
```

In the example, `room` is active for the full protocol, `tone` is active through all ten repeat
iterations, and the trial changes the inherited room intensity to 0.7.

An inherited context can be disabled:

```json
{
  "context": {
    "tone": {
      "enabled": false
    }
  }
}
```

Context lifetimes are:

- A top-level context covers the complete protocol, including inter-trial intervals.
- A repeat context covers every nested iteration and its inter-trial intervals.
- A trial context covers trial onset through `trial.duration`, but not the following interval.

A context may have a `probability`. Its occurrence is sampled once when entering its scope. A
repeat context is consequently present or absent for the whole repeated block. A trial context is
sampled separately on every trial execution.

### Context changes between phases

Repeated blocks make context shifts explicit. For example, acquisition can occur in context A and
extinction in context B:

```json
{
  "protocol": [
    {
      "repeat": 10,
      "context": {
        "chamber": {
          "stimulus": "context_a"
        }
      },
      "protocol": [
        {
          "trial": {
            "name": "acquisition",
            "duration": 3.0,
            "stimuli": []
          }
        }
      ]
    },
    {
      "repeat": 10,
      "context": {
        "chamber": {
          "stimulus": "context_b"
        }
      },
      "protocol": [
        {
          "trial": {
            "name": "extinction",
            "duration": 3.0,
            "stimuli": []
          }
        }
      ]
    }
  ]
}
```

Both blocks use the context entry name `chamber`. The second block replaces the first block's
chamber definition rather than adding a second chamber stimulus.

## Responses and sampling windows

Top-level `responses` declares logical response names:

```json
{
  "responses": {
    "gaze_direction": {},
    "tracking": {},
    "lick_response": {}
  }
}
```

The method for connecting these names to experimental signals will be defined separately. Logical
names allow one protocol design to be reused with different sensors or models.

Response declarations describe what the experimenter intends to observe, rather than how a sensor
or simulation produces the signal. A response might represent a lick detector, eyelid closure,
approach velocity, gaze alignment, or an internal model variable. Keeping that binding outside the
trial syntax means the temporal design remains readable and portable.

A trial can contain named sampling windows. Responses are sampled once per simulation tick by
default:

```json
{
  "sampling": [
    {
      "name": "baseline",
      "onset": 0,
      "duration": 1.0,
      "responses": [
        {
          "response": "lick_response",
          "measurements": ["integral", "maximum"]
        }
      ]
    }
  ]
}
```

`onset` is relative to trial onset unless another reference is specified.

### Stimulus-relative sampling

```json
{
  "name": "conditioned_response",
  "relative_to": {
    "stimulus": "cs",
    "event": "onset"
  },
  "onset": 0.2,
  "duration": 1.5,
  "responses": [
    {
      "response": "lick_response",
      "measurements": [
        {
          "type": "latency",
          "threshold": 0.5,
          "direction": "rising"
        },
        "integral",
        "maximum"
      ]
    }
  ]
}
```

This window starts 0.2 seconds after onset of the presentation identified as `cs`. The reference
event can be `onset` or `offset`.

A negative window onset includes time before the reference event:

```json
{
  "name": "around_cs_offset",
  "relative_to": {
    "stimulus": "cs",
    "event": "offset"
  },
  "onset": -0.5,
  "duration": 1.5,
  "responses": []
}
```

### Reduced sampling rates

Sampling once per tick is the default. A window can sample every Nth tick:

```json
"sample_every": {
  "ticks": 5
}
```

or at a time interval:

```json
"sample_every": {
  "seconds": 0.1
}
```

Only one form may be used. Seconds are preferable when results must remain comparable across
different simulation tick durations.

## Measurements

### Latency

Latency is measured from the beginning of the sampling window to the first qualifying threshold
crossing:

```json
{
  "type": "latency",
  "threshold": 0.5,
  "direction": "rising"
}
```

| Direction | Meaning |
| --- | --- |
| `rising` | First transition from below to at or above the threshold. |
| `falling` | First transition from above to at or below the threshold. |
| `either` | First crossing in either direction. |
| `above` | First sample at or above threshold, without requiring an observed transition. |

If no qualifying event occurs, latency is `null`. Zero remains a valid latency.

### Integral or area under the curve

`integral` is the area under the response curve, calculated from actual timestamps using
trapezoidal integration:

```text
integral += 0.5 * (previous value + current value)
                  * (current time - previous time)
```

`area_under_curve` may be accepted as a verbose alias. An unweighted sample sum is deliberately not
used because it changes with tick duration; the integral is its time-independent counterpart.

### Maximum

`maximum` is the largest value sampled in the window. It is not weighted by tick duration, although
very brief peaks require adequate temporal resolution to be observed.

## Tick-duration independence

Protocol times describe continuous time in seconds, not numbers of ticks:

- A stimulus begins on the first tick at or beyond its scheduled onset.
- A stimulus ends on the first tick at or beyond its scheduled offset.
- Trial and interval boundaries follow the same rule.
- Recorded samples retain their actual simulation timestamps.
- Latency can interpolate between samples surrounding a threshold crossing.
- Integrals use actual elapsed time between samples.

Changing tick duration does not change the intended schedule or measurement units. Because output
can only change on a tick, an observed boundary may differ from its requested time by less than one
tick.

Tick-duration independence does not imply infinite temporal resolution. A shorter tick can resolve
faster response changes and estimate threshold crossings more precisely. The important guarantee is
that changing the tick duration does not silently reinterpret 1 second as a different physical
duration or turn an integral into a tick-count-dependent sum.

## Randomization and reproducibility

```json
{
  "seed": 12345
}
```

The seed controls random intervals, presentation probabilities, and context probabilities. All
random decisions are resolved when the protocol is loaded. The complete schedule can therefore be
inspected before execution and repeated with the same seed.

## Recording capacity

After randomization, the complete experiment duration is calculated recursively:

```text
trial span = trial duration + inter-trial interval
repeat span = repeat count * sum of nested item spans
protocol span = sum of top-level item spans
```

For recording once per tick:

```text
sample capacity = ceil(resolved protocol duration / tick duration) + 1
```

This lets the recorder allocate a fixed-size buffer at startup and retain every sample without a
rolling history. The extra sample accommodates the final protocol boundary.

The complete recording should include the experiment time, trial and repeat indices, trial-relative
time, active stimuli, active sampling windows, and every declared response. The display may show a
zoomed portion of this recording, but viewing a smaller interval does not discard earlier data.

## Organizing a larger experiment

A practical file is easiest to review when it follows a few conventions:

- Give stimuli names based on experimental roles, such as `tone_cs`, `food_us`, and `context_a`.
- Give every presentation referenced by a sampling window a short, unique `id`.
- Give trials names that identify phases or contingencies, such as `acquisition_cs_plus` and
  `extinction_cs_minus`.
- Put phase-wide context on the repeat block instead of copying it into every trial.
- Use a fixed seed while developing and analyzing a protocol.
- Keep baseline and response windows explicit, even when their boundaries coincide with stimulus
  boundaries.
- Prefer `integral` over an unweighted sum when experiments may use different tick durations.

These conventions are recommendations rather than additional syntax rules.

## Complete acquisition example

```json
{
  "version": 1,
  "seed": 12345,
  "metadata": {
    "name": "Partial-reinforcement acquisition",
    "condition": "80_percent_reinforcement"
  },
  "defaults": {
    "angle": 0,
    "reward": 0,
    "punishment": 0,
    "intensity": 1,
    "rgb": [1, 1, 1]
  },
  "stimului": {
    "room_light": {
      "rgb": [0.15, 0.15, 0.1]
    },
    "conditioned_stimulus": {
      "angle": -25,
      "rgb": [0, 0, 1],
      "components": ["vertical"]
    },
    "unconditioned_stimulus": {
      "angle": 25,
      "reward": 1,
      "rgb": [1, 1, 1],
      "components": ["dot"]
    }
  },
  "responses": {
    "conditioned_response": {}
  },
  "context": {
    "room": {
      "stimulus": "room_light",
      "angle": -75,
      "intensity": 0.4
    }
  },
  "protocol": [
    {
      "repeat": 10,
      "protocol": [
        {
          "trial": {
            "name": "acquisition",
            "duration": 4.0,
            "stimuli": [
              {
                "id": "cs",
                "stimulus": "conditioned_stimulus",
                "onset": 0.5,
                "duration": 2.0
              },
              {
                "id": "us",
                "stimulus": "unconditioned_stimulus",
                "inter_stimulus_interval": 1.5,
                "duration": 1.0,
                "probability": 0.8
              }
            ],
            "sampling": [
              {
                "name": "pre_cs_baseline",
                "onset": 0,
                "duration": 0.5,
                "responses": [
                  {
                    "response": "conditioned_response",
                    "measurements": ["integral", "maximum"]
                  }
                ]
              },
              {
                "name": "during_cs",
                "relative_to": {
                  "stimulus": "cs",
                  "event": "onset"
                },
                "onset": 0,
                "duration": 2.0,
                "responses": [
                  {
                    "response": "conditioned_response",
                    "measurements": [
                      {
                        "type": "latency",
                        "threshold": 0.5,
                        "direction": "rising"
                      },
                      "integral",
                      "maximum"
                    ]
                  }
                ]
              }
            ],
            "inter_trial_interval": {
              "min": 10.0,
              "max": 15.0
            }
          }
        }
      ]
    }
  ]
}
```

## Validation summary

A protocol is invalid when, for example:

- A referenced stimulus or response name does not exist.
- A sampling reference uses an unknown presentation `id`.
- A probability, color channel, or intensity lies outside 0 to 1.
- An interval is negative or a range has `min` greater than `max`.
- A presentation specifies both `onset` and `inter_stimulus_interval`.
- A presentation or sampling window extends outside its trial.
- A stimulus property remains `null` after all overrides are applied.
- A repeat count is negative or is not an integer.
- The protocol cannot be resolved to a finite duration.

Validation should report the JSON path and a plain-language explanation so the file can be corrected
without knowledge of the module implementation.
