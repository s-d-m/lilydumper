TARGET_DIR := ../bin
TARGET := ${TARGET_DIR}/lilydumper

SRC :=  main.cc \
	svg_extractor.cc \
	notes_file_extractor.cc \
	utils.cc\
	chords_extractor.cc \
	cursor_boxes_extractor.cc \
	keyboard_events_extractor.cc \
	bar_number_events_extractor.cc \
	staff_num_to_instr_extractor.cc \
	file_exporter.cc \
	command_executor.cc \


OBJS := ${SRC:.cc=.o}

OPEN_PRELOADER_SRC := open_preloader.c
OPEN_PRELOADER_OBJS := ${OPEN_PRELOADER_SRC:.c=.o}
OPEN_PRELOADER_LIB := open_preloader.so

LIBS= -lpugixml -lstdc++fs


COVERAGE_HTML_DIR := ../COVERAGE_OUTPUT
LCOV=lcov
LCOV_FLAGS= --rc lcov_branch_coverage=1 \
	    --rc genhtml_branch_coverage=1 \
	    --rc geninfo_gcov_all_blocks=1 \
	    --directory .

GENHTML = genhtml
GENHTML_FLAGS = --branch-coverage

GPROF = gprof
PROFILING_OUTPUT = profiling_output.txt

TOOLCHAIN=gcc

CLANG_CC  = clang
CLANG_CXX = clang++
SCAN_BUILD = scan-build

ifeq ($(TOOLCHAIN),clang)
  CXX := ${CLANG_CXX}
  CXX_WARN_FLAGS ?= -Weverything \
		    -Wno-c++98-compat \
		    -Wno-c++98-compat-pedantic \
		    -Wno-c99-extensions \
		    -Wno-padded # remove padding warning
  CC := ${CLANG_CC}
  CC_WARN_FLAGS ?= -Weverything


  SANITIZERS = \
	-fsanitize=address \
	-fsanitize=undefined \

# -fsanitize=memory \
# -fsanitize=dataflow \
# -fsanitize=cfi \
# -fsanitize=safe-stack

# -fsanitize=thread #is incompatible with sanitize address
# -fsanitize=leak # leak sanitizer can't be used if either address or thread sanitizer is used

  HARDENING_FLAGS = \
	-fstack-protector-all \
	--param ssp-buffer-size=4 \
	-D_FORTIFY_SOURCE=2 \
	-fPIC

  LDFLAGS = \
	-z noexecstack \
	-z now\
	-z relro \
	-z nodlopen \
	-z nodump

else

  CC_WARN_FLAGS_FOR_GCC_4.9 := \
	-Werror \
	-Wpedantic \
	-Wall \
	-Wextra \
	-Wdouble-promotion \
	-Wformat=2 \
	-Winit-self \
	-Wuninitialized \
	-Wmissing-include-dirs \
	-Wswitch-default \
	-Wswitch-enum \
	-Wunused \
	-Wunused-parameter \
	-Wunknown-pragmas \
	-Wstrict-aliasing=3 \
	-Wstrict-overflow=5 \
	-Wsuggest-attribute=pure \
	-Wsuggest-attribute=const \
	-Wsuggest-attribute=noreturn \
	-Wsuggest-attribute=format \
	-Wmissing-format-attribute \
	-Wtrampolines \
	-Wfloat-equal \
	-Wtraditional-conversion \
	-Wundef \
	-Wshadow \
	-Wunsafe-loop-optimizations \
	-Wpointer-arith \
	-Wtype-limits \
	-Wbad-function-cast \
	-Wc++-compat \
	-Wcast-qual \
	-Wcast-align \
	-Wwrite-strings \
	-Wconversion \
	-Wdate-time \
	-Wjump-misses-init \
	-Wconversion \
	-Wlogical-op \
	-Waggregate-return \
	-Wstrict-prototypes \
	-Wold-style-definition \
	-Wmissing-declarations \
	-Wpacked \
	-Wpadded \
	-Wredundant-decls \
	-Wnested-externs \
	-Winline \
	-Winvalid-pch \
	-Wvariadic-macros \
	-Wvarargs \
	-Wvla \
	-Wdisabled-optimization \
	-Wunsuffixed-float-constants \
	-Wno-c++-compat

  CC_WARN_FLAGS_FOR_GCC_5 := \
	${CC_WARN_FLAGS_FOR_GCC_4.9} \
# All the new warning flags for gcc 5 are enabled by default or -Wall, hence nothing to add here

#-fcheck-pointer-bounds
# Pointer Bounds Checker, a bounds violation detector, has been added and can be enabled via
# -fcheck-pointer-bounds. Memory accesses are instrumented with run-time checks of used pointers
# against their bounds to detect pointer bounds violations (overflows). The Pointer Bounds Checker
# is available on x86/x86-64 GNU/Linux targets with a new ISA extension Intel MPX support. See the
# Pointer Bounds Checker Wiki page for more details.

  CC_WARN_FLAGS_FOR_GCC_6 := \
	${CC_WARN_FLAGS_FOR_GCC_5} \
	-Wshift-overflow=2 \
	-fdelete-null-pointer-checks \
	-Wnull-dereference \
	-Wduplicated-cond

  CC_WARN_FLAGS_FOR_GCC_7 := \
	${CC_WARN_FLAGS_FOR_GCC_6} \
	-Wduplicated-branches \
	-Wrestrict \
	-Walloc-zero \
	-Wformat-overflow=2 \
	-Wformat-signedness \
	-Wformat-truncation=2

  CC_WARN_FLAGS_FOR_GCC_8 := \
	${CC_WARN_FLAGS_FOR_GCC_7} \
	-Wmultistatement-macros \
	-Wcast-align=strict

# for g++ 4.9
  CXX_WARN_FLAGS_FOR_GCC_4.9 ?= \
	-Werror \
	-Wno-pedantic \
	-Wpointer-arith \
	-Wall \
	-Wextra \
	-Wformat=2 \
	-Wnonnull \
	-Winit-self \
	-Wmissing-include-dirs \
	-Wswitch-default \
	-Wswitch-enum \
	-Wuninitialized \
	-fstrict-aliasing \
	-Wstrict-overflow=5 \
	-Warray-bounds \
	-Wfloat-equal \
	-Wundef \
	-Wshadow \
	-Wcast-qual \
	-Wcast-align \
	-Wwrite-strings \
	-Wconversion \
	-Wsign-conversion \
	-Wmissing-declarations \
	-Wpacked \
	-Wredundant-decls \
	-Winvalid-pch \
	-Wlong-long \
	-Wvarargs \
	-Wvla \
	-funsafe-loop-optimizations \
	-Wunsafe-loop-optimizations \
	-Wdouble-promotion \
	-Wsuggest-attribute=pure \
	-Wsuggest-attribute=const \
	-Wsuggest-attribute=noreturn \
	-Wsuggest-attribute=format \
	-Wtrampolines \
	-Wzero-as-null-pointer-constant \
	-Wuseless-cast \
	-Wlogical-op \
	-Wvector-operation-performance \
	-Wabi=11 \
	-Wctor-dtor-privacy \
	-Wnoexcept \
	-Weffc++ \
	-Wstrict-null-sentinel \
	-Wold-style-cast \
	-Woverloaded-virtual \
	-Wsign-promo

#	unfortunately g++ reports padding warnings with lambda's variable capturing
#	-Wpadded \

# for G++ 5.1/5.2
  CXX_WARN_FLAGS_FOR_GCC_5 := \
	${CXX_WARN_FLAGS_FOR_GCC_4.9} \
	-Wswitch-bool \
	-Wlogical-not-parentheses \
	-Wsizeof-array-argument \
	-Wbool-compare  \
	-Wodr \
	-Wsuggest-final-types \
	-Wsuggest-final-methods \
	-Wsuggest-override

  CXX_WARN_FLAGS_FOR_GCC_6 := \
	${CXX_WARN_FLAGS_FOR_GCC_5} \
	-Wnull-dereference -fdelete-null-pointer-checks \
	-Wduplicated-cond \
	-Wplacement-new=2 \
	-Wconditionally-supported \
	-fsized-deallocation \
	-Wsized-deallocation \

  CXX_WARN_FLAGS_FOR_GCC_7 := \
	${CXX_WARN_FLAGS_FOR_GCC_6} \
	-Wduplicated-branches \
	-Wrestrict \
	-Wregister \
	-Wdangling-else \
	-Walloc-size-larger-than=1073741824 \
	-Walloc-zero \
	-Walloca \
	-Wformat-overflow=2 \
	-Wformat-truncation=2 \
	-Wstringop-overflow=4 \
	-Waligned-new
#	-Walloc-size-larger-than=1073741824 \ # Allocating more than 1GiB is likely a bug

  CXX_WARN_FLAGS_FOR_GCC_8 := \
	${CXX_WARN_FLAGS_FOR_GCC_7} \
	-Wmultistatement-macros \
	-Wcast-align=strict \

  HARDENING_FLAGS_FOR_GCC_4.9 := \
	-fstack-protector-all \
	--param ssp-buffer-size=4 \
	-fPIC

ifeq ($(BUILD),release)
  HARDENING_FLAGS_FOR_GCC_4.9 += -D_FORTIFY_SOURCE=2
endif

  HARDENING_FLAGS_FOR_GCC_5 := ${HARDENING_FLAGS_FOR_GCC_4.9}

  HARDENING_FLAGS_FOR_GCC_6 := ${HARDENING_FLAGS_FOR_GCC_5}

  HARDENING_FLAGS_FOR_GCC_7 := ${HARDENING_FLAGS_FOR_GCC_6}

  HARDENING_FLAGS_FOR_GCC_8 := \
	${HARDENING_FLAGS_FOR_GCC_7} \
	-fcf-protection=full \
	-fstack-clash-protection

  GCC_VERSION := $(shell "${CXX}" -v 2>&1 | gawk '($$1 == "gcc") && ($$2 == "version") { sub(/\..*/, "", $$3) ; print $$3 ; exit }' )

  ifeq ($(shell [ "${GCC_VERSION}" -ge 8 ] && echo $$?),0)
	CXX_WARN_FLAGS := ${CXX_WARN_FLAGS_FOR_GCC_8}
	CC_WARN_FLAGS := ${CC_WARN_FLAGS_FOR_GCC_8}
	HARDENING_FLAGS := ${HARDENING_FLAGS_FOR_GCC_8}
  endif

  ifeq (${GCC_VERSION},7)
	CXX_WARN_FLAGS := ${CXX_WARN_FLAGS_FOR_GCC_7}
	CC_WARN_FLAGS := ${CC_WARN_FLAGS_FOR_GCC_7}
	HARDENING_FLAGS := ${HARDENING_FLAGS_FOR_GCC_7}
  endif

  ifeq (${GCC_VERSION},6)
	CXX_WARN_FLAGS := ${CXX_WARN_FLAGS_FOR_GCC_6}
	CC_WARN_FLAGS := ${CC_WARN_FLAGS_FOR_GCC_6}
	HARDENING_FLAGS := ${HARDENING_FLAGS_FOR_GCC_6}
  endif

  ifeq (${GCC_VERSION},5)
	CXX_WARN_FLAGS := ${CXX_WARN_FLAGS_FOR_GCC_5}
	CC_WARN_FLAGS := ${CC_WARN_FLAGS_FOR_GCC_5}
	HARDENING_FLAGS := ${HARDENING_FLAGS_FOR_GCC_5}
  endif

  ifeq (${CXX_WARN_FLAGS},)
	CXX_WARN_FLAGS := ${CXX_WARN_FLAGS_FOR_GCC_4.9}
	CC_WARN_FLAGS := ${CC_WARN_FLAGS_FOR_GCC_4.9}
	HARDENING_FLAGS := ${HARDENING_FLAGS_FOR_GCC_4.9}
  endif


# sanitizers on g++5.2
  SANITIZERS = \
	-fsanitize=address \
	-fsanitize=undefined \
	-fbounds-check \
# -fcheck-pointer-bounds -mmpx # pointer checker is not supported with address sanitiser
# -fsanitize=thread #is incompatible with sanitize address
# -fsanitize=leak # leak sanitizer can't be used if either address or thread sanitizer is used

  LDFLAGS = \
	-pie \
	-z noexecstack \
	-z now\
	-z relro \
	-z nodlopen \
	-z nodump
endif

BUILD ?= debug

CFLAGS := -std=c11 -Werror ${HARDENING_FLAGS} ${CC_WARN_FLAGS} # ${SANITIZERS}
CXXFLAGS := -std=c++1z -Werror -fno-rtti -fstrict-enums ${HARDENING_FLAGS} ${CXX_WARN_FLAGS} ${SANITIZERS}

ifeq ($(BUILD),release)
  CXXFLAGS += -O3 -flto
  CFLAGS += -O3 -flto
  LDFLAGS += -s

  ifeq ($(TOOLCHAIN),clang)
    # clang-4.0 complains about recursive macro '#define stderr stderr' when compiling the
    # open-preloader in release mode. Disable the warning so it compiles.
    CFLAGS += -Wno-disabled-macro-expansion
  else
    # g++-6 complains at link time when compiling in release mode with the following error
    #
    # main.cc:154:5: error: assuming that the loop counter does not overflow [-Werror=unsafe-loop-optimizations]
    #  int main(int argc, const char * const * argv)
    #
    # lto1: all warnings being treated as errors
    # lto-wrapper: fatal error: g++-6 returned 1 exit status
    LDFLAGS += -Wno-unsafe-loop-optimizations
  endif

else
  CXXFLAGS += -O0 -ggdb3
  CFLAGS += -O0 -ggdb3
endif

ifeq ($(GCOV),1)
  CXXFLAGS += --coverage
  CFLAGS += --coverage
endif

ifeq ($(GPROF),1)
  CXXFLAGS += -pg
  CFLAGS += -pg
endif

ifeq ($(32_BITS),1)
  CXXFLAGS += -m32
  CFLAGS += -m32
endif


all lilydumper: ${OPEN_PRELOADER_LIB} ${TARGET}

${TARGET}: ${OBJS} ${OPEN_PRELOADER_LIB} command_executor.o
	-mkdir -p ${TARGET_DIR}
	${CXX} ${CXXFLAGS} -fPIE ${LDFLAGS} -o ${TARGET} ${OBJS} ${LIBS}

${OPEN_PRELOADER_LIB}: ${OPEN_PRELOADER_OBJS} event_listener.h
	${CC} ${CFLAGS} ${LDFLAGS} -shared -ldl -MD -o "$@" ${OPEN_PRELOADER_OBJS}

event_listener.h: event-listener.scm
	xxd -i "$<" "$@"
	sed -i -e 's/unsigned char event_listener_scm/static constexpr const unsigned char event_listener_scm/' \
	       -e 's/unsigned int event_listener_scm_len = /static constexpr const unsigned int event_listener_scm_len = /' "$@"

open_preloader.h: ${OPEN_PRELOADER_LIB}
	xxd -i "$<" "$@"
	sed -i -e 's/unsigned char open_preloader_so/static constexpr const unsigned char open_preloader_so/' \
	       -e 's/unsigned int open_preloader_so_len = /static constexpr const unsigned int open_preloader_so_len = /' "$@"

command_executor.o: event_listener.h
command_executor.o: open_preloader.h

%.o: %.c
	${CC} ${CFLAGS} -MD -c -o "$@" "$<"
	@cp $*.d $*.P; \
	   sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	       -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	   rm -f $*.d

%.o: %.cc
	${CXX} ${CXXFLAGS} ${INCLUDES} -MD -c -o "$@" "$<"
	 @cp $*.d $*.P; \
	   sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	       -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	   rm -f $*.d

scan-build:
	${SCAN_BUILD} -analyze-headers --use-c++=${CLANG_CXX} --use-cc=${CLANG_CC} --status-bugs --keep-going  make -B TOOLCHAIN=clang

${COVERAGE_HTML_DIR}:
	[ -d "$@" ] || mkdir -p "$@"

coverage: | ${COVERAGE_HTML_DIR}
	${MAKE} GCOV=1 -B CXX=g++
	${LCOV} ${LCOV_FLAGS} --zerocounters
	${MAKE} check
	${LCOV} ${LCOV_FLAGS} --no-external --capture --output-file "${TARGET}.info"
	${GENHTML} ${GENHTML_FLAGS} --output-directory="${COVERAGE_HTML_DIR}" "${TARGET}.info"

profiling:
	${MAKE} GPROF=1 BUILD=release SANITIZERS= -B CXX=g++
	-rm -f gmon.out
	${TARGET}
	${GPROF} "${TARGET}" gmon.out > "${PROFILING_OUTPUT}"

clean:
	rm -f ${TARGET} ${OBJS} $(SRC:%.cc=$/%.P) "${COVERAGE_HTML_DIR}" "${PROFILING_OUTPUT}"

.PHONY: all clean  scan-build coverage profiling

.SUFFIXES:

-include $(SRC:%.cc=$/%.P)
