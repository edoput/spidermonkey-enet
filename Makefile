CXX=g++
CPPFLAGS := $(CPPFLAGS) -g $(shell pkg-config --cflags mozjs-78 libenet)
CXXFLAGS := $(CXXFLAGS) -std=c++17 -Wno-missing-field-initializers -Wno-unused-parameter
LDFLAGS := $(LDFLAGS) $(shell pkg-config --libs-only-L mozjs-78 libenet)
LDLIBS := $(LDLIBS) $(shell pkg-config --libs-only-l mozjs-78 libenet)
