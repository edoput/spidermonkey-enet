CXX=g++
CPPFLAGS := $(CPPFLAGS) -g $(shell pkg-config --cflags mozjs-78 libenet) -I./includes
CXXFLAGS := $(CXXFLAGS) -std=c++17 -Wno-missing-field-initializers -Wno-unused-parameter
LDFLAGS := $(LDFLAGS) $(shell pkg-config --libs-only-L mozjs-78 libenet)
LDLIBS := $(LDLIBS) $(shell pkg-config --libs-only-l mozjs-78 libenet) -lstdc++


BUILTIN_SOURCES := $(wildcard sources/builtin/*.cpp)
BUILTIN_OBJECTS := $(patsubst %.cpp,%.o, $(BUILTIN_SOURCES))

BINDING_SOURCES := $(wildcard sources/binding/*.cpp)
BINDING_OBJECTS := $(patsubst %.cpp,%.o, $(BINDING_SOURCES))

SOURCES := $(BUILTIN_SOURCES) $(BINDING_SOURCES)
OBJECTS := main.o $(BUILTIN_OBJECTS) $(BINDING_OBJECTS)

main: $(OBJECTS)

clean:
	rm $(OBJECTS)

debug:
	$(foreach source,$(SOURCES), $(info source $(source)))
	$(foreach object,$(OBJECTS), $(info object $(object)))
	@echo
	@echo "compiler flags"
	@echo $(CPPFLAGS)
	@echo
	@echo "linker flags"
	@echo $(LDFLAGS)
	@echo
	@echo "linker libraries"
	@echo $(LDLIBS)
