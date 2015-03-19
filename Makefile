PLUGIN_NAME = fir_window

RTXI_INCLUDES = /usr/local/lib/rtxi_includes

HEADERS = fir-window.h\
          $(RTXI_INCLUDES)/DSP/gen_win.h\
			 $(RTXI_INCLUDES)/DSP/rectnglr.h\
			 $(RTXI_INCLUDES)/DSP/trianglr.h\
			 $(RTXI_INCLUDES)/DSP/hamming.h\
			 $(RTXI_INCLUDES)/DSP/hann.h\
			 $(RTXI_INCLUDES)/DSP/dolph.h\
			 $(RTXI_INCLUDES)/DSP/kaiser.h\
			 $(RTXI_INCLUDES)/DSP/acosh.h\
			 $(RTXI_INCLUDES)/DSP/firideal.h\
			 $(RTXI_INCLUDES)/DSP/fir_dsgn.h\
			 $(RTXI_INCLUDES)/DSP/lin_dsgn.h\
			 $(RTXI_INCLUDES)/DSP/fir_resp.h\

SOURCES = fir-window.cpp \
          $(RTXI_INCLUDES)/DSP/gen_win.cpp\
			 $(RTXI_INCLUDES)/DSP/rectnglr.cpp\
			 $(RTXI_INCLUDES)/DSP/trianglr.cpp\
			 $(RTXI_INCLUDES)/DSP/hamming.cpp\
			 $(RTXI_INCLUDES)/DSP/hann.cpp\
			 $(RTXI_INCLUDES)/DSP/dolph.cpp\
			 $(RTXI_INCLUDES)/DSP/kaiser.cpp\
			 $(RTXI_INCLUDES)/DSP/acosh.cpp\
			 $(RTXI_INCLUDES)/DSP/firideal.cpp\
			 $(RTXI_INCLUDES)/DSP/fir_dsgn.cpp\
			 $(RTXI_INCLUDES)/DSP/lin_dsgn.cpp\
			 $(RTXI_INCLUDES)/DSP/fir_resp.cpp\
			 moc_fir-window.cpp\

LIBS = -lqwt

### Do not edit below this line ###

include $(shell rtxi_plugin_config --pkgdata-dir)/Makefile.plugin_compile
