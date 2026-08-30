# Potential Ikaros Errors To Investigate

## 1. Relative state filenames passed to `-W` may resolve unexpectedly

Observed while running a CVAE evaluation from `/Users/cba/ikaros`:

```bash
/Users/cba/ikaros/Bin/ikaros -b -s 5000 \
    -W UserData/cvae_hierarchy_evaluation/moving_blob_trained.state \
    UserData/ConvolutionalVariationalAutoEncoder_hierarchy_moving_blob_graph_test.ikg
```

The model executed, but state saving failed during shutdown:

```text
Could not open state file "UserData/cvae_hierarchy_evaluation/moving_blob_trained.state" for writing.
```

Using an absolute state path worked:

```bash
-W /Users/cba/ikaros/UserData/cvae_hierarchy_evaluation/moving_blob_trained.state
```

This may be expected behavior, but then it should be documented. It may also be a bug if users are expected to use project-relative or `UserData`-relative paths consistently across model files, input files, output files, and state files.

Suggested follow-up: decide whether command-line `-W` and `-L` paths should be resolved through the same project/UserData path handling used elsewhere.

## 2. Nested command-line parameter overrides did not appear to affect `.ikg` component parameters

Observed command-line overrides such as:

```bash
ConvolutionalVariationalAutoEncoder_hierarchy_moving_blob_graph_test.Movie.filename=cvae_hierarchy_moving_blob/moving_blob_family.mp4
ConvolutionalVariationalAutoEncoder_hierarchy_moving_blob_graph_test.Metrics.filename=moving_blob_family_metrics.csv
```

The model started and ran, but the expected overridden `OutputFile` target was not produced. The run appeared to use the filename from the `.ikg` file instead.

This may mean that nested `.ikg` component parameter overrides are unsupported, require different syntax, or are applied too late/at the wrong dictionary level. In any case, batch evaluation would benefit from either support or clear documentation.

Suggested follow-up: add a small command-line override regression test for nested component parameters, then either fix the behavior or document the supported override syntax.

## 3. Persistent state paths include the top-level group name

Persistent state items are stored under full component paths, including the top-level group name. A state file saved from:

```text
ConvolutionalVariationalAutoEncoder_hierarchy_moving_blob_graph_test.CVAE_TopDown_Level1
```

did not load into an otherwise identical held-out `.ikg` when the root group name differed. The load failed because the saved state item path did not match a persistent private state in the loaded model.

This is internally consistent, but it is surprising when two `.ikg` files intentionally have the same module names and architecture but different file/root group names.

Suggested follow-up: decide whether this behavior should remain strict, be documented more clearly, or support an optional state-loading mode that maps by relative path below the root group.

## 4. Batch runs may warn about failed session logging when offline

Observed during local batch smoke tests:

```text
Session logging failed: Couldn't resolve host name. Further failures will be suppressed until delivery recovers.
```

The model itself continued and completed successfully. This may be expected when the machine is offline or the logging endpoint is unreachable, but the warning can look like an Ikaros setup/runtime problem even when it is unrelated to model execution.

Suggested follow-up: clarify whether session logging is optional in batch mode, and consider making the message explicitly say that local simulation will continue unaffected.

## 5. WebUI fatal delay-history rotation test can fail to stop the kernel

Observed while running the full kernel test suite after CVAE module changes:

```text
[ FAIL ] Stop and reload after a fatal delay-history rotation failure during a WebUI step - test_239_webui_fatal_rotation_step.ikg
(Fatal WebUI step did not stop the kernel: state=2, tick=1)
```

The surrounding kernel tests and the CVAE smoke tests passed. This appears unrelated to the CVAE change, but it may indicate a timing-sensitive WebUI/kernel recovery issue in the fatal-step path.

Suggested follow-up: rerun `test_239_webui_fatal_rotation_step.ikg` repeatedly in isolation and inspect whether state `2` is a transient running/stopping state that the test polls too early, or whether the kernel sometimes fails to enter the expected stopped state after the fatal rotation.
