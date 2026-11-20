# CategoryCompetition

Recurrent inhibitory competition with optional category drive.

Each tick performs one evaluation. Structural dimensions are fixed at startup. All image batches use field, channel (when present), row, column order. Persistent matrices use native Ikaros state loading and saving. No model files are downloaded automatically.

See the `.ikc` descriptor for ports and parameters. Inputs and outputs are finite floating-point arrays. Unconnected optional inputs disable the corresponding intervention.

`HOLD` optionally freezes the recurrent state while positive, overriding `RESET`. A one-tick hold can populate delayed connections after loading initial states. Competition still reports normalized weights from the held activity.
