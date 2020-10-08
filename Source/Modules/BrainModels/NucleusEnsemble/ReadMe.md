# NucleusEnsemble


<br><br>
## Short description



<br><br>

## Inputs

|Name|Description|Optional|
|:----|:-----------|:-------|
|EXCITATION|The excitatory input|Yes|
|INHIBITION|The inhibitory input|Yes|
|SHUNTING_INHIBITION|The shunting inhibitory input|Yes|
|DOPAMINE|Lowers threshold to make excitation more likely - dopamine|Yes|
|NORADRENALINE|Inhibitive below NA threshold, excitative over (Maletic et al., 2017). |Yes|
|NA_THRESHOLD|NA threshold, should be [0,1] and simulates proportion of high affinity inhibitive receptors vs low affinity excitatory ones (Maletic et al., 2017). Default value=0.5|Yes|
|ADENO_INPUT|Inhibits threshold lowering - adenosine|Yes|
|SETPOINT|Activity setpoint - weights will be changed to maintain this. Ignored if not connected|Yes|
|ADAPTATIONRATE|Learning rate for weights when using activity setpoint|Yes|
|EX_TOPOLOGY|Optional external topology overrides topology settings|Yes|
|INH_TOPOLOGY|Optional external topology overrides topology settings|Yes|
|SH_INH_TOPOLOGY|Optional external topology overrides topology settings|Yes|

<br><br>

## Outputs

|Name|Description|
|:----|:-----------|
|OUTPUT|The output from the nucleus|
|ADENO_OUTPUT|The adenosine (metabolic waste) output from the nucleus|
|THRESHOLD|The activation threshold of the nucleus|
|TETANUS|The buildup to activation threshold of the nucleus|

<br><br>

## Parameters

|Name|Description|Type|Default value|
|:----|:-----------|:----|:-------------|
|debug|Whether to print debug info|boolean|no|
|size|Size of ensemble|int|1|
|alpha|Bias term / offset level|float|0|
|beta|Activity scaling|float|1|
|phi|Excitation weight|float|1.0|
|chi|Inhibition weight|float|1.0|
|psi|Shunting weight|float|1.0|
|tau|Recursion weight|float|1.0|
|sigma|Dopamine weight|float|1.0|
|rho|Adenosine weight|float|1.0|
|scale|Scale by number of inputs|bool|yes|
|threshold|Default threshold for activity. this is a positive number, which corresponds to the input activity necessary to elicit action potentials. it is not directly translateable to the threshold value of -55mv of rate neurons, however.|float|0.0|
|epsilon|How quick to adapt to change|float|0.1|
|tetanus_leak|How quickly current leaks from buildup (tetanus) to activity|float|1|
|tetanus_growth|How much input contributes to buidup (tetanus) to activity|float|1|
|excitation_topology|How exc input is connected to ensemble nuclei|list|one_to_one|
|inhibition_topology|How inh input is connected to ensemble nuclei|list|one_to_one|
|shunting_inhibition_topology|How inh input is connected to ensemble nuclei|list|one_to_one|
|recurrent_topology|How inh input is connected to ensemble nuclei|list|none|
|activation_function|Function applied to output|list|scaledsigmoid|
|moving_avg_window_size|Size of moving avg window used for adaptation to set point|int|100|

<br><br>
## Long description
Ensemble of nuclei, where output is an array with size equal to number of nuclei.
		Output = alpha + beta * [1/(1+delta*ShuntingInhibition)] *SUM(Excitation) - gamma*SUM/Inhibition).
        
        If not set, beta and gamma are set to 1/N, where N are the number of connected inputs.

		TODO:
		- ok add input: SETPOINT - weights should change so activity reaches this
		- ok add input: ADAPTIONRATE - a kind of learning rate determining rate of adaptation to set point (ie acetylcholine receptors)