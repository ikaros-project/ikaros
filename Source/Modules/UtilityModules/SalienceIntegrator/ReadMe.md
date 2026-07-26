# SalienceIntegrator

`SalienceIntegrator` combines one or more salience maps into a persistent master map.

Before each update, `OUTPUT` is multiplied by `decay`. Each salience map in `INPUT`
is then multiplied by the corresponding value in `factors` and added to `OUTPUT`.

When several modules are connected to `INPUT`, Ikaros stacks them along the first
dimension. For example, two `4 x 4` salience maps connected to `INPUT` are read as
`INPUT[0]` and `INPUT[1]`, and the output remains `4 x 4`.

Missing factor values default to `1`.

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| factors | Scale factor for each input map; missing factors default to 1 | matrix | 1 |
| decay | Factor applied to the previous OUTPUT before adding the current inputs | number | 0.9 |

## Inputs

| Name | Description | Optional |
| --- | --- | --- |
| INPUT | One or more salience maps; stack adds the first dimension as the map index |  |

## Outputs

| Name | Description |
| --- | --- |
| OUTPUT | Integrated salience map |
