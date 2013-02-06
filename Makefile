APP_PATH := $(PWD)

LIB_PATH := -L ./ -L /usr/lib64/ -L /usr/local/lib/ 

LIB := -lpthread
#-lprotobuf -ltcmalloc -lprofiler

INCLUDE := -I ./ -I /usr/include/ 

#CXXFLAGS =-g -Wall -fPIC $(INCLUDE)
CXXFLAGS = -Wall -fPIC -O2 $(INCLUDE)

OBJ = at_exit.o \
	atomicops_internals_x86_gcc.o \
	bloom_filter.o \
	counter.o \
	debug_util.o \
	demangle.o \
	dmg_fp.o \
	dynamic_annotations.o \
	env.o \
	file_base.o \
	file.o \
	file_enumerator.o \
	file_posix.o \
	flags.o \
	flags_reporting.o \
	icu_utf.o \
	logging.o \
	net.o \
	prtime.o \
	safe_strerror_posix.o \
	string16.o \
	string_piece.o \
	string_split.o \
	string_util.o \
	symbolize.o \
	thread.o \
	thread_pool.o \
	time.o \
	utf.o \
	utf_string_conversions.o \
	utf_string_conversion_utils.o

LIBNAME=base.a

STLIB_MAKE_CMD=ar rcs $(LIBNAME) 

$(LIBNAME): $(OBJ)
	$(STLIB_MAKE_CMD) $(OBJ) 

clean:
	rm -rf $(LIBNAME) *.o 
