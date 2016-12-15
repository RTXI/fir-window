PLUGIN_NAME = fir_window

RTXI_INCLUDES =

HEADERS = fir-window.h\

SOURCES = fir-window.cpp \
          moc_fir-window.cpp\

LIBS = -lrtdsp

### Do not edit below this line ###

include $(shell rtxi_plugin_config --pkgdata-dir)/Makefile.plugin_compile
