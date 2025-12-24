#
# use `make SINGLE_THREAD=1` to build version with single threaded renderer
# use `make USE_AVX2=1` to build version with AVX2 instructions
# use `make USE_SSE41=1` to build version with SSE4.1 instructions
# use `make USE_SSE2=1` to build version with SSE2 instructions
# use `make USE_NEON=1` to build version with NEON instructions
#

all: fmtg_coredrv

CFLAGS = -c -O3 -Wall
LDFLAGS =

ifndef SINGLE_THREAD
	CFLAGS += -DMULTI_THREAD
	LDFLAGS += -lpthread
endif

CXXLAGS = $(CFLAGS)
CXXLAGS += -x c++ -Wno-multichar -include ../compat.h -I../../FM-tone-generator

ifdef USE_AVX2
	CXXLAGS += -mavx2
endif
ifdef USE_SSE41
	CXXLAGS += -msse4.1
endif
ifdef USE_SSE2
	CXXLAGS += -msse2
endif
ifdef USE_NEON
	CXXLAGS += -mfpu=neon -faligned-new
endif

common_object_files := Channel8.o ChannelManager.o Operator8.o FMMidi.o
coredrv_object_files := fmtg_coredrv.o

Channel8.o: ../../FM-tone-generator/Channel8.mm ../compat.h ../../FM-tone-generator/Channel8.h ../../FM-tone-generator/Operator8.h ../../FM-tone-generator/types.h
	$(CXX) $(CXXLAGS) -DTARGET_OS_IPHONE=0 -include ../Channel8-platform.h -Wno-parentheses -flax-vector-conversions ../../FM-tone-generator/Channel8.mm -o $@

ChannelManager.o: ../../FM-tone-generator/ChannelManager.mm ../compat.h ../../FM-tone-generator/ChannelManager.h ../../FM-tone-generator/Channel8.h ../../FM-tone-generator/Operator8.h ../../FM-tone-generator/types.h
	$(CXX) $(CXXLAGS) ../../FM-tone-generator/ChannelManager.mm -o $@

Operator8.o: ../../FM-tone-generator/Operator8.mm ../compat.h ../../FM-tone-generator/Operator8.h ../../FM-tone-generator/types.h
	$(CXX) $(CXXLAGS) ../../FM-tone-generator/Operator8.mm -o $@

FMMidi.o: ../FMMidi.cc ../compat.h ../FMMidi.h ../../FM-tone-generator/ChannelManager.h ../../FM-tone-generator/Channel8.h ../../FM-tone-generator/Operator8.h ../../FM-tone-generator/types.h
	$(CXX) $(CXXLAGS) ../FMMidi.cc -o $@

fmtg_coredrv.o: fmtg_coredrv.c ../FMMidi.h
	$(CC) $(CFLAGS) fmtg_coredrv.c -I.. -o $@

fmtg_coredrv: $(common_object_files) fmtg_coredrv.o
	$(CXX) -o fmtg_coredrv $(common_object_files) $(coredrv_object_files) $(LDFLAGS) -framework CoreMIDI -framework CoreFoundation -framework AudioToolbox

.PHONY: clean
clean:
	rm -f fmtg_coredrv $(common_object_files) $(coredrv_object_files)
