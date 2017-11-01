CUDA_PATH ?= /opt/cuda

HOST_ARCH := $(shell uname -m)
TARGET_ARCH ?= $(HOST_ARCH)
TARGET_SIZE := 64

HOST_COMPILER ?= g++
NVCC := $(CUDA_PATH)/bin/nvcc -ccbin $(HOST_COMPILER)

CPPFLAGS := -O0 -fno-inline -Wall -g -pthread -std=c++11
LDFLAGS := -g -L/usr/local/lib -L/opt/cuda/lib64
INCLUDES := -I/usr/local/include -I$(CUDA_PATH)/include
LIBRARIES := -lboost_fiber -lboost_context -lboost_system -lboost_filesystem -lcudart

NVCCFLAGS := -m${TARGET_SIZE} -std=c++11 --expt-relaxed-constexpr

ALL_CCFLAGS := $(NVCCFLAGS)
ALL_CCFLAGS += $(addprefix -Xcompiler ,$(CXXFLAGS))

ALL_LDFLAGS := $(ALL_CXXFLAGS)
ALL_LDFLAGS += $(addprefix -Xlinker ,$(LDFLAGS))

SM ?= 61
GENCODE_FLAGS += -gencode arch=compute_$(SM),code=compute_$(SM)

all: build

build: single_stream multiple_streams

single_stream.o:single_stream.cu
	$(NVCC) $(INCLUDES) $(ALL_CCFLAGS) $(GENCODE_FLAGS) -o $@ -c $<

single_stream: single_stream.o
	$(NVCC) $(ALL_LDFLAGS) $(GENCODE_FLAGS) -o $@ $+ $(LIBRARIES)

multiple_streams.o:multiple_streams.cu
	$(NVCC) $(INCLUDES) $(ALL_CCFLAGS) $(GENCODE_FLAGS) -o $@ -c $<

multiple_streams: multiple_streams.o
	$(NVCC) $(ALL_LDFLAGS) $(GENCODE_FLAGS) -o $@ $+ $(LIBRARIES)

clean:
	rm -f single_stream single_stream.o multiple_streams multiple_streams.o

clobber: clean
