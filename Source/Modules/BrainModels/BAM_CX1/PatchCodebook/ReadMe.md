# PatchCodebook

Shared competitive patch prototypes with local learning and soft coding.

Each tick performs one evaluation. Structural dimensions are fixed at startup. All image batches use field, channel (when present), row, column order. Persistent matrices use native Ikaros state loading and saving. No model files are downloaded automatically.

See the `.ikc` descriptor for ports and parameters. Inputs and outputs are finite floating-point arrays. Unconnected optional inputs disable the corresponding intervention.
