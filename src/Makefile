#
# use `make SINGLE_THREAD=1` to build version with single threaded renderer
# use `make USE_AVX2=1` to build version with AVX2 instructions
# use `make USE_SSE41=1` to build version with SSE4.1 instructions
# use `make USE_SSE2=1` to build version with SSE2 instructions
# use `make USE_NEON=1` to build version with NEON instructions
#

all: fmtg_pcmconvert fmtg_alsadrv

CFLAGS = -c -O3 -Wall
LDFLAGS =

ifndef SINGLE_THREAD
	CFLAGS += -DMULTI_THREAD
	LDFLAGS += -lpthread
endif

CXXLAGS = $(CFLAGS)
CXXLAGS += -x c++ -Wno-multichar -include compat.h -I../FM-tone-generator

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
pcmconvert_object_files := midi_loader.o fmtg_pcmconvert.o
alsadrv_object_files := fmtg_alsadrv.o

Channel8.o: ../FM-tone-generator/Channel8.mm compat.h ../FM-tone-generator/Channel8.h ../FM-tone-generator/Operator8.h ../FM-tone-generator/types.h
	$(CXX) $(CXXLAGS) -include Channel8-platform.h -Wno-parentheses -flax-vector-conversions ../FM-tone-generator/Channel8.mm -o $@

ChannelManager.o: ../FM-tone-generator/ChannelManager.mm compat.h ../FM-tone-generator/ChannelManager.h ../FM-tone-generator/Channel8.h ../FM-tone-generator/Operator8.h ../FM-tone-generator/types.h
	$(CXX) $(CXXLAGS) ../FM-tone-generator/ChannelManager.mm -o $@

Operator8.o: ../FM-tone-generator/Operator8.mm compat.h ../FM-tone-generator/Operator8.h ../FM-tone-generator/types.h
	$(CXX) $(CXXLAGS) ../FM-tone-generator/Operator8.mm -o $@

FMMidi.o: FMMidi.cc compat.h FMMidi.h ../FM-tone-generator/ChannelManager.h ../FM-tone-generator/Channel8.h ../FM-tone-generator/Operator8.h ../FM-tone-generator/types.h
	$(CXX) $(CXXLAGS) FMMidi.cc -o $@

midi_loader.o: midi_loader.c midi_loader.h
	$(CC) $(CFLAGS) midi_loader.c -o $@

fmtg_pcmconvert.o: fmtg_pcmconvert.c midi_loader.h FMMidi.h
	$(CC) $(CFLAGS) fmtg_pcmconvert.c -o $@

fmtg_alsadrv.o: fmtg_alsadrv.c FMMidi.h
	$(CC) $(CFLAGS) fmtg_alsadrv.c -o $@

fmtg_pcmconvert: $(common_object_files) $(pcmconvert_object_files)
	$(CXX) -s -o fmtg_pcmconvert $(common_object_files) $(pcmconvert_object_files) $(LDFLAGS)

fmtg_alsadrv: $(common_object_files) fmtg_alsadrv.o
	$(CXX) -s -o fmtg_alsadrv $(common_object_files) $(alsadrv_object_files) -lasound -lpthread $(LDFLAGS)

.PHONY: clean
clean:
	rm -f fmtg_pcmconvert fmtg_alsadrv $(common_object_files) $(pcmconvert_object_files) $(alsadrv_object_files)
