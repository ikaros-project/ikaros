# RingWorldProtocol

## Protocol format manual

## Introduction

RingWorld Protocol describes classical-conditioning experiments in JSON. It specifies reusable
stimuli, trial timing, contextual stimuli, repeated blocks, probability, response measurements,
and optional bounded training that continues until a response criterion is met.

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
  "units": {},
  "metadata": {},
  "notes": "",
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
| `units` | Declares the physical units used by numeric protocol values. |
| `metadata` | Describes the experiment, subject, session, and experimental condition. |
| `notes` | Stores human-readable comments that do not affect execution. |
| `seed` | Makes random intervals and probability decisions reproducible. |
| `defaults` | Supplies values omitted from named stimuli. |
| `stimului` | Defines reusable, named stimulus templates. |
| `trials` | Defines reusable, named trial templates. |
| `responses` | Declares logical response signals that can be measured. |
| `context` | Defines stimuli active throughout the complete protocol. |
| `protocol` | Lists trials, repeated blocks, choices, and bounded criterion loops in execution order. |

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

## Units

Every complete version 1 file explicitly declares its units:

```json
{
  "units": {
    "time": "seconds",
    "angle": "degrees",
    "color": "normalized",
    "intensity": "normalized",
    "reinforcement": "normalized"
  }
}
```

The declarations apply throughout the file, including defaults, templates, overrides, randomized
values, sampling windows, and resolved schedules.

| Quantity | Required version 1 unit | Applies to |
| --- | --- | --- |
| `time` | `seconds` | Onsets, durations, ISIs, ITIs, sampling intervals, and latency. |
| `angle` | `degrees` | Gaze-relative and absolute ring positions. |
| `color` | `normalized` | Red, green, and blue channels from 0 to 1. |
| `intensity` | `normalized` | Visual and auditory intensity from 0 to 1. |
| `reinforcement` | `normalized` | Reward and punishment from 0 to 1. |

Version 1 does not perform unit conversion. A loader must reject `milliseconds`, `radians`, byte
colors, or other unit strings rather than silently reinterpret them. A future format version may add
conversion while preserving the physical meaning of existing protocols.

Measurement output uses the corresponding physical units. Latency is in seconds, maximum retains
the response signal's own unit, and an integral has response-unit seconds. Response-signal units
will be declared when logical response names are mapped to concrete signals.

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

## Notes

Standard JSON does not support comments. A `notes` field provides a safe place for explanations that
must travel with the protocol:

```json
{
  "notes": "The 80 percent reinforcement schedule reproduces pilot session 3."
}
```

`notes` may be a string or a list of strings:

```json
{
  "notes": [
    "Angles were selected from the generalization pilot.",
    "Do not compare raw maxima with sessions recorded below 100 Hz."
  ]
}
```

Notes are permitted at the top level and in metadata, stimulus definitions, trial templates,
protocol blocks, trials, presentations, contexts, response definitions, and sampling windows. They
are ignored when resolving timing and stimulus values, but retained in the resolved protocol and
experiment record with their JSON paths.

Notes must never be interpreted as instructions, expressions, or executable code. Use `factors`
for values needed in statistical grouping and `metadata` for searchable session identity. Use
`notes` for explanatory prose.

## A minimal protocol

This is the smallest useful conditioning example. It defines a blue vertical conditioned stimulus
(CS), a rewarding white unconditioned stimulus (US), and one trial in which the US begins 1.5
seconds after CS onset.

```json
{
  "version": 1,
  "units": {
    "time": "seconds",
    "angle": "degrees",
    "color": "normalized",
    "intensity": "normalized",
    "reinforcement": "normalized"
  },
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

The top-level `protocol` is an ordered list. Its items include trials and the protocol blocks
described later in this manual:

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

or sampled uniformly using the concise interval-range form:

```json
"inter_trial_interval": {
  "min": 10.0,
  "max": 15.0
}
```

The range form is also valid for `inter_stimulus_interval`. Bounds must be non-negative and `min`
must not exceed `max`. It is shorthand for the generalized `uniform` form described below. A new
value is selected for every trial execution.

Every presentation must fit inside `trial.duration`.

## Generalized random values

Most continuous numeric fields accept either a literal number or a random-value specification. This
allows the same notation to randomize timing, position, appearance, and reinforcement magnitude.
Structural integer fields such as `version`, `seed`, `repeat`, `count`, counterbalancing `sequence`,
and criterion repetition or history counts must remain literal integers.

### Uniform distribution

```json
{
  "onset": {
    "uniform": {
      "min": 0.4,
      "max": 0.8
    }
  },
  "angle": {
    "uniform": {
      "min": -45,
      "max": 45
    }
  }
}
```

`uniform` samples a continuous value between `min` and `max`. Equal bounds produce that fixed
value. The concise `{ "min": value, "max": value }` syntax is accepted only for interval fields;
using the explicit form elsewhere avoids confusing a random value with an ordinary object.

### Choice from discrete values

```json
{
  "duration": {
    "choice": [1.0, 1.5, 2.0]
  },
  "angle": {
    "choice": [-30, 0, 30]
  }
}
```

`choice` selects each listed number with equal probability. The list must be non-empty. Weighted
selection of complete protocol items uses a `choose` block instead.

### Bounded normal distribution

```json
{
  "angle": {
    "normal": {
      "mean": 0,
      "standard_deviation": 15,
      "min": -45,
      "max": 45
    }
  }
}
```

`standard_deviation` must be non-negative. `min` and `max` are optional. The sampled normal value is
clamped to supplied bounds, guaranteeing finite startup resolution even in the distribution tails.
A zero standard deviation produces the mean, subsequently clamped if necessary.

Generalized random values are valid for:

- Trial, presentation, context, sampling-window, ISI, and ITI times.
- Stimulus angle, reward, punishment, and intensity.
- Individual channels in an `rgb` list.
- Presentation and context probabilities.
- Choice-item weights.

For example, independently randomized color channels can be written as:

```json
{
  "rgb": [
    { "uniform": { "min": 0.5, "max": 1.0 } },
    0,
    { "choice": [0, 0.25, 0.5] }
  ]
}
```

Each specification is sampled once for every resolved occurrence of the object containing it. A
stimulus-template random value is therefore resampled for each presentation, while a repeat-context
random value is sampled once on entry and retained throughout that repeat scope. Validation of
ranges such as intensity from 0 to 1 occurs after sampling and clamping.

All random values are resolved at startup from the shared protocol seed and written into the
resolved schedule. No random-number generation occurs in the per-tick execution path.

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

## Randomized and counterbalanced order

Fixed repeats preserve the written order. A `randomized` block instead creates the requested number
of each item and shuffles the resulting block:

```json
{
  "randomized": {
    "items": [
      {
        "trial": {
          "template": "cs_plus"
        },
        "count": 10
      },
      {
        "trial": {
          "template": "cs_minus"
        },
        "count": 10
      }
    ],
    "constraints": {
      "maximum_consecutive_same": 2
    }
  }
}
```

Each entry under `items` contains one protocol item and a non-negative integer `count`. The block is
expanded to the requested multiset and shuffled using the top-level seed. Nested repeat, randomized,
counterbalanced, and choice blocks may be used as items.

Supported version 1 constraints are:

| Constraint | Meaning |
| --- | --- |
| `maximum_consecutive_same` | Largest permitted run of items having the same trial template or item identity. |
| `avoid_immediate_repetition` | Shorthand requiring adjacent items to differ. |
| `first` | Trial template or item identity that must occur first. |
| `last` | Trial template or item identity that must occur last. |

Constraints are requirements, not preferences. If no valid order exists, protocol loading fails
with an explanation rather than silently relaxing a constraint. The final resolved order is stored
with the experiment record.

### Counterbalanced blocks

A `counterbalanced` block selects a systematic order rather than a random order. A Latin square
rotates which condition appears in each ordinal position:

```json
{
  "counterbalanced": {
    "method": "latin_square",
    "sequence": 2,
    "repetitions": 5,
    "items": [
      {
        "trial": {
          "template": "condition_a"
        }
      },
      {
        "trial": {
          "template": "condition_b"
        }
      },
      {
        "trial": {
          "template": "condition_c"
        }
      }
    ]
  }
}
```

`sequence` is zero-based and selects one row of the Latin square. It is normally assigned from the
subject or session outside the reusable protocol. `repetitions` repeats the selected row and
defaults to one. With three items, the Latin-square rows are conceptually `A B C`, `B C A`, and
`C A B`.

For published or historically established orders, an explicit counterbalancing table can be used:

```json
{
  "counterbalanced": {
    "method": "explicit",
    "sequence": 1,
    "items": [
      { "trial": { "template": "condition_a" } },
      { "trial": { "template": "condition_b" } },
      { "trial": { "template": "condition_c" } }
    ],
    "orders": [
      [0, 1, 2],
      [2, 1, 0]
    ]
  }
}
```

Order indices refer to `items`. Every explicit order must contain valid indices and must satisfy the
study's intended replication rule. Counterbalanced order is deterministic and does not consume the
random-number stream unless a nested item itself contains random values.

## Random choice

A `choose` block selects complete protocol items according to relative weights:

```json
{
  "choose": {
    "count": 20,
    "with_replacement": true,
    "items": [
      {
        "weight": 0.7,
        "trial": {
          "template": "reinforced"
        }
      },
      {
        "weight": 0.3,
        "trial": {
          "template": "nonreinforced"
        }
      }
    ]
  }
}
```

`count` is the number of selections and must be a non-negative integer. `weight` must be
non-negative, and at least one eligible item must have positive weight. Weights are relative and do
not need to sum to one. With replacement, the same item may be selected repeatedly and every draw
uses the original weights.

When `with_replacement` is false, each selected item is removed from the candidate set. In that
case, `count` cannot exceed the number of positive-weight items:

```json
{
  "choose": {
    "count": 2,
    "with_replacement": false,
    "items": [
      { "weight": 1, "trial": { "template": "probe_a" } },
      { "weight": 1, "trial": { "template": "probe_b" } },
      { "weight": 1, "trial": { "template": "probe_c" } }
    ]
  }
}
```

Choice differs from presentation probability. A failed presentation-probability decision preserves
the trial and its timing but omits one stimulus. A `choose` block determines which complete
protocol item is executed. Trials, repeats, randomized blocks, counterbalanced blocks, and nested
choice blocks are all valid choice items.

All choices are resolved at startup from the protocol seed. The selected item identities and draw
order are stored in the resolved schedule and experiment record.

## Contextual stimuli

`context` may occur at the top level, in a repeated or `until` block, or in a trial. Context entries
are named so that inner scopes can override them:

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
- A repeat or `until` context covers every nested iteration and its inter-trial intervals.
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

## Trial and phase factors

`factors` attach categorical or numeric analysis labels without changing protocol execution. They
may be placed at the top level, on a repeat, randomized, counterbalanced, choice, or `until` block,
in a trial template, or in an individual trial:

```json
{
  "factors": {
    "study": "renewal"
  },
  "protocol": [
    {
      "repeat": 10,
      "factors": {
        "phase": "acquisition",
        "context_condition": "A"
      },
      "protocol": [
        {
          "trial": {
            "template": "cs_plus",
            "factors": {
              "contingency": "CS+",
              "probe": false
            }
          }
        }
      ]
    }
  ]
}
```

Factors are inherited from outer to inner scopes. A factor with the same name at an inner scope
replaces the inherited value:

```text
top-level factors -> enclosing block factors -> trial-template factors -> trial factors
```

Factor values may be strings, numbers, booleans, or null. They are copied into every trial record,
sampling-window result, and exported measurement row. This allows statistics to group results by
phase, contingency, context condition, probe status, or other experimental variables without
recovering those categories from trial names.

Factors do not activate stimuli and must not be confused with `context`. A context changes what is
presented to the subject. A factor labels the design or resulting data. It is common for a repeat
block to contain both:

```json
{
  "repeat": 10,
  "context": {
    "chamber": {
      "stimulus": "context_b"
    }
  },
  "factors": {
    "phase": "extinction",
    "physical_context": "B"
  },
  "protocol": []
}
```

Randomized and chosen items retain their own resolved factors. The recorder also stores the full
inherited factor set with the resolved trial, so later changes to the original protocol file cannot
alter the meaning of existing results.

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

## Training until a response criterion

An `until` protocol item repeats a nested protocol until a recorded response measurement satisfies
a criterion. This is the protocol's only response-dependent control structure:

```json
{
  "until": {
    "minimum_repetitions": 5,
    "maximum_repetitions": 50,
    "criterion": {
      "source": {
        "trial": "acquisition_cs_plus",
        "window": "during_cs",
        "response": "conditioned_response",
        "measurement": "maximum"
      },
      "operator": ">=",
      "value": 0.7
    },
    "protocol": [
      {
        "trial": {
          "template": "acquisition_cs_plus"
        }
      }
    ]
  }
}
```

One repetition means one complete execution of the nested `protocol`. The criterion is evaluated
after that execution and after all of its sampling windows have been finalized. Training stops when
the criterion is satisfied and at least `minimum_repetitions` have completed. It otherwise stops
after `maximum_repetitions` and records that the criterion was not reached.

`maximum_repetitions` is mandatory and must be a positive integer. `minimum_repetitions` defaults
to one, must be non-negative, and must not exceed the maximum. An `until` block may occur anywhere a
repeat or trial item can occur, including inside another bounded block.

The criterion `source` identifies one measurement result by resolved trial name, sampling-window
name, response name, and measurement type. Version 1 requires this source to produce exactly one
result during every repetition of the nested protocol. This deliberately excludes ambiguous cases
where the named trial can execute zero times or several times in one repetition. The response must
be bound to a concrete signal before execution begins.

Supported comparison operators are `<`, `<=`, `>`, and `>=`. Exact equality is not supported for
floating-point response measurements. `value` has the unit of the selected measurement and may be
a literal or a generalized random value; when randomized, one value is pre-resolved for each
possible `until` block occurrence and retained throughout that block execution.

### Criteria over recent repetitions

A noisy response can be evaluated over a recent history:

```json
{
  "criterion": {
    "source": {
      "trial": "acquisition_cs_plus",
      "window": "during_cs",
      "response": "conditioned_response",
      "measurement": "integral"
    },
    "history": {
      "last": 5,
      "aggregate": "mean"
    },
    "operator": ">=",
    "value": 1.2
  }
}
```

`last` is a positive integer. Version 1 supports `mean`, `minimum`, and `maximum` as history
aggregates. The criterion is not eligible to pass until all requested history observations exist.
The observations are the selected measurement from the most recent repetitions of this `until`
block, not measurements from earlier blocks.

The result can also be required to persist across evaluations:

```json
{
  "criterion": {
    "source": {
      "trial": "acquisition_cs_plus",
      "window": "during_cs",
      "response": "conditioned_response",
      "measurement": "maximum"
    },
    "operator": ">=",
    "value": 0.7,
    "consecutive": 3
  }
}
```

`consecutive` defaults to one and must be a positive integer. A failed evaluation resets the
consecutive-pass count. `minimum_repetitions`, history availability, and `consecutive` must all be
satisfied before the block can end early.

Every evaluation is stored with its repetition index, source measurement, aggregated value,
threshold, operator, pass state, and consecutive-pass count. The block termination record states
whether the criterion was reached or the maximum was exhausted. A non-finite runtime measurement
cannot satisfy a criterion, resets persistence, and produces a warning; a missing response binding
or structurally unresolvable source is a startup error.

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

The seed controls random intervals, presentation probabilities, context probabilities, randomized
order, choices, and randomized criterion thresholds. All random decisions are resolved when the
protocol is loaded, expanding every `until` block to its maximum possible repetitions. The complete
maximum schedule can therefore be inspected before execution and repeated with the same seed. An
`until` block executes a response-dependent prefix of that pre-resolved schedule; its actual stopping
point is recorded and is reproducible only when the response stream is also reproduced.

## Recording capacity

After randomization, the complete experiment duration is calculated recursively:

```text
trial span = trial duration + inter-trial interval
repeat span = repeat count * sum of nested item spans
until maximum span = maximum repetitions * sum of nested item maximum spans
protocol span = sum of top-level item spans
```

For recording once per tick:

```text
sample capacity = ceil(resolved protocol duration / tick duration) + 1
```

This lets the recorder allocate a fixed-size buffer at startup and retain every sample without a
rolling history. For an `until` block the allocation uses its worst-case duration, although execution
may stop earlier. The extra sample accommodates the final protocol boundary.

The complete recording should include the experiment time, trial and repeat indices, trial-relative
time, active stimuli, active sampling windows, every declared response, criterion evaluations, and
block-termination reasons. The display may show a zoomed portion of this recording, but viewing a
smaller interval does not discard earlier data.

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
- Give every response-dependent block a scientifically justified maximum rather than treating the
  maximum merely as a technical safeguard.

These conventions are recommendations rather than additional syntax rules.

## Complete acquisition example

```json
{
  "version": 1,
  "units": {
    "time": "seconds",
    "angle": "degrees",
    "color": "normalized",
    "intensity": "normalized",
    "reinforcement": "normalized"
  },
  "seed": 12345,
  "metadata": {
    "name": "Partial-reinforcement acquisition",
    "condition": "80_percent_reinforcement"
  },
  "notes": "Ten acquisition trials with an 80 percent US schedule.",
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

- The protocol version is missing or unsupported.
- A required unit declaration is missing or uses an unsupported unit.
- A referenced stimulus or response name does not exist.
- A sampling reference uses an unknown presentation `id`.
- A probability, color channel, or intensity lies outside 0 to 1.
- An interval is negative or a range has `min` greater than `max`.
- A presentation specifies both `onset` and `inter_stimulus_interval`.
- A presentation or sampling window extends outside its trial.
- A stimulus property remains `null` after all overrides are applied.
- A repeat count is negative or is not an integer.
- An `until` block lacks a positive integer `maximum_repetitions`, its minimum exceeds its maximum,
  or its source does not identify exactly one measurement per repetition.
- A criterion uses an unsupported operator, an invalid history or persistence count, or an unbound
  response.
- The protocol cannot be resolved to a finite duration.

Validation should report the JSON path and a plain-language explanation so the file can be corrected
without knowledge of the module implementation.

## Module interface

The initial C++ implementation executes fixed trials, recursively nested `repeat` blocks, and
bounded `until` blocks containing one resolved trial per repetition. It
supports named stimulus and trial templates, top-level and block/trial contexts, explicit and
onset-to-onset presentation timing, presentation probabilities, generalized numeric random values,
trial-relative sampling windows, and finite inter-trial intervals. Other documented protocol blocks
are rejected until their corresponding implementation stage is complete.

## Parameters

| Parameter | Description |
| --- | --- |
| `filename` | Version 1 protocol JSON file, resolved relative to the loaded model and then UserData. |
| `max_stimuli` | Fixed number of output rows available for simultaneously active visual stimuli. |
| `max_sampling_windows` | Fixed number of sampling-window activity channels. |

## Inputs

| Input | Description |
| --- | --- |
| `CRITERION_MET` | Optional result from `RingWorldResponseAnalysis`; a passing result ends an eligible bounded `until` block. |

## Outputs

| Output | Description |
| --- | --- |
| `STIMULI` | Active RingWorld stimulus rows; unused rows have zero intensity. |
| `ACTIVE_STIMULI` | Number of active rows at the beginning of `STIMULI`. |
| `SAMPLE_WINDOWS` | Activity flags for sampling windows in the current trial. |
| `PROTOCOL_TIME` | Elapsed protocol time in seconds. |
| `TRIAL_TIME` | Elapsed current-trial time in seconds, or zero outside a trial. |
| `TRIAL_INDEX` | Zero-based resolved trial index, or -1 after completion. |
| `TRIAL_ACTIVE` | One during the trial period and zero during an ITI or after completion. |
| `COMPLETED` | One after the finite schedule has completed. |
| `UNTIL_ACTIVE` | One while a bounded `until` block is executing. |
| `UNTIL_REPETITION` | One-based repetition within the current `until` block. |
