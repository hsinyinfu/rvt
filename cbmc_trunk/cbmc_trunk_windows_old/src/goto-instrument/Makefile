SRC = goto_instrument_main.cpp goto_instrument_parseoptions.cpp \
      document_properties.cpp goto_instrument_languages.cpp \
      uninitialized.cpp full_slicer.cpp k_induction.cpp \
      object_id.cpp show_locations.cpp points_to.cpp \
      alignment_checks.cpp race_check.cpp rw_set.cpp \
      nondet_volatile.cpp interrupt.cpp function.cpp branch.cpp \
      mmio.cpp stack_depth.cpp nondet_static.cpp \
      concurrency.cpp dump_c.cpp dot.cpp havoc_loops.cpp \
      wmm/weak_memory.cpp wmm/fence.cpp wmm/event_graph.cpp \
      wmm/goto2graph.cpp wmm/data_dp.cpp wmm/abstract_event.cpp \
      wmm/instrumenter_strategies.cpp wmm/cycle_collection.cpp \
      call_sequences.cpp unwind.cpp \
      accelerate/accelerate.cpp accelerate/polynomial.cpp \
      accelerate/scratch_program.cpp accelerate/polynomial_accelerator.cpp \
      accelerate/util.cpp count_eloc.cpp reachability_slicer.cpp

OBJ += ../ansi-c/ansi-c$(LIBEXT) \
      ../linking/linking$(LIBEXT) \
      ../big-int/big-int$(LIBEXT) \
      ../goto-programs/goto-programs$(LIBEXT) \
      ../goto-symex/goto-symex$(LIBEXT) \
      ../assembler/assembler$(LIBEXT) \
      ../pointer-analysis/pointer-analysis$(LIBEXT) \
      ../analyses/analyses$(LIBEXT) \
      ../langapi/langapi$(LIBEXT) \
      ../util/util$(LIBEXT) \
      ../solvers/solvers$(LIBEXT)

INCLUDES= -I ..

LIBS =

CLEANFILES = goto-instrument$(EXEEXT)

include ../config.inc
include ../common

all: goto-instrument$(EXEEXT)

ifneq ($(wildcard ../cpp/Makefile),)
  OBJ += ../cpp/cpp$(LIBEXT)
  CP_CXXFLAGS += -DHAVE_CPP
endif

ifneq ($(wildcard ../java/Makefile),)
  OBJ += ../java/java$(LIBEXT)
  CP_CXXFLAGS += -DHAVE_JAVA
endif

ifneq ($(wildcard ../specc/Makefile),)
  OBJ += ../specc/specc$(LIBEXT)
  CP_CXXFLAGS += -DHAVE_SPECC
endif

ifneq ($(wildcard ../php/Makefile),)
  OBJ += ../php/php$(LIBEXT)
  CP_CXXFLAGS += -DHAVE_PHP
endif

###############################################################################

goto-instrument$(EXEEXT): $(OBJ)
	$(LINKBIN)

.PHONY: goto-instrument-mac-signed

goto-instrument-mac-signed: goto-instrument$(EXEEXT)
	codesign -v -s $(OSX_IDENTITY) goto-instrument$(EXEEXT)

