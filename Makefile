################################################################################
lib.name = n_lib
cflags = -mavx
class.sources = \
n_canvas.c \
n_clock~.c \
n_cnvinfo.c \
n_date.c \
n_demux~.c \
n_env.c \
n_img.c \
n_knob.c \
n_matrix~.c \
n_mux~.c \
n_p2f.c	\
n_peak~.c \
n_print.c \
n_r.c \
n_random.c \
n_rec1p~.c \
n_sysgui.c \
n_ds~.c \
n_conv~.c \
n_euklid.c \
n_graincloud~.c \
n_sampler~.c \
n_seq_drum.c \
n_seq_one.c \
n_seq_mod.c
sources = \
./include/*
datafiles = \
n_lib-meta.pd \
README.md \
LICENSE.txt

################################################################################
PDLIBBUILDER_DIR=pd-lib-builder/
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder
