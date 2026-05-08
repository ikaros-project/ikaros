# SalienceIntegrator

`SalienceIntegrator` combines one or more salience maps into a persistent master map.

Before each update, `OUTPUT` is multiplied by `decay`. Each salience map in `INPUT`
is then multiplied by the corresponding value in `factors` and added to `OUTPUT`.

When several modules are connected to `INPUT`, Ikaros stacks them along the first
dimension. For example, two `4 x 4` salience maps connected to `INPUT` are read as
`INPUT[0]` and `INPUT[1]`, and the output remains `4 x 4`.

Missing factor values default to `1`.
