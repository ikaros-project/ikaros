set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SOURCE
	${SOURCE}
	${CMAKE_CURRENT_SOURCE_DIR}/MPIFaceDetector.cc
	${CMAKE_CURRENT_SOURCE_DIR}/MPEyeFinder/beyesobject.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/MPEyeFinder/eyefinder.cc
	${CMAKE_CURRENT_SOURCE_DIR}/MPEyeFinder/eyefinderBinary.cc
	${CMAKE_CURRENT_SOURCE_DIR}/MPEyeFinder/eyeobject.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/MPEyeFinder/faceobject.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/MPEyeFinder/visualobject.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/MPISearch/ci.cc
	${CMAKE_CURRENT_SOURCE_DIR}/MPISearch/faceboxlist.cc
	${CMAKE_CURRENT_SOURCE_DIR}/MPISearch/gentleFace1.cc
	${CMAKE_CURRENT_SOURCE_DIR}/MPISearch/mpisearchFaceDetector.cc
	${CMAKE_CURRENT_SOURCE_DIR}/MPISearch/roi.cc
	${CMAKE_CURRENT_SOURCE_DIR}/MPISearch/square.cc
	${CMAKE_CURRENT_SOURCE_DIR}/MPEyeFinder/EyeWeightsFiles/lefteye_eye_dist_eye_centered.cc
	${CMAKE_CURRENT_SOURCE_DIR}/MPEyeFinder/EyeWeightsFiles/lefteye_eye_dist_face_centered.cc
	${CMAKE_CURRENT_SOURCE_DIR}/MPEyeFinder/EyeWeightsFiles/lefteye_eye_only_eye_centered.cc
	${CMAKE_CURRENT_SOURCE_DIR}/MPEyeFinder/EyeWeightsFiles/lefteye_half_dist_eye_centered.cc
	${CMAKE_CURRENT_SOURCE_DIR}/MPEyeFinder/EyeWeightsFiles/lefteye_largest_2_eye_centered.cc
	${CMAKE_CURRENT_SOURCE_DIR}/MPEyeFinder/EyeWeightsFiles/lefteye_largest_eye_centered.cc
	${CMAKE_CURRENT_SOURCE_DIR}/MPEyeFinder/EyeWeightsFiles/lefteye_largest_face_centered.cc
	${CMAKE_CURRENT_SOURCE_DIR}/MPEyeFinder/EyeWeightsFiles/lefteye_smallest_eye_centered.cc
	${CMAKE_CURRENT_SOURCE_DIR}/MPEyeFinder/EyeWeightsFiles/righteye_eye_dist_eye_centered.cc
	${CMAKE_CURRENT_SOURCE_DIR}/MPEyeFinder/EyeWeightsFiles/righteye_eye_dist_face_centered.cc
	${CMAKE_CURRENT_SOURCE_DIR}/MPEyeFinder/EyeWeightsFiles/righteye_eye_only_eye_centered.cc
	${CMAKE_CURRENT_SOURCE_DIR}/MPEyeFinder/EyeWeightsFiles/righteye_half_dist_eye_centered.cc
	${CMAKE_CURRENT_SOURCE_DIR}/MPEyeFinder/EyeWeightsFiles/righteye_largest_2_eye_centered.cc
	${CMAKE_CURRENT_SOURCE_DIR}/MPEyeFinder/EyeWeightsFiles/righteye_largest_eye_centered.cc
	${CMAKE_CURRENT_SOURCE_DIR}/MPEyeFinder/EyeWeightsFiles/righteye_largest_face_centered.cc
	${CMAKE_CURRENT_SOURCE_DIR}/MPEyeFinder/EyeWeightsFiles/righteye_smallest_eye_centered.cc
	PARENT_SCOPE
)

set(MODULES_INCLUDS 
	${MODULES_INCLUDS} 
	${CMAKE_CURRENT_SOURCE_DIR}/MPISearch/
	${CMAKE_CURRENT_SOURCE_DIR}/MPEyeFinder/
	${CMAKE_CURRENT_SOURCE_DIR}/MPEyeFinder/EyeWeightsFiles/
PARENT_SCOPE)
set(MODULES_LIBS ${MODULES_LIBS} PARENT_SCOPE)