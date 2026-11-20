set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(Nucleus)
add_subdirectory(NeuralTissue)
add_subdirectory(SCN)
add_subdirectory(SpikingPopulation)
add_subdirectory(IntegrateAndFirePopulation)
add_subdirectory(ConductanceNeuronPopulation)
add_subdirectory(RatePopulation)
add_subdirectory(SpikeGenerator)
add_subdirectory(LeakyIntegrator)
add_subdirectory(Cerebellum)
add_subdirectory(Concentration)
add_subdirectory(DeepNetwork)
add_subdirectory(ConvolutionalVariationalAutoEncoder)
add_subdirectory(Delta)

add_subdirectory(FieldExtraction)

add_subdirectory(PatchCodebook)

add_subdirectory(PatchReconstruction)

add_subdirectory(BatchedProjection)

add_subdirectory(CategoryEvidence)

add_subdirectory(CategoryCompetition)

add_subdirectory(ReferenceSelection)

add_subdirectory(CategoryPrediction)

add_subdirectory(ContentIntegration)

add_subdirectory(SensoryMismatch)

add_subdirectory(CategoryLocationBinding)

add_subdirectory(SpatialCompetition)

add_subdirectory(SpatialCategoryPooling)

add_subdirectory(SpatialGain)

add_subdirectory(FeedbackAlignment)

add_subdirectory(FieldReconstruction)

set(SOURCE ${SOURCE} PARENT_SCOPE)
set(MODULES_INCLUDS ${MODULES_INCLUDS} PARENT_SCOPE)
set(MODULES_LIBS ${MODULES_LIBS} PARENT_SCOPE)
