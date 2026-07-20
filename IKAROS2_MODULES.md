# Ikaros 2 Module Port Status

Generated on 2026-07-20 by comparing `.ikc` module names in the Ikaros 2 `main` branch under `Source/Modules` with local Ikaros 3 `.ikc` module names under `Source/Modules`.

Example and test `.ikc` files were excluded. Matching is by module/class filename, so renamed, merged, or split modules may need manual review.

Total Ikaros 2 modules considered: 218
Total ported by name: 58
Total not yet ported by name: 160

## Ported Modules

| Module | Ikaros 2 path | Ikaros 3 path |
| --- | --- | --- |
| `Abs` | `Source/Modules/UtilityModules/Abs/Abs.ikc` | `Source/Modules/UtilityModules/Functions/Abs/Abs.ikc` |
| `Add` | `Source/Modules/UtilityModules/Add/Add.ikc` | `Source/Modules/UtilityModules/CombineFunctions/Add/Add.ikc` |
| `And` | `Source/Modules/UtilityModules/And/And.ikc` | `Source/Modules/UtilityModules/CombineFunctions/And/And.ikc` |
| `Arbiter` | `Source/Modules/UtilityModules/Arbiter/Arbiter.ikc` | `Source/Modules/UtilityModules/Arbiter/Arbiter.ikc` |
| `ArgMax` | `Source/Modules/UtilityModules/ArgMax/ArgMax.ikc` | `Source/Modules/UtilityModules/ArgMax/ArgMax.ikc` |
| `Average` | `Source/Modules/UtilityModules/Average/Average.ikc` | `Source/Modules/UtilityModules/ReduceFunctions/Average/Average.ikc` |
| `CannyEdgeDetector` | `Source/Modules/VisionModules/ImageOperators/EdgeDetectors/CannyEdgeDetector/CannyEdgeDetector.ikc` | `Source/Modules/VisionModules/CannyEdgeDetector/CannyEdgeDetector.ikc` |
| `ColorMatch` | `Source/Modules/VisionModules/ColorMatch/ColorMatch.ikc` | `Source/Modules/VisionModules/ColorMatch/ColorMatch.ikc` |
| `ColorTransform` | `Source/Modules/VisionModules/ColorTransform/ColorTransform.ikc` | `Source/Modules/VisionModules/ColorTransform/ColorTransform.ikc` |
| `Constant` | `Source/Modules/UtilityModules/Constant/Constant.ikc` | `Source/Modules/UtilityModules/Constant/Constant.ikc` |
| `Delta` | `Source/Modules/LearningModules/Delta/Delta.ikc` | `Source/Modules/BrainModels/Delta/Delta.ikc` |
| `Divide` | `Source/Modules/UtilityModules/Divide/Divide.ikc` | `Source/Modules/UtilityModules/CombineFunctions/Divide/Divide.ikc` |
| `Downsample` | `Source/Modules/VisionModules/Scaling/Downsample/Downsample.ikc` | `Source/Modules/ImageProcessing/Downsample/Downsample.ikc` |
| `EnergyMeter` | `Source/Modules/UtilityModules/EnergyMeter/EnergyMeter.ikc` | `Source/Modules/UtilityModules/EnergyMeter/EnergyMeter.ikc` |
| `EpiServos` | `Source/Modules/RobotModules/EpiServos/EpiServos.ikc` | `Source/Modules/RobotModules/EpiServos/EpiServos.ikc` |
| `EpiSpeech` | `Source/Modules/IOModules/EpiSpeech/EpiSpeech.ikc` | `Source/Modules/EpiSpeech/EpiSpeech.ikc` |
| `EpiVoice` | `Source/Modules/IOModules/EpiVoice/EpiVoice.ikc` | `Source/Modules/EpiVoice/EpiVoice.ikc` |
| `FadeCandy` | `Source/Modules/RobotModules/FadeCandy/FadeCandy.ikc` | `Source/Modules/RobotModules/FadeCandy/FadeCandy.ikc` |
| `FunctionGenerator` | `Source/Modules/UtilityModules/FunctionGenerator/FunctionGenerator.ikc` | `Source/Modules/UtilityModules/FunctionGenerator/FunctionGenerator.ikc` |
| `InputFile` | `Source/Modules/IOModules/FileInput/InputFile/InputFile.ikc` | `Source/Modules/IOModules/FileInput/InputFile/InputFile.ikc` |
| `InputJPEG` | `Source/Modules/IOModules/FileInput/InputJPEG/InputJPEG.ikc` | `Source/Modules/IOModules/FileInput/InputJPEG/InputJPEG.ikc` |
| `InputVideo` | `Source/Modules/IOModules/Video/InputVideo/InputVideo.ikc` | `Source/Modules/IOModules/Video/InputVideo/InputVideo.ikc` |
| `InputVideoFile` | `Source/Modules/IOModules/FileInput/InputVideoFile/InputVideoFile.ikc` | `Source/Modules/IOModules/FileInput/InputVideoFile/InputVideoFile.ikc` |
| `InputVideoStream` | `Source/Modules/IOModules/Video/InputVideoStream/InputVideoStream.ikc` | `Source/Modules/IOModules/Video/InputVideoStream/InputVideoStream.ikc` |
| `Integrator` | `Source/Modules/UtilityModules/Integrator/Integrator.ikc` | `Source/Modules/UtilityModules/Integrator/Integrator.ikc` |
| `KalmanFilter` | `Source/Modules/ControlModules/KalmanFilter/KalmanFilter.ikc` | `Source/Modules/ControlModules/KalmanFilter/KalmanFilter.ikc` |
| `LinearSplines` | `Source/Modules/UtilityModules/LinearSplines/LinearSplines.ikc` | `Source/Modules/UtilityModules/LinearSplines/LinearSplines.ikc` |
| `Logger` | `Source/Modules/UtilityModules/Logger/Logger.ikc` | `Source/Modules/UtilityModules/Logger/Logger.ikc` |
| `MatrixMultiply` | `Source/Modules/UtilityModules/MatrixMultiply/MatrixMultiply.ikc` | `Source/Modules/UtilityModules/MatrixMultiply/MatrixMultiply.ikc` |
| `Max` | `Source/Modules/UtilityModules/Max/Max.ikc` | `Source/Modules/UtilityModules/ReduceFunctions/Max/Max.ikc` |
| `Min` | `Source/Modules/UtilityModules/Min/Min.ikc` | `Source/Modules/UtilityModules/ReduceFunctions/Min/Min.ikc` |
| `Multiply` | `Source/Modules/UtilityModules/Multiply/Multiply.ikc` | `Source/Modules/UtilityModules/CombineFunctions/Multiply/Multiply.ikc` |
| `Noise` | `Source/Modules/UtilityModules/Noise/Noise.ikc` | `Source/Modules/UtilityModules/Noise/Noise.ikc` |
| `Normalize` | `Source/Modules/UtilityModules/Normalize/Normalize.ikc` | `Source/Modules/UtilityModules/Normalize/Normalize.ikc` |
| `Nucleus` | `Source/Modules/BrainModels/Nucleus/Nucleus.ikc` | `Source/Modules/BrainModels/Nucleus/Nucleus.ikc` |
| `OneHotVector` | `Source/Modules/UtilityModules/OneHotVector/OneHotVector.ikc` | `Source/Modules/UtilityModules/OneHotVector/OneHotVector.ikc` |
| `Or` | `Source/Modules/UtilityModules/Or/Or.ikc` | `Source/Modules/UtilityModules/CombineFunctions/Or/Or.ikc` |
| `OutputFile` | `Source/Modules/IOModules/FileOutput/OutputFile/OutputFile.ikc` | `Source/Modules/IOModules/FileOutput/OutputFile/OutputFile.ikc` |
| `PIDController` | `Source/Modules/ControlModules/PIDController/PIDController.ikc` | `Source/Modules/ControlModules/PIDController/PIDController.ikc` |
| `Product` | `Source/Modules/UtilityModules/Product/Product.ikc` | `Source/Modules/UtilityModules/ReduceFunctions/Product/Product.ikc` |
| `Randomizer` | `Source/Modules/UtilityModules/Randomizer/Randomizer.ikc` | `Source/Modules/UtilityModules/Randomizer/Randomizer.ikc` |
| `RotationConverter` | `Source/Modules/UtilityModules/RotationConverter/RotationConverter.ikc` | `Source/Modules/UtilityModules/RotationConverter/RotationConverter.ikc` |
| `Scale` | `Source/Modules/UtilityModules/Scale/Scale.ikc` | `Source/Modules/UtilityModules/Scale/Scale.ikc` |
| `SequenceRecorder` | `Source/Modules/RobotModules/SequenceRecorder/SequenceRecorder.ikc` | `Source/Modules/UtilityModules/SequenceRecorder/SequenceRecorder.ikc` |
| `ServoConnector` | `Source/Modules/RobotModules/ServoConnector/ServoConnector.ikc` | `Source/Modules/RobotModules/ServoConnector/ServoConnector.ikc` |
| `SobelEdgeDetector` | `Source/Modules/VisionModules/ImageOperators/EdgeDetectors/SobelEdgeDetector/SobelEdgeDetector.ikc` | `Source/Modules/VisionModules/SobelEdgeDetector/SobelEdgeDetector.ikc` |
| `Softmax` | `Source/Modules/UtilityModules/Softmax/Softmax.ikc` | `Source/Modules/UtilityModules/Softmax/Softmax.ikc` |
| `SoundOutput` | `Source/Modules/IOModules/SoundOutput/SoundOutput.ikc` | `Source/Modules/IOModules/SoundOutput/SoundOutput.ikc` |
| `SpikingPopulation` | `Source/Modules/BrainModels/SpikingPopulation/SpikingPopulation.ikc` | `Source/Modules/BrainModels/SpikingPopulation/SpikingPopulation.ikc` |
| `StaufferGrimson` | `Source/Modules/VisionModules/BackgroundSubtraction/StaufferGrimson/StaufferGrimson.ikc` | `Source/Modules/ImageProcessing/StaufferGrimson/StaufferGrimson.ikc` |
| `Subtract` | `Source/Modules/UtilityModules/Subtract/Subtract.ikc` | `Source/Modules/UtilityModules/CombineFunctions/Subtract/Subtract.ikc` |
| `Sum` | `Source/Modules/UtilityModules/Sum/Sum.ikc` | `Source/Modules/UtilityModules/ReduceFunctions/Sum/Sum.ikc` |
| `Threshold` | `Source/Modules/UtilityModules/Threshold/Threshold.ikc` | `Source/Modules/UtilityModules/Threshold/Threshold.ikc` |
| `Transform` | `Source/Modules/UtilityModules/Transform/Transform.ikc` | `Source/Modules/UtilityModules/Transform/Transform.ikc` |
| `UM7` | `Source/Modules/RobotModules/UM7/UM7.ikc` | `Source/Modules/RobotModules/UM7/UM7.ikc` |
| `Upsample` | `Source/Modules/VisionModules/Scaling/Upsample/Upsample.ikc` | `Source/Modules/ImageProcessing/Upsample/Upsample.ikc` |
| `WhiteBalance` | `Source/Modules/VisionModules/WhiteBalance/WhiteBalance.ikc` | `Source/Modules/VisionModules/WhiteBalance/WhiteBalance.ikc` |
| `Xor` | `Source/Modules/UtilityModules/Xor/Xor.ikc` | `Source/Modules/UtilityModules/CombineFunctions/Xor/Xor.ikc` |

## Modules Not Yet Ported

| Module | Ikaros 2 path |
| --- | --- |
| `ActionCompetition` | `Source/Modules/RobotModules/ActionCompetition/ActionCompetition.ikc` |
| `ArrayToMatrix` | `Source/Modules/UtilityModules/ArrayToMatrix/ArrayToMatrix.ikc` |
| `AttentionFocus` | `Source/Modules/VisionModules/AttentionFocus/AttentionFocus.ikc` |
| `AttentionWindow` | `Source/Modules/VisionModules/AttentionWindow/AttentionWindow.ikc` |
| `Autoassociator` | `Source/Modules/ANN/Autoassociator/Autoassociator.ikc` |
| `BackProp` | `Source/Modules/ANN/BackProp/BackProp.ikc` |
| `BlackBox` | `Source/Modules/UtilityModules/BlackBox/BlackBox.ikc` |
| `BlockMatching` | `Source/Modules/VisionModules/ImageOperators/BlockMatching/BlockMatching.ikc` |
| `Burner` | `Source/Modules/UtilityModules/Burner/Burner.ikc` |
| `CalibrateCameraPosition` | `Source/Modules/VisionModules/MarkerTracker/Utilities/CalibrateCameraPosition/CalibrateCameraPosition.ikc` |
| `ChangeDetector` | `Source/Modules/VisionModules/ImageOperators/ChangeDetector/ChangeDetector.ikc` |
| `CIFaceDetector` | `Source/Modules/VisionModules/CIFaceDetector/CIFaceDetector.ikc` |
| `CircleDetector` | `Source/Modules/VisionModules/CircleDetector/CircleDetector.ikc` |
| `CoarseCoder` | `Source/Modules/CodingModules/CoarseCoder/CoarseCoder.ikc` |
| `ColorClassifier` | `Source/Modules/VisionModules/ColorClassifier/ColorClassifier.ikc` |
| `Concat` | `Source/Modules/UtilityModules/Concat/Concat.ikc` |
| `Counter` | `Source/Modules/UtilityModules/Counter/Counter.ikc` |
| `CSOM` | `Source/Modules/ANN/CSOM/CSOM/CSOM.ikc` |
| `CSOM_L` | `Source/Modules/ANN/CSOM/CSOM_L/CSOM_L.ikc` |
| `CSOM_PCA` | `Source/Modules/ANN/CSOM/CSOM_PCA/CSOM_PCA.ikc` |
| `DataBuffer` | `Source/Modules/UtilityModules/DataBuffer/DataBuffer.ikc` |
| `DataConverter` | `Source/Modules/UtilityModules/DataConverter/DataConverter.ikc` |
| `DecomposeTransform` | `Source/Modules/UtilityModules/DecomposeTransform/DecomposeTransform.ikc` |
| `Delay` | `Source/Modules/UtilityModules/Delay/Delay.ikc` |
| `DepthBlobList` | `Source/Modules/VisionModules/DepthProcessing/DepthBlobList/DepthBlobList.ikc` |
| `DepthContourTrace` | `Source/Modules/VisionModules/DepthProcessing/DepthContourTrace/DepthContourTrace.ikc` |
| `DepthHistogram` | `Source/Modules/VisionModules/DepthProcessing/DepthHistogram/DepthHistogram.ikc` |
| `DepthPointList` | `Source/Modules/VisionModules/DepthProcessing/DepthPointList/DepthPointList.ikc` |
| `DepthSegmentation` | `Source/Modules/VisionModules/DepthProcessing/DepthSegmentation/DepthSegmentation.ikc` |
| `DepthTransform` | `Source/Modules/VisionModules/DepthProcessing/DepthTransform/DepthTransform.ikc` |
| `DFaceDetector` | `Source/Modules/VisionModules/DFaceDetector/DFaceDetector.ikc` |
| `Dilate` | `Source/Modules/VisionModules/ImageOperators/MorphologicalOperators/Dilate/Dilate.ikc` |
| `DirectionDetector` | `Source/Modules/VisionModules/ImageOperators/DirectionDetector/DirectionDetector.ikc` |
| `Distance` | `Source/Modules/UtilityModules/Distance/Distance.ikc` |
| `DoGFilter` | `Source/Modules/VisionModules/ImageOperators/DoGFilter/DoGFilter.ikc` |
| `DTracker` | `Source/Modules/VisionModules/DTracker/DTracker.ikc` |
| `Dynamixel` | `Source/Modules/RobotModules/Dynamixel/Dynamixel.ikc` |
| `DynamixelConfigure` | `Source/Modules/RobotModules/Dynamixel/DynamixelConfigure.ikc` |
| `EdgeSegmentation` | `Source/Modules/VisionModules/ImageOperators/EdgeDetectors/EdgeSegmentation/EdgeSegmentation.ikc` |
| `Epuck` | `Source/Modules/RobotModules/Epuck/Epuck.ikc` |
| `Erode` | `Source/Modules/VisionModules/ImageOperators/MorphologicalOperators/Erode/Erode.ikc` |
| `EventTrigger` | `Source/Modules/UtilityModules/EventTrigger/EventTrigger.ikc` |
| `Expression` | `Source/Modules/UtilityModules/Expression/Expression.ikc` |
| `EyelidAnimation` | `Source/Modules/RobotModules/EyelidAnimation/EyelidAnimation.ikc` |
| `EyeModel` | `Source/Modules/BrainModels/EyeModel/EyeModel.ikc` |
| `FanIn` | `Source/Modules/UtilityModules/FanIn/FanIn.ikc` |
| `FanOut` | `Source/Modules/UtilityModules/FanOut/FanOut.ikc` |
| `FASTDetector` | `Source/Modules/VisionModules/ImageOperators/CurvatureDetectors/FASTDetector/FASTDetector.ikc` |
| `FFT` | `Source/Modules/UtilityModules/FFT/FFT.ikc` |
| `Flip` | `Source/Modules/UtilityModules/Flip/Flip.ikc` |
| `FlipFlop` | `Source/Modules/UtilityModules/FlipFlop/FlipFlop.ikc` |
| `Fuse` | `Source/Modules/UtilityModules/Fuse/Fuse.ikc` |
| `GaborFilter` | `Source/Modules/VisionModules/ImageOperators/GaborFilter/GaborFilter.ikc` |
| `Gate` | `Source/Modules/UtilityModules/Gate/Gate.ikc` |
| `GaussianEdgeDetector` | `Source/Modules/VisionModules/ImageOperators/EdgeDetectors/GaussianEdgeDetector/GaussianEdgeDetector.ikc` |
| `GaussianFilter` | `Source/Modules/VisionModules/ImageOperators/GaussianFilter/GaussianFilter.ikc` |
| `GazeController` | `Source/Modules/RobotModules/GazeController/GazeController.ikc` |
| `GLRobotSimulator` | `Source/Modules/EnvironmentModules/GLRobotSimulator/GLRobotSimulator.ikc` |
| `GridWorld` | `Source/Modules/EnvironmentModules/GridWorld/GridWorld.ikc` |
| `GrowthDecay` | `Source/Modules/UtilityModules/GrowthDecay/GrowthDecay.ikc` |
| `HarrisDetector` | `Source/Modules/VisionModules/ImageOperators/CurvatureDetectors/HarrisDetector/HarrisDetector.ikc` |
| `Histogram` | `Source/Modules/UtilityModules/Histogram/Histogram.ikc` |
| `HysteresisThresholding` | `Source/Modules/VisionModules/ImageOperators/EdgeDetectors/HysteresisThresholding/HysteresisThresholding.ikc` |
| `ImageConvolution` | `Source/Modules/VisionModules/ImageConvolution/ImageConvolution.ikc` |
| `InitialValue` | `Source/Modules/UtilityModules/InitialValue/InitialValue.ikc` |
| `InputPNG` | `Source/Modules/IOModules/FileInput/InputPNG/InputPNG.ikc` |
| `InputRawImage` | `Source/Modules/IOModules/FileInput/InputRawImage/InputRawImage.ikc` |
| `InputSelector` | `Source/Modules/UtilityModules/InputSelector/InputSelector.ikc` |
| `InputVideoAV` | `Source/Modules/IOModules/Video/InputVideoAV/InputVideoAV.ikc` |
| `IntegralImage` | `Source/Modules/VisionModules/IntegralImage/IntegralImage.ikc` |
| `IntervalCoder` | `Source/Modules/CodingModules/IntervalCoder/IntervalCoder.ikc` |
| `IntervalDecoder` | `Source/Modules/CodingModules/IntervalDecoder/IntervalDecoder.ikc` |
| `IPCClient` | `Source/Modules/IOModules/Network/IPCClient/IPCClient.ikc` |
| `IPCServer` | `Source/Modules/IOModules/Network/IPCServer/IPCServer.ikc` |
| `Kinect` | `Source/Modules/IOModules/Video/Kinect/Kinect.ikc` |
| `KNN` | `Source/Modules/LearningModules/KNN/KNN.ikc` |
| `KNN_Pick` | `Source/Modules/LearningModules/KNN_Pick/KNN_Pick.ikc` |
| `LearningModule` | `Source/Modules/LearningModules/LearningModule/LearningModule.ikc` |
| `LinearAssociator` | `Source/Modules/LearningModules/LinearAssociator/LinearAssociator.ikc` |
| `ListIterator` | `Source/Modules/UtilityModules/ListIterator/ListIterator.ikc` |
| `LM22_ContinuousWorld` | `Source/Modules/BrainModels/LearningModel2022/LM22_ContinuousWorld/LM22_ContinuousWorld.ikc` |
| `LM22_Perception` | `Source/Modules/BrainModels/LearningModel2022/LM22_Perception/LM22_Perception.ikc` |
| `LM22_ValueAccumulator` | `Source/Modules/BrainModels/LearningModel2022/LM22_ValueAccumulator/LM22_ValueAccumulator.ikc` |
| `LocalApproximator` | `Source/Modules/LearningModules/LocalApproximator/LocalApproximator.ikc` |
| `LoGFilter` | `Source/Modules/VisionModules/ImageOperators/LoGFilter/LoGFilter.ikc` |
| `M02_Amygdala` | `Source/Modules/BrainModels/Moren2002/M02_Amygdala/M02_Amygdala.ikc` |
| `M02_Cortex` | `Source/Modules/BrainModels/Moren2002/M02_Cortex/M02_Cortex.ikc` |
| `M02_Hippocampus` | `Source/Modules/BrainModels/Moren2002/M02_Hippocampus/M02_Hippocampus.ikc` |
| `M02_OFC` | `Source/Modules/BrainModels/Moren2002/M02_OFC/M02_OFC.ikc` |
| `M02_SensoryCortex` | `Source/Modules/BrainModels/Moren2002/M02_SensoryCortex/M02_SensoryCortex.ikc` |
| `M02_Thalamus` | `Source/Modules/BrainModels/Moren2002/M02_Thalamus/M02_Thalamus.ikc` |
| `Map` | `Source/Modules/UtilityModules/Map/Map.ikc` |
| `MarkerTracker` | `Source/Modules/VisionModules/MarkerTracker/MarkerTracker.ikc` |
| `MatrixRotation` | `Source/Modules/UtilityModules/MatrixRotation/MatrixRotation.ikc` |
| `MatrixScale` | `Source/Modules/UtilityModules/MatrixScale/MatrixScale.ikc` |
| `MatrixTranslation` | `Source/Modules/UtilityModules/MatrixTranslation/MatrixTranslation.ikc` |
| `Maxima` | `Source/Modules/UtilityModules/Maxima/Maxima.ikc` |
| `MazeGenerator` | `Source/Modules/EnvironmentModules/MazeGenerator/MazeGenerator.ikc` |
| `Mean` | `Source/Modules/UtilityModules/Mean/Mean.ikc` |
| `MeanShift` | `Source/Modules/VisionModules/OpticFlow/MeanShift/MeanShift.ikc` |
| `MedianFilter` | `Source/Modules/VisionModules/ImageOperators/MedianFilter/MedianFilter.ikc` |
| `MemorySequencer` | `Source/Modules/BrainModels/DecisionModel2020/MemorySequencer/MemorySequencer.ikc` |
| `MidiFilter` | `Source/Modules/UtilityModules/MidiFilter/MidiFilter.ikc` |
| `MidiInterface` | `Source/Modules/UtilityModules/MidiInterface/MidiInterface.ikc` |
| `MotionGuard` | `Source/Modules/RobotModules/MotionGuard/MotionGuard.ikc` |
| `MotionRecorder` | `Source/Modules/RobotModules/MotionRecorder/MotionRecorder.ikc` |
| `MouthAnimation` | `Source/Modules/RobotModules/MouthAnimation/MouthAnimation.ikc` |
| `NetworkCamera` | `Source/Modules/IOModules/Video/NetworkCamera/NetworkCamera.ikc` |
| `NeuralArray` | `Source/Modules/BrainModels/NeuralArray/NeuralArray.ikc` |
| `NeuralField` | `Source/Modules/BrainModels/NeuralField/NeuralField.ikc` |
| `NucleusEnsemble` | `Source/Modules/BrainModels/NucleusEnsemble/NucleusEnsemble.ikc` |
| `Ones` | `Source/Modules/UtilityModules/Ones/Ones.ikc` |
| `OscInterface` | `Source/Modules/UtilityModules/OscInterface/OscInterface.ikc` |
| `OuterProduct` | `Source/Modules/UtilityModules/OuterProduct/OuterProduct.ikc` |
| `OutputJPEG` | `Source/Modules/IOModules/FileOutput/OutputJPEG/OutputJPEG.ikc` |
| `OutputPNG` | `Source/Modules/IOModules/FileOutput/OutputPNG/OutputPNG.ikc` |
| `OutputRawImage` | `Source/Modules/IOModules/FileOutput/OutputRawImage/OutputRawImage.ikc` |
| `OutputVideoFile` | `Source/Modules/IOModules/FileOutput/OutputVideoFile/OutputVideoFile.ikc` |
| `Perception` | `Source/Modules/BrainModels/DecisionModel2020/Perception/Perception.ikc` |
| `Perceptron` | `Source/Modules/ANN/Perceptron/Perceptron.ikc` |
| `Phidgets` | `Source/Modules/RobotModules/Phidgets/Phidgets.ikc` |
| `PlanarArm` | `Source/Modules/EnvironmentModules/PlanarArm/PlanarArm.ikc` |
| `Polynomial` | `Source/Modules/UtilityModules/Polynomial/Polynomial.ikc` |
| `PopulationCoder` | `Source/Modules/CodingModules/PopulationCoder/PopulationCoder.ikc` |
| `PopulationDecoder` | `Source/Modules/CodingModules/PopulationDecoder/PopulationDecoder.ikc` |
| `PrewittEdgeDetector` | `Source/Modules/VisionModules/ImageOperators/EdgeDetectors/PrewittEdgeDetector/PrewittEdgeDetector.ikc` |
| `QLearning` | `Source/Modules/LearningModules/QLearning/QLearning.ikc` |
| `ReactionTimeStatistics` | `Source/Modules/UtilityModules/ReactionTimeStatistics/ReactionTimeStatistics.ikc` |
| `RingBuffer` | `Source/Modules/UtilityModules/RingBuffer/RingBuffer.ikc` |
| `RNN` | `Source/Modules/ANN/RNN/RNN.ikc` |
| `RobertsEdgeDetector` | `Source/Modules/VisionModules/ImageOperators/EdgeDetectors/RobertsEdgeDetector/RobertsEdgeDetector.ikc` |
| `SaliencePoints` | `Source/Modules/VisionModules/SaliencePoints/SaliencePoints.ikc` |
| `SaliencyMap` | `Source/Modules/VisionModules/SaliencyMap/SaliencyMap.ikc` |
| `Select` | `Source/Modules/UtilityModules/Select/Select.ikc` |
| `SelectMax` | `Source/Modules/UtilityModules/SelectMax/SelectMax.ikc` |
| `SelectRow` | `Source/Modules/UtilityModules/SelectRow/SelectRow.ikc` |
| `SetSubmatrix` | `Source/Modules/UtilityModules/SetSubmatrix/SetSubmatrix.ikc` |
| `Sink` | `Source/Modules/UtilityModules/Sink/Sink.ikc` |
| `SparseFlow` | `Source/Modules/VisionModules/OpticFlow/SparseFlow/SparseFlow.ikc` |
| `SpatialClustering` | `Source/Modules/VisionModules/SpatialClustering/SpatialClustering.ikc` |
| `SpectralTiming` | `Source/Modules/CodingModules/SpectralTiming/SpectralTiming.ikc` |
| `SSC32` | `Source/Modules/RobotModules/SSC32/SSC32.ikc` |
| `Stop` | `Source/Modules/UtilityModules/Stop/Stop.ikc` |
| `Submatrix` | `Source/Modules/UtilityModules/Submatrix/Submatrix.ikc` |
| `SumMatrix` | `Source/Modules/UtilityModules/SumMatrix/SumMatrix.ikc` |
| `Sweep` | `Source/Modules/UtilityModules/Sweep/Sweep.ikc` |
| `Tanh` | `Source/Modules/UtilityModules/Tanh/Tanh.ikc` |
| `TappedDelayLine` | `Source/Modules/CodingModules/TappedDelayLine/TappedDelayLine.ikc` |
| `Test` | `Source/Modules/UtilityModules/Test/Test.ikc` |
| `Text` | `Source/Modules/UtilityModules/Text/Text.ikc` |
| `TheEyeTribe` | `Source/Modules/IOModules/EyeTracker/TheEyeTribe/TheEyeTribe.ikc` |
| `Threshold_b` | `Source/Modules/UtilityModules/Threshold/Threshold_b.ikc` |
| `TimedStop` | `Source/Modules/UtilityModules/TimedStop/TimedStop.ikc` |
| `Topology` | `Source/Modules/UtilityModules/Topology/Topology.ikc` |
| `TouchBoard` | `Source/Modules/IOModules/TouchBoard/TouchBoard.ikc` |
| `Trainer` | `Source/Modules/LearningModules/Trainer/Trainer.ikc` |
| `TruncateArray` | `Source/Modules/UtilityModules/TruncateArray/TruncateArray.ikc` |
| `ValueAccumulator` | `Source/Modules/BrainModels/DecisionModel2020/ValueAccumulator/ValueAccumulator.ikc` |
| `YARPPort` | `Source/Modules/IOModules/Network/YARPPort/YARPPort.ikc` |
| `ZeroCrossings` | `Source/Modules/VisionModules/ImageOperators/ZeroCrossings/ZeroCrossings.ikc` |
