//
//	Modules.h		This file is a part of the IKAROS project
//                  This file includes the h-files of all standard modules
//
//    Copyright (C) 2006-2010	Christian Balkenius
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

//
// This section includes all modules that are to be compiled with IKAROS
// When new standard modules are added to the system, they must be included here
//
// User modules should not be included here but in UserModules.h
//

// IO Modules


#include "Modules/IOModules/FileInput/InputFile/InputFile.h"
#include "Modules/IOModules/FileOutput/OutputFile/OutputFile.h"
#include "Modules/IOModules/FileInput/InputRawImage/InputRawImage.h"
#include "Modules/IOModules/FileInput/InputJPEG/InputJPEG.h"
#include "Modules/IOModules/FileInput/InputPNG/InputPNG.h"
#include "Modules/IOModules/FileInput/InputQTMovie/InputQTMovie.h"
#include "Modules/IOModules/FileInput/InputQTAudio/InputQTAudio.h"

#include "Modules/IOModules/FileOutput/OutputRawImage/OutputRawImage.h"
#include "Modules/IOModules/FileOutput/OutputJPEG/OutputJPEG.h"
#include "Modules/IOModules/FileOutput/OutputPNG/OutputPNG.h"
#include "Modules/IOModules/FileOutput/OutputQTAudioVisual/OutputQTAudioVisual.h"

#include "Modules/IOModules/Video/InputVideoQT/InputVideoQT.h"
#include "Modules/IOModules/Video/NetworkCamera/NetworkCamera.h"

// UtilityModules

#include "Modules/UtilityModules/Add/Add.h"
#include "Modules/UtilityModules/Arbiter/Arbiter.h"
#include "Modules/UtilityModules/Average/Average.h"
#include "Modules/UtilityModules/Constant/Constant.h"
#include "Modules/UtilityModules/Counter/Counter.h"
#include "Modules/UtilityModules/Delay/Delay.h"
#include "Modules/UtilityModules/Distance/Distance.h"
#include "Modules/UtilityModules/Divide/Divide.h"
#include "Modules/UtilityModules/FunctionGenerator/FunctionGenerator.h"
#include "Modules/UtilityModules/Gate/Gate.h"
#include "Modules/UtilityModules/Integrator/Integrator.h"
#include "Modules/UtilityModules/MatrixMultiply/MatrixMultiply.h"
#include "Modules/UtilityModules/Max/Max.h"
#include "Modules/UtilityModules/Min/Min.h"
#include "Modules/UtilityModules/ArgMax/ArgMax.h"
#include "Modules/UtilityModules/Multiply/Multiply.h"
#include "Modules/UtilityModules/Normalize/Normalize.h"
#include "Modules/UtilityModules/Noise/Noise.h"
#include "Modules/UtilityModules/OuterProduct/OuterProduct.h"
#include "Modules/UtilityModules/Polynomial/Polynomial.h"
#include "Modules/UtilityModules/Randomizer/Randomizer.h"
#include "Modules/UtilityModules/Abs/Abs.h"
#include "Modules/UtilityModules/Scale/Scale.h"
#include "Modules/UtilityModules/SelectMax/SelectMax.h"
#include "Modules/UtilityModules/Shift/Shift.h"
#include "Modules/UtilityModules/Softmax/Softmax.h"
#include "Modules/UtilityModules/Subtract/Subtract.h"
#include "Modules/UtilityModules/Sweep/Sweep.h"
#include "Modules/UtilityModules/Threshold/Threshold.h"

// CodingModules

#include "Modules/CodingModules/IntervalCoder/IntervalCoder.h"
#include "Modules/CodingModules/IntervalDecoder/IntervalDecoder.h"
#include "Modules/CodingModules/CoarseCoder/CoarseCoder.h"

// VisionModules

#include "Modules/VisionModules/ImageConvolution/ImageConvolution.h"
#include "Modules/VisionModules/ColorTransform/ColorTransform.h"
#include "Modules/VisionModules/ColorMatch/ColorMatch.h"
#include "Modules/VisionModules/WhiteBalance/WhiteBalance.h"
#include "Modules/VisionModules/SaliencyMap/SaliencyMap.h"
#include "Modules/VisionModules/AttentionFocus/AttentionFocus.h"

#include "Modules/VisionModules/ImageOperators/GaborFilter/GaborFilter.h"

#include "Modules/VisionModules/ImageOperators/EdgeDetectors/RobertsEdgeDetector/RobertsEdgeDetector.h"
#include "Modules/VisionModules/ImageOperators/EdgeDetectors/PrewittEdgeDetector/PrewittEdgeDetector.h"
#include "Modules/VisionModules/ImageOperators/EdgeDetectors/SobelEdgeDetector/SobelEdgeDetector.h"
#include "Modules/VisionModules/ImageOperators/EdgeDetectors/GaussianEdgeDetector/GaussianEdgeDetector.h"
#include "Modules/VisionModules/ImageOperators/EdgeDetectors/CannyEdgeDetector/CannyEdgeDetector.h"
#include "Modules/VisionModules/ImageOperators/EdgeDetectors/HysteresisThresholding/HysteresisThresholding.h"

#include "Modules/VisionModules/ImageOperators/CurvatureDetectors/HarrisDetector/HarrisDetector.h"

#include "Modules/VisionModules/ImageOperators/ChangeDetector/ChangeDetector.h"

#include "Modules/VisionModules/Scaling/Upsample/Upsample.h"
#include "Modules/VisionModules/Scaling/Downsample/Downsample.h"

#include "Modules/VisionModules/ImageOperators/MorphologicalOperators/Erode/Erode.h"
#include "Modules/VisionModules/ImageOperators/MorphologicalOperators/Dilate/Dilate.h"

#include "Modules/VisionModules/ColorClassifier/ColorClassifier.h"
#include "Modules/VisionModules/SpatialClustering/SpatialClustering.h"


// Examples

#include "Modules/Examples/DelayOne/DelayOne.h"
#include "Modules/Examples/Sum/Sum.h"

// Environment Modules

#include "Modules/EnvironmentModules/GridWorld/GridWorld.h"
#include "Modules/EnvironmentModules/PlanarArm/PlanarArm.h"
#include "Modules/EnvironmentModules/MazeGenerator/MazeGenerator.h"

// Learning Modules

#include "Modules/LearningModules/LinearAssociator/LinearAssociator.h"
#include "Modules/LearningModules/QLearning/QLearning.h"
#include "Modules/LearningModules/KNN/KNN.h"
#include "Modules/LearningModules/KNN_Pick/KNN_Pick.h"

// Learning Modules

#include "Modules/ControlModules/PIDController/PIDController.h"
#include "Modules/ControlModules/KalmanFilter/KalmanFilter.h"

// ANN Modules

#include "Modules/ANN/Perceptron/Perceptron.h"

// Robot Modules

#include "Modules/RobotModules/Epuck/Epuck.h"
#include "Modules/RobotModules/SSC32/SSC32.h"
#include "Modules/RobotModules/Dynamixel/Dynamixel.h"

// Brain Models

#include "BrainModels/Moren2002/M02_Amygdala/M02_Amygdala.h"
#include "BrainModels/Moren2002/M02_Cortex/M02_Cortex.h"
#include "BrainModels/Moren2002/M02_Hippocampus/M02_Hippocampus.h"
#include "BrainModels/Moren2002/M02_OFC/M02_OFC.h"
#include "BrainModels/Moren2002/M02_SensoryCortex/M02_SensoryCortex.h"
#include "BrainModels/Moren2002/M02_Thalamus/M02_Thalamus.h"

void InitModules(Kernel & k);

void
InitModules(Kernel & k)
{
    // Install module classes in the kernel
    // All modules that are to be used must be installed here or in the file UserModules.h

    // IOModules

    k.AddClass("InputFile", &InputFile::Create, "Source/Modules/IOModules/FileInput/InputFile/");
    k.AddClass("OutputFile", &OutputFile::Create, "Source/Modules/IOModules/FileOutput/OutputFile/");
    k.AddClass("InputRawImage", &InputRawImage::Create, "Source/Modules/IOModules/FileInput/InputRawImage/");
    k.AddClass("InputJPEG", &InputJPEG::Create, "Source/Modules/IOModules/FileInput/InputJPEG/");
    k.AddClass("InputPNG", &InputPNG::Create, "Source/Modules/IOModules/FileInput/InputPNG/");
    k.AddClass("InputQTMovie", &InputQTMovie::Create, "Source/Modules/IOModules/FileInput/InputQTMovie/");
    k.AddClass("InputQTAudio", &InputQTAudio::Create, "Source/Modules/IOModules/FileInput/InputQTAudio/");
    k.AddClass("OutputRawImage", &OutputRawImage::Create, "Source/Modules/IOModules/FileOutput/OutputRawImage/");
    k.AddClass("OutputJPEG", &OutputJPEG::Create, "Source/Modules/IOModules/FileOutput/OutputJPEG/");
    k.AddClass("OutputPNG", &OutputPNG::Create, "Source/Modules/IOModules/FileOutput/OutputPNG/");
    k.AddClass("OutputQTAudioVisual", &OutputQTAudioVisual::Create, "Source/Modules/IOModules/FileOutput/OutputQTAudioVisual/");
    k.AddClass("InputVideoQT", &InputVideoQT::Create, "Source/Modules/IOModules/Video/InputVideoQT/");
    k.AddClass("NetworkCamera", &NetworkCamera::Create, "Source/Modules/IOModules/Video/NetworkCamera/");

    // Utlity Modules

    k.AddClass("Arbiter", &Arbiter::Create, "Source/Modules/UtilityModules/Arbiter/");
    k.AddClass("Constant", &Constant::Create, "Source/Modules/UtilityModules/Constant/");
    k.AddClass("Sweep", &Sweep::Create, "Source/Modules/UtilityModules/Sweep/");
    k.AddClass("Randomizer", &Randomizer::Create, "Source/Modules/UtilityModules/Randomizer/");

    k.AddClass("Abs", &Abs::Create, "Source/Modules/UtilityModules/Abs/");
    k.AddClass("Add", &Add::Create, "Source/Modules/UtilityModules/Add/");
    k.AddClass("Subtract", &Subtract::Create, "Source/Modules/UtilityModules/Subtract/");
    k.AddClass("Multiply", &Multiply::Create, "Source/Modules/UtilityModules/Multiply/");
    k.AddClass("Divide", &Divide::Create, "Source/Modules/UtilityModules/Divide/");
    k.AddClass("Scale", &Scale::Create, "Source/Modules/UtilityModules/Scale/");
    k.AddClass("Normalize", &Normalize::Create, "Source/Modules/UtilityModules/Normalize/");
    k.AddClass("Noise", &Noise::Create, "Source/Modules/UtilityModules/Noise/");

    k.AddClass("OuterProduct", &OuterProduct::Create, "Source/Modules/UtilityModules/OuterProduct/");
    k.AddClass("MatrixMultiply", &MatrixMultiply::Create, "Source/Modules/UtilityModules/MatrixMultiply/");

    k.AddClass("Max", &Max::Create, "Source/Modules/UtilityModules/Max/");
    k.AddClass("Min", &Min::Create, "Source/Modules/UtilityModules/Min/");
    k.AddClass("ArgMax", &ArgMax::Create, "Source/Modules/UtilityModules/ArgMax/");

    k.AddClass("Delay", &Delay::Create, "Source/Modules/UtilityModules/Delay/");
    k.AddClass("Threshold", &Threshold::Create, "Source/Modules/UtilityModules/Threshold/");
    k.AddClass("Gate", &Gate::Create, "Source/Modules/UtilityModules/Gate/");
    k.AddClass("Integrator", &Integrator::Create, "Source/Modules/UtilityModules/Integrator/");
    k.AddClass("Average", &Average::Create, "Source/Modules/UtilityModules/Average/");
    k.AddClass("Softmax", &Softmax::Create, "Source/Modules/UtilityModules/Softmax/");
    k.AddClass("SelectMax", &SelectMax::Create, "Source/Modules/UtilityModules/SelectMax/");
    k.AddClass("Shift", &Shift::Create, "Source/Modules/UtilityModules/Shift/");
    k.AddClass("FunctionGenerator", &FunctionGenerator::Create, "Source/Modules/UtilityModules/FunctionGenerator/");
    k.AddClass("Polynomial", &Polynomial::Create, "Source/Modules/UtilityModules/Polynomial/");
    k.AddClass("Distance", &Distance::Create, "Source/Modules/UtilityModules/Distance/");

    k.AddClass("Counter", &Counter::Create, "Source/Modules/UtilityModules/Counter/");

    // Coding Modules

    k.AddClass("IntervalCoder", &IntervalCoder::Create, "Source/Modules/CodingModules/IntervalCoder/");
    k.AddClass("IntervalDecoder", &IntervalDecoder::Create, "Source/Modules/CodingModules/IntervalDecoder/");
    k.AddClass("CoarseCoder", &CoarseCoder::Create, "Source/Modules/CodingModules/CoarseCoder/");


    // Vision Modules

    k.AddClass("ImageConvolution", &ImageConvolution::Create, "Source/Modules/VisionModules/ImageConvolution/");
    k.AddClass("ColorTransform", &ColorTransform::Create, "Source/Modules/VisionModules/ColorTransform/");
    k.AddClass("ColorMatch", &ColorMatch::Create, "Source/Modules/VisionModules/ColorMatch/");
    k.AddClass("WhiteBalance", &WhiteBalance::Create, "Source/Modules/VisionModules/WhiteBalance/");
    k.AddClass("SaliencyMap", &SaliencyMap::Create, "Source/Modules/VisionModules/SaliencyMap/");
    k.AddClass("AttentionFocus", &AttentionFocus::Create, "Source/Modules/VisionModules/AttentionFocus/");

    k.AddClass("GaborFilter", &GaborFilter::Create, "Source/Modules/VisionModules/ImageOperators/GaborFilter/");

    k.AddClass("RobertsEdgeDetector", &RobertsEdgeDetector::Create, "Source/Modules/VisionModules/ImageOperators/EdgeDetectors/RobertsEdgeDetector/");
    k.AddClass("PrewittEdgeDetector", &PrewittEdgeDetector::Create, "Source/Modules/VisionModules/ImageOperators/EdgeDetectors/PrewittEdgeDetector/");
    k.AddClass("SobelEdgeDetector", &SobelEdgeDetector::Create, "Source/Modules/VisionModules/ImageOperators/EdgeDetectors/SobelEdgeDetector/");
    k.AddClass("GaussianEdgeDetector", &GaussianEdgeDetector::Create, "Source/Modules/VisionModules/ImageOperators/EdgeDetectors/GaussianEdgeDetector/");
    k.AddClass("CannyEdgeDetector", &CannyEdgeDetector::Create, "Source/Modules/VisionModules/ImageOperators/EdgeDetectors/CannyEdgeDetector/");

    k.AddClass("HysteresisThresholding", &HysteresisThresholding::Create, "Source/Modules/VisionModules/ImageOperators/EdgeDetectors/HysteresisThresholding/");

    k.AddClass("HarrisDetector", &HarrisDetector::Create, "Source/Modules/VisionModules/ImageOperators/CurvatureDetectors/HarrisDetector/");

    k.AddClass("ChangeDetector", &ChangeDetector::Create, "Source/Modules/VisionModules/ImageOperators/ChangeDetector/");

    k.AddClass("Erode", &Erode::Create, "Source/Modules/VisionModules/ImageOperators/MorphologicalOperators/Erode/");
    k.AddClass("Dilate", &Dilate::Create, "Source/Modules/VisionModules/ImageOperators/MorphologicalOperators/Dilate/");

    k.AddClass("Upsample", &Upsample::Create, "Source/Modules/VisionModules/Scaling/Upsample/");
    k.AddClass("Downsample", &Downsample::Create , "Source/Modules/VisionModules/Scaling/Downsample/");

    k.AddClass("ColorClassifier", &ColorClassifier::Create , "Source/Modules/VisionModules/ColorClassifier/");
    k.AddClass("SpatialClustering", &SpatialClustering::Create , "Source/Modules/VisionModules/SpatialClustering/");

    // Examples

    k.AddClass("DelayOne", &DelayOne::Create , "Source/Modules/Examples/DelayOne/");
    k.AddClass("Sum", &Sum::Create, "Source/Modules/Examples/Sum/");

    // Environment Modules

    k.AddClass("GridWorld", &GridWorld::Create, "Source/Modules/EnvironmentModules/GridWorld/");
    k.AddClass("PlanarArm", &PlanarArm::Create, "Source/Modules/EnvironmentModules/PlanarArm/");
    k.AddClass("MazeGenerator", &MazeGenerator::Create, "Source/Modules/EnvironmentModules/MazeGenerator/");
	
	// Learning Modules
	
    k.AddClass("LinearAssociator", &LinearAssociator::Create, "Source/Modules/LearningModules/LinearAssociator/");
    k.AddClass("QLearning", &QLearning::Create, "Source/Modules/LearningModules/QLearning/");

    // ANN Modules
    
    k.AddClass("Perceptron", &Perceptron::Create, "Source/Modules/ANN/Perceptron/");
    
	// Control Modules
	
    k.AddClass("PIDController", &PIDController::Create, "Source/Modules/ControlModules/PIDController/");
    k.AddClass("KalmanFilter", &KalmanFilter::Create, "Source/Modules/ControlModules/KalmanFilter/");

	// Robot Modules
	
    k.AddClass("Epuck", &Epuck::Create, "Source/Modules/RobotModules/Epuck/");
    k.AddClass("SSC32", &SSC32::Create, "Source/Modules/RobotModules/SSC32/");
    k.AddClass("Dynamixel", &Dynamixel::Create, "Source/Modules/RobotModules/Dynamixel/");
    
    // Brain Models
    
    k.AddClass("M02_Amygdala", &M02_Amygdala::Create, "Source/Modules/BrainModels/Moren2002/M02_Amygdala/");
    k.AddClass("M02_Cortex", &M02_Cortex::Create, "Source/Modules/BrainModels/Moren2002/M02_Cortex/");
    k.AddClass("M02_Hippocampus", &M02_Hippocampus::Create, "Source/Modules/BrainModels/Moren2002/M02_Hippocampus/");
    k.AddClass("M02_OFC", &M02_OFC::Create, "Source/Modules/BrainModels/Moren2002/M02_OFC/");
    k.AddClass("M02_SensoryCortex", &M02_SensoryCortex::Create, "Source/Modules/BrainModels/Moren2002/M02_SensoryCortex/");
    k.AddClass("M02_Thalamus", &M02_Thalamus::Create, "Source/Modules/BrainModels/Moren2002/M02_Thalamus/");
}
