
HOST_GCC=g++11
TARGET_GCC=gcc11
EXE=metadataTestProgramGCC
READER_EXE=lecturaELFBinPayload

INCLUDE_SYS_DIR= /usr/src/sys

PLUGIN_SOURCE_FILES= insertPayload_GCCPlugin.c
PLUGIN_NAME=InsertPayload_GCCPlugin
EXE_SRC= test/src/prueba.c
READER_EXE_SRC=lecturaELFBinPayload.c

#DIR_GCC_PLUGIN_HEADER_FBSD13=/usr/local/lib/gcc11/gcc/x86_64-portbld-freebsd13.0/11.2.0/plugin/include
#DIR_GCC_PLUGIN_HEADER_FBSD12=/usr/local/lib/gcc11/gcc/x86_64-portbld-freebsd12.2/11.2.0/plugin/include

CXXFLAGS+= -fPIC -fno-rtti -O2 -I$(INCLUDE_SYS_DIR)

.if defined(ARGS)
PLUGIN_ARGS+= $(ARGS)
.else
.endif



.if defined(DIR_GCC_PLUGIN_HEADER)
CXXFLAGS+= -I$(DIR_GCC_PLUGIN_HEADER)
.else
#.error "No GCC plugin header path has been provided"
.endif

###### LA FUNCION DEL PAYLOAD ESTA DADA EN LA KEY DEL ARGUMENTO DEL PLUGIN (-fplugin-arg-<NOMBRE PLUGIN>-<NUMERO FUNC>)

.PHONY: all test clean

$(PLUGIN_NAME).so: $(PLUGIN_SOURCE_FILES)
	$(HOST_GCC) -shared $(CXXFLAGS) $(PLUGIN_SOURCE_FILES) -o $@ 
	
clean: 
	rm -f $(EXE) $(PLUGIN_NAME).so $(READER_EXE)

test:
	$(TARGET_GCC) -o $(EXE) $(EXE_SRC) -fplugin=./$(PLUGIN_NAME).so $(PLUGIN_ARGS) -I$(INCLUDE_SYS_DIR)

hexread:
	readelf -x .metadata $(EXE)

sections:
	readelf -SW $(EXE)

read:
	$(TARGET_GCC) -o $(READER_EXE) $(READER_EXE_SRC) -I$(INCLUDE_SYS_DIR)
	./$(READER_EXE)