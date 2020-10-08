# Modules
* ANN/
  * [Autoassociator](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/ANN/Autoassociator/) : Template for general learming modules
  * [BackProp](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/ANN/BackProp/) : Basic multi-layer perceptron
  * CSOM/
    * [CSOM_PCA](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/ANN/CSOM/CSOM_PCA/) : Self-organizing convolution map
  * [Perceptron](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/ANN/Perceptron/) : Single layer of perceptrons
  * [RNN](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/ANN/RNN/) : Recurrent neural network - template module
* BrainModels/
  * DecisionModel2020/
    * [MemorySequencer](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/BrainModels/DecisionModel2020/MemorySequencer/) : Implements a memory sequencer
    * [Perception](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/BrainModels/DecisionModel2020/Perception/) : Implements a perception
    * [ValueAccumulator](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/BrainModels/DecisionModel2020/ValueAccumulator/) : Implements a valueaccumulator
  * [EyeModel](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/BrainModels/EyeModel/) : Models an eye
  * Moren2002/
    * [M02_Amygdala](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/BrainModels/Moren2002/M02_Amygdala/) : An amygdala model
    * [M02_Cortex](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/BrainModels/Moren2002/M02_Cortex/) : A cortex model
    * [M02_Hippocampus](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/BrainModels/Moren2002/M02_Hippocampus/) : A hippocampus model
    * [M02_OFC](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/BrainModels/Moren2002/M02_OFC/) : An model of orbitofrontal cortex
    * [M02_SensoryCortex](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/BrainModels/Moren2002/M02_SensoryCortex/) : A (naive) cortex model
    * [M02_Thalamus](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/BrainModels/Moren2002/M02_Thalamus/) : A thalamus model
  * [NeuralArray](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/BrainModels/NeuralArray/) : Neuralarray
  * [NeuralField](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/BrainModels/NeuralField/) : Neuralfield
  * [NucleusEnsemble](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/BrainModels/NucleusEnsemble/) : Ensemble of nuclei with support for dopamine, adenosine, noradrenaline
  * [Nucleus](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/BrainModels/Nucleus/) : Implements a nucleus
  * [SpikingPopulation](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/BrainModels/SpikingPopulation/) : Simulates a population of spiking neurons
* CodingModules/
  * [CoarseCoder](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/CodingModules/CoarseCoder/) : Encodes a two-dimensional value in a vector
  * [IntervalCoder](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/CodingModules/IntervalCoder/) : Encodes a value in a vector
  * [IntervalDecoder](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/CodingModules/IntervalDecoder/) : Decodes an interval coded vector
  * [PopulationCoder](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/CodingModules/PopulationCoder/) : Encodes a two-dimensional value in a vector
  * [PopulationDecoder](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/CodingModules/PopulationDecoder/) : Encodes a two-dimensional value in a vector
  * [SpectralTiming](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/CodingModules/SpectralTiming/) : Encodes a signal in a set of temporal components
  * [TappedDelayLine](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/CodingModules/TappedDelayLine/) : Encodes a signal as a time series
* ControlModules/
  * [KalmanFilter](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/ControlModules/KalmanFilter/) : A standard kalman filter
  * [PIDController](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/ControlModules/PIDController/) : Standard pid controller
* EnvironmentModules/
  * [GLRobotSimulator](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/EnvironmentModules/GLRobotSimulator/) : Simulates a robot
  * [GridWorld](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/EnvironmentModules/GridWorld/) : Simple agent and world simulator
  * [MazeGenerator](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/EnvironmentModules/MazeGenerator/) : Generates a perfect maze
  * [PlanarArm](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/EnvironmentModules/PlanarArm/) : Simulation of a simple arm and a target object
* IOModules/
  * EyeTracker/
    * [TheEyeTribe](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/IOModules/EyeTracker/TheEyeTribe/) : Module that uses the eye tribe api
  * FileInput/
    * [InputFile](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/IOModules/FileInput/InputFile/) : Reads a text file
    * [InputJPEG](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/IOModules/FileInput/InputJPEG/) : Reads jpeg files
    * [InputPNG](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/IOModules/FileInput/InputPNG/) : Reads png files
    * [InputRawImage](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/IOModules/FileInput/InputRawImage/) : Reads images raw format
    * [InputVideoFile](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/IOModules/FileInput/InputVideoFile/) : Reads a video file using ffmpeg
  * FileOutput/
    * [OutputFile](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/IOModules/FileOutput/OutputFile/) : Generates a text file
    * [OutputJPEG](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/IOModules/FileOutput/OutputJPEG/) : Writes jpeg files
    * [OutputPNG](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/IOModules/FileOutput/OutputPNG/) : Writes png files
    * [OutputRawImage](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/IOModules/FileOutput/OutputRawImage/) : Writes images in raw format
    * [OutputVideoFile](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/IOModules/FileOutput/OutputVideoFile/) : Save to a video file
  * Network/
    * [IPCClient](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/IOModules/Network/IPCClient/) : Client for receiving inter-process communication (IPC)
    * [IPCServer](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/IOModules/Network/IPCServer/) : Module to send data using sockets
    * [YARPPort](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/IOModules/Network/YARPPort/) : Ikaros to yarp network communication
  * [SoundOutput](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/IOModules/SoundOutput/) : Plays a sound file
  * [TouchBoard](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/IOModules/TouchBoard/) : Get data from bare conductive touch board
  * Video/
    * [InputVideoAV](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/IOModules/Video/InputVideoAV/) : Grabs images from quicktime camera
    * [InputVideoStream](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/IOModules/Video/InputVideoStream/) : Grabs video using ffmpeg
    * [InputVideo](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/IOModules/Video/InputVideo/) : Grabs video using ffmpeg
    * [Kinect](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/IOModules/Video/Kinect/) : Grabs images from kinect or xtion
    * [NetworkCamera](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/IOModules/Video/NetworkCamera/) : Grabs images from network camera
* LearningModules/
  * [Delta](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/LearningModules/Delta/) : Learning using the delta rule
  * [KNN](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/LearningModules/KNN/) : (k nearest neighbors) using a kd-tree
  * [KNN_Pick](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/LearningModules/KNN_Pick/) : Chooses a class based on some elements (neighbors)
  * [LearningModule](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/LearningModules/LearningModule/) : Template for general learming modules
  * [LinearAssociator](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/LearningModules/LinearAssociator/) : Learns a linear mapping
  * [LocalApproximator](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/LearningModules/LocalApproximator/) : Chooses a class based on some elements (neighbors)
  * [QLearning](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/LearningModules/QLearning/) : Simple q-learning
  * [Trainer](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/LearningModules/Trainer/) : Trains a learning module
* RobotModules/
  * [ActionCompetition](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/RobotModules/ActionCompetition/) : Maintains activation levels for a number iof actions
  * [Dynamixel/DynamixelConfigure](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/RobotModules/Dynamixel/DynamixelConfigure/) : Configures dynamixel servos
  * [Dynamixel](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/RobotModules/Dynamixel/) : Interfaces dynamixel servos
  * [Epuck](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/RobotModules/Epuck/) : Interfaces the e-puck robot
  * [FadeCandy](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/RobotModules/FadeCandy/) : Allows control of NeoPixels hardware through FadeCandy server
  * [GazeController](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/RobotModules/GazeController/) : Controls a 4 dof stereo head
  * [MotionGuard](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/RobotModules/MotionGuard/) : Module to prevent suddden and dangerous movements
  * [MotionRecorder](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/RobotModules/MotionRecorder/) : Module that records a sequence of values and saves them to a file
  * [Phidgets](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/RobotModules/Phidgets/) : Controlling a phidgets ioboard
  * [SSC32](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/RobotModules/SSC32/) : Interfaces the ssc-32 rc servos
  * [SequenceRecorder](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/RobotModules/SequenceRecorder/) : Records a sequence
  * [ServoConnector](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/RobotModules/ServoConnector/) : Organize servos
* UtilityModules/
  * [Abs](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Abs/) : Rectifies a signal
  * [Add](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Add/) : Adds two inputs
  * [Arbiter](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Arbiter/) : Selects between multiple inputs
  * [ArgMax](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/ArgMax/) : Selects maximum element
  * [ArrayToMatrix](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/ArrayToMatrix/) : Split an input array and concatenate a output matrix
  * [Average](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Average/) : Calculates average over time
  * [BlackBox](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/BlackBox/) : Black box module with parameterized io; functions as an operad
  * [Concat](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Concat/) : Concatenate inputs into array
  * [Constant](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Constant/) : Outputs a constant value
  * [Counter](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Counter/) : Counts signals
  * [DataBuffer](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/DataBuffer/) : A data buffer
  * [DataConverter](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/DataConverter/) : Converts between rotation notations
  * [DecomposeTransform](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/DecomposeTransform/) : Decompose a transform into components
  * [Delay](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Delay/) : Delays a signal
  * [Distance](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Distance/) : Calculates a distance
  * [Divide](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Divide/) : Divides the first input with the second
  * [EventTrigger](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/EventTrigger/) : Triggers events
  * [Expression](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Expression/) : Mathematical expression
  * [FFT](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/FFT/) : Fast Fourier transform
  * [FanIn](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/FanIn/) : Multiplexer for CSOM networks
  * [FanOut](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/FanOut/) : De-multiplexer for use with CSOM networks
  * [Flip](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Flip/) : Flips a signal
  * [FunctionGenerator](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/FunctionGenerator/) : Functions of times
  * [Fuse](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Fuse/) : Fuses streams of coordinate transformations
  * [Gate](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Gate/) : Gates a signal
  * [GrowthDecay](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/GrowthDecay/) : Produces a biological growth-decay curve
  * [Histogram](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Histogram/) : Generates a histogram
  * [InitialValue](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/InitialValue/) : Initial value for difference equations
  * [InputSelector](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/InputSelector/) : Outputs one of n inputs based on an input index
  * [Integrator](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Integrator/) : Sums input over time
  * [LinearSplines](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/LinearSplines/) : Computes linear spline functions
  * [ListIterator](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/ListIterator/) : Iterates through a list of numbers whenever gets a trigger signal
  * [Logger](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Logger/) : Sends a log message at start-up to the ikaros site
  * [Map](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Map/) : Maps from one interval to another
  * [MatrixMultiply](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/MatrixMultiply/) : Multiplies two matrices
  * [MatrixRotation](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/MatrixRotation/) : Rotates a matrix around its center by given angle
  * [MatrixScale](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/MatrixScale/) : Scales content of matrix
  * [MatrixTranslation](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/MatrixTranslation/) : Translates content of a matrix
  * [Max](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Max/) : Maximum of two inputs
  * [Maxima](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Maxima/) : Selects maximum element
  * [Mean](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Mean/) : Caluates mean of all inputs
  * [MidiFilter](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/MidiFilter/) : Filters MIDI signals
  * [MidiInterface](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/MidiInterface/) : MIDI input, output
  * [Min](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Min/) : Minimum of two inputs
  * [Multiply](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Multiply/) : Multiplies two inputs
  * [Noise](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Noise/) : Adds noise
  * [Normalize](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Normalize/) : Normalizes its input
  * [Ones](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Ones/) : A matrix with a single value
  * [OscInterface](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/OscInterface/) : OSC (open sound control) input output
  * [OuterProduct](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/OuterProduct/) : Outer product of two inputs
  * [Polynomial](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Polynomial/) : Calculates polynomial
  * [Product](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Product/) : Multiplies all input
  * [Randomizer](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Randomizer/) : Generates random values
  * [ReactionTimeStatistics](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/ReactionTimeStatistics/) : Collects reaction time statistics
  * [RingBuffer](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/RingBuffer/) : A ring buffer
  * [RotationConverter](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/RotationConverter/) : Converts between rotation notations
  * [Scale](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Scale/) : Scales a signal
  * [SelectMax](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/SelectMax/) : Selects maximum element
  * [SelectRow](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/SelectRow/) : Selectrows maximum element
  * [Select](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Select/) : Selects maximum element
  * [SetSubmatrix](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/SetSubmatrix/) : Set one matrix as a submatrix of another
  * [Sink](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Sink/) : Throws away a signal
  * [Softmax](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Softmax/) : Calculates softmax
  * [Submatrix](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Submatrix/) : Submatrixs matrix elements
  * [Subtract](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Subtract/) : Subtracts two inputs
  * [SumMatrix](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/SumMatrix/) : Sums a matrix to one value
  * [Sum](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Sum/) : Sums inputs
  * [Sweep](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Sweep/) : Produces a sequence of values
  * [Tanh](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Tanh/) : Applies the tanh function to the input
  * [Text](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Text/) : Tests a text parameter
  * [Threshold](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Threshold/) : Applies a threshold
  * [Transform](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/Transform/) : Transforms a set of matrices
  * [TruncateArray](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/UtilityModules/TruncateArray/) : Split an input array and concatenate a output matrix
* VisionModules/
  * [AttentionFocus](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/AttentionFocus/) : Selects an image region
  * [AttentionWindow](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/AttentionWindow/) : Selects an object region
  * [BackgroundSubtraction/StaufferGrimson](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/BackgroundSubtraction/StaufferGrimson/) : Forground/background segmentation
  * [CIFaceDetector](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/CIFaceDetector/) : Wraps apple's cifacedetector
  * [CircleDetector](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/CircleDetector/) : Find circles in an image
  * [ColorClassifier](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/ColorClassifier/) : Tracks colored objects
  * [ColorMatch](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/ColorMatch/) : Match a color
  * [ColorTransform](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/ColorTransform/) : Transforms between color spaces
  * [DFaceDetector](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/DFaceDetector/) : Wraps dlib's dfacedetector
  * [DTracker](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/DTracker/) : Wraps dlib's correlation tracker
  * DepthProcessing/
    * [DepthBlobList](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/DepthProcessing/DepthBlobList/) : Segments into depth planes
    * [DepthContourTrace](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/DepthProcessing/DepthContourTrace/) : Segments into depth planes
    * [DepthHistogram](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/DepthProcessing/DepthHistogram/) : Segments into depth planes
    * [DepthPointList](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/DepthProcessing/DepthPointList/) : Converts an rgb-d image to a h_matrix list
    * [DepthSegmentation](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/DepthProcessing/DepthSegmentation/) : Segments into depth planes
    * [DepthTransform](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/DepthProcessing/DepthTransform/) : Segments into depth planes
  * [ImageConvolution](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/ImageConvolution/) : Filter an image
  * ImageOperators/
    * [BlockMatching](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/ImageOperators/BlockMatching/) : Calculates motion in an image
    * [ChangeDetector](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/ImageOperators/ChangeDetector/) : Detects flicker
    * CurvatureDetectors/
      * [FASTDetector](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/ImageOperators/CurvatureDetectors/FASTDetector/) : Finds curvature points
      * [HarrisDetector](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/ImageOperators/CurvatureDetectors/HarrisDetector/) : Finds curvature points
    * [DirectionDetector](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/ImageOperators/DirectionDetector/) : Calculates motion in an image
    * [DoGFilter](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/ImageOperators/DoGFilter/) : Applies a dog filter
    * EdgeDetectors/
      * [CannyEdgeDetector](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/ImageOperators/EdgeDetectors/CannyEdgeDetector/) : Finds edges
      * [EdgeSegmentation](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/ImageOperators/EdgeDetectors/EdgeSegmentation/) : Finds edges
      * [GaussianEdgeDetector](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/ImageOperators/EdgeDetectors/GaussianEdgeDetector/) : Finds edges and orientations
      * [HysteresisThresholding](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/ImageOperators/EdgeDetectors/HysteresisThresholding/) : Adaptive edge threshold
      * [PrewittEdgeDetector](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/ImageOperators/EdgeDetectors/PrewittEdgeDetector/) : Finds edges
      * [RobertsEdgeDetector](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/ImageOperators/EdgeDetectors/RobertsEdgeDetector/) : Finds edges
      * [SobelEdgeDetector](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/ImageOperators/EdgeDetectors/SobelEdgeDetector/) : Finds edges
    * [GaborFilter](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/ImageOperators/GaborFilter/) : Filters an image
    * [GaussianFilter](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/ImageOperators/GaussianFilter/) : Applies a gaussian filter
    * [LoGFilter](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/ImageOperators/LoGFilter/) : Applies a dog filter
    * [MedianFilter](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/ImageOperators/MedianFilter/) : Applies a median filter
    * MorphologicalOperators/
      * [Dilate](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/ImageOperators/MorphologicalOperators/Dilate/) : Dilates an image
      * [Erode](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/ImageOperators/MorphologicalOperators/Erode/) : Erodes an image
    * [ZeroCrossings](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/ImageOperators/ZeroCrossings/) : Finds zero crossings in an image
  * [IntegralImage](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/IntegralImage/) : Calculate integral image
  * MarkerTracker/
    * Utilities/
      * [CalibrateCameraPosition](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/MarkerTracker/Utilities/CalibrateCameraPosition/) : Outputs the position and rotation of the camera using an artoolkit marker.
  * [MarkerTracker](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/MarkerTracker/) : Finds markers in an image
  * [SaliencePoints](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/SaliencePoints/) : Converts points to a saliency map
  * [SaliencyMap](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/SaliencyMap/) : A saliency map
  * Scaling/
    * [Downsample](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/Scaling/Downsample/) : Halves an image
    * [Upsample](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/Scaling/Upsample/) : Doubles an image
  * [SpatialClustering](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/SpatialClustering/) : Finds clusters of pixels
  * [WhiteBalance](https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/VisionModules/WhiteBalance/) : Removes colorisation from an image
