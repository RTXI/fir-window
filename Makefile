PLUGIN_NAME = fir_window

HEADERS = fir-window.h\
          /usr/local/lib/rtxi_includes/DSP/gen_win.h\
			 /usr/local/lib/rtxi_includes/DSP/rectnglr.h\
			 /usr/local/lib/rtxi_includes/DSP/trianglr.h\
			 /usr/local/lib/rtxi_includes/DSP/hamming.h\
			 /usr/local/lib/rtxi_includes/DSP/hann.h\
			 /usr/local/lib/rtxi_includes/DSP/dolph.h\
			 /usr/local/lib/rtxi_includes/DSP/kaiser.h\
			 /usr/local/lib/rtxi_includes/DSP/acosh.h\
			 /usr/local/lib/rtxi_includes/DSP/firideal.h\
			 /usr/local/lib/rtxi_includes/DSP/fir_dsgn.h\
			 /usr/local/lib/rtxi_includes/DSP/lin_dsgn.h\
			 /usr/local/lib/rtxi_includes/DSP/fir_resp.h\

SOURCES = fir-window.cpp \
          /usr/local/lib/rtxi_includes/DSP/gen_win.cpp\
			 /usr/local/lib/rtxi_includes/DSP/rectnglr.cpp\
			 /usr/local/lib/rtxi_includes/DSP/trianglr.cpp\
			 /usr/local/lib/rtxi_includes/DSP/hamming.cpp\
			 /usr/local/lib/rtxi_includes/DSP/hann.cpp\
			 /usr/local/lib/rtxi_includes/DSP/dolph.cpp\
			 /usr/local/lib/rtxi_includes/DSP/kaiser.cpp\
			 /usr/local/lib/rtxi_includes/DSP/acosh.cpp\
			 /usr/local/lib/rtxi_includes/DSP/firideal.cpp\
			 /usr/local/lib/rtxi_includes/DSP/fir_dsgn.cpp\
			 /usr/local/lib/rtxi_includes/DSP/lin_dsgn.cpp\
			 /usr/local/lib/rtxi_includes/DSP/fir_resp.cpp\
			 moc_fir-window.cpp\

LIBS = -lqwt

### Do not edit below this line ###

include $(shell rtxi_plugin_config --pkgdata-dir)/Makefile.plugin_compile
