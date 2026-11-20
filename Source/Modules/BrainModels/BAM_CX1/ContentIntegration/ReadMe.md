# ContentIntegration

Recurrent mixture of sensory evidence and top-down prediction.

Each tick performs one evaluation. Structural dimensions are fixed at startup. All image batches use field, channel (when present), row, column order. Persistent matrices use native Ikaros state loading and saving. No model files are downloaded automatically.

See the `.ikc` descriptor for ports and parameters. Inputs and outputs are finite floating-point arrays. Unconnected optional inputs disable the corresponding intervention.

`HOLD` optionally freezes the recurrent state while positive, overriding `RESET`. A one-tick hold can populate delayed connections after loading initial states.

## Recurrent scheduling

Use a one-tick connection when an input must read the preceding population state. Use zero delay only for dependencies within the current update. After loading persistent outputs, hold the recurrent populations for one tick to fill delayed connections before counting model updates. `HOLD` has no effect when disconnected.

The synthetic regression test in `tests/test_native_recurrence.py` verifies a loaded nonzero state, an explicit initialization tick and the subsequent delayed mixture trajectory. No kernel changes are needed.
