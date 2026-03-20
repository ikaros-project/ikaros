# Protocol

## Description

Generate experiment stimuli. Protocol is intended as a compact way to define experiment timing
inside an Ikaros model. Its parameters describe conditioned-stimulus and unconditioned-stimulus
durations together with inter-stimulus and inter-trial intervals, and the module exposes STIMULUS
and REWARD outputs as the places where that schedule would be driven during execution.

It produces STIMULUS and REWARD while parameters such as CS_duration, US_duration, ISI, ITI, and
ITI_variation shape its behavior. A good use case is classical-conditioning or reinforcement-
learning studies where cue timing, reward delivery, and inter-trial structure need to be controlled
precisely while downstream brain-inspired modules learn predictive associations.

Timed cue-and-reward structure is fundamental in conditioning, reinforcement learning, and many
behavioral paradigms. A protocol module is therefore most valuable when the temporal arrangement of
stimuli is itself part of the hypothesis, such as in prediction-error studies, extinction and
renewal experiments, or robot training tasks where delayed outcomes must be linked back to earlier
signals.

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| CS_duration | Duration of conditioned stimulus in seconds | float | 0.1 |
| US_duration | Duration of unconditioned stimulus in seconds | float | 0.1 |
| ISI | Inter-stimulus interval | float | 0.1 |
| ITI | Inter-trial interval | float | 1.0 |
| ITI_variation | Variation in inter-trial interval | float | 1.0 |
| trials | Number of trials in each phase | int | 10 |

## Outputs

| Name | Description |
| --- | --- |
| STIMULUS | Stimulus |
| REWARD | The reward |

*This description was automatically created and may not describe the full function of the module.*
