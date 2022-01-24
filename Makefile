################################################################################
lib.name = n_lib
cflags = 
class.sources = \
n_canvas.c \
n_scope~.c \
n_spectr~.c \
n_life.c \
n_date.c \
n_sysgui.c \
n_cnvinfo.c \
n_cnvrcv.c \
n_env.c \
n_mux~.c \
n_demux~.c \
n_key2n.c \
n_print.c \
n_clock~.c \
n_peak~.c \
n_r.c \
n_random.c \
n_array.c \
n_peakdetect.c \
n_stretch.c \
n_p2f.c	\
n_knob.c \
n_browser.c
sources = \
./include/*
datafiles = \
README.md \
LICENSE.txt

################################################################################
PDLIBBUILDER_DIR=pd-lib-builder/
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder
