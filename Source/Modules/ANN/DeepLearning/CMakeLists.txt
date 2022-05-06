set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(HDF5)

if(HDF5_FOUND)
    set(SOURCE
    	${SOURCE}
    	${CMAKE_CURRENT_SOURCE_DIR}/DeepLearning.cc
    	PARENT_SCOPE
    )
    
    set(MODULES_INCLUDS
        ${MODULES_INCLUDS}
        ${HDF5_INCLUDE_DIRS}
        PARENT_SCOPE
        )

    set(MODULES_LIBS
        ${MODULES_LIBS}
        ${HDF5_LIBRARIES}
        PARENT_SCOPE
        )
else()
    message(STATUS "Ikaros: HDF5 not found. DeepLearning module is not included into the build. Visit www.ikaros-project.org for information of module dependencies." )
endif(HDF5_FOUND)

