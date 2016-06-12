# -------------------------------------------------------------------------------
# Project configuration.
# Created: 2015-09-22
# -------------------------------------------------------------------------------

QT += core gui svg

TARGET = render
DESTDIR = bin
TEMPLATE = app
CONFIG += warn_on debug qwt

SOURCES += \
    src/main.cpp \
    src/Chromosome.cpp

HEADERS += \
        src/Chromosome.hpp

OTHER_FILES += distance.cu

#QMAKE_CFLAGS+=-pg
#QMAKE_CXXFLAGS+=-pg
#QMAKE_LFLAGS+=-pg

OBJECTS_DIR = build
MOC_DIR     = build
RCC_DIR     = build
UI_DIR      = build
CUDA_OBJECTS_DIR = build

# CUDA
CUDA_SOURCES += src/distance.cu
CUDA_DIR = /opt/cuda
NVCC_OPTIONS = --use_fast_math -std=c++11 -Xcompiler -D__CORRECT_ISO_CPP11_MATH_H_PROTO -O2

# include paths
INCLUDEPATH += \
            $$CUDA_DIR/include \
            $$CUDA_DIR/samples/commom/inc

QMAKE_LIBDIR += $$CUDA_DIR/lib64
LIBS += -lcuda -lcudart

CONFIG(debug, debug|release){
    cuda_d.input = CUDA_SOURCES
    cuda_d.output = $$CUDA_OBJECTS_DIR/${QMAKE_FILE_BASE}_cuda.o
    cuda_d.commands = $$CUDA_DIR/bin/nvcc -D_DEBUG $$NVCC_OPTIONS $$CUDA_INCLUDE_PATHS $$LIBS -c -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
    cuda_d.dependency_type = TYPE_C
    QMAKE_EXTRA_COMPILERS += cuda_d
}
else{
    cuda_d.input = CUDA_SOURCES
    cuda_d.output = $$CUDA_OBJECTS_DIR/${QMAKE_FILE_BASE}_cuda.o
    cuda_d.commands = $$CUDA_DIR/bin/nvcc $$NVCC_OPTIONS $$CUDA_INCLUDE_PATHS $$LIBS -c -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
    cuda_d.dependency_type = TYPE_C
    QMAKE_EXTRA_COMPILERS += cuda_d
}
