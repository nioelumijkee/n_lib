################################################################################
lib.name = n_lib
cflags = 
class.sources = \
n_cnvinfo.c \
n_date.c \
n_demux~.c \
n_env.c \
n_life.c \
n_matrix~.c \
n_mux~.c \
n_p2f.c	\
n_peak~.c \
n_print.c \
n_r.c \
n_random.c \
n_sysgui.c
sources = \
./include/*
datafiles = \
n_lib-meta.pd \
README.md \
LICENSE.txt

################################################################################
PDLIBBUILDER_DIR=pd-lib-builder/
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder
