CCACHE:=$(shell hash ccache 2>/dev/null || echo "0")
CC = ccache gcc
CXX32 = ccache g++ -m32
CXX64 = ccache g++ -m64
CXX = ccache g++ -m32
CC32 = ccache gcc
ifeq ($(CCACHE),0)
      CC = gcc
      CXX32 = g++ -m32
	  CXX64 = g++ -m64
      CXX = g++ -m32
      CC32 = gcc
endif

export make_basedir = .
export other_dir = ../

CXXFLAG = -g -Wall -std=c++14
LINK_CXXFLAG = $(CXXFLAG) -Wl,-rpath,-Wl,-rpath,$(make_basedir)/bin
INCLUDE := -I./h -I$(make_basedir) -I$(other_dir)

#自定义选项
DBCXXFLAG += -DMETRICS_ON

#业务代码
#src_biz := $(wildcard *.cpp)
src_biz := serialize_test.cpp
other += $(other_dir)/buffer.cpp

SRCS=$(src_biz) #$(other)
OBJ_BIZ = $(patsubst %.c, %.o, $(src_biz:.cpp=.o))
OBJ_OTHER = $(patsubst %.c, %.o, $(other:.cpp=.o))

.SUFFIXES: .o .cpp .c .d

OBJS=$(SRCS:.cpp=.o)

.PHONY: all clean
%.o : %.cpp
	$(CXX64) $(CXXFLAG) $(DBCXXFLAG) $(INCLUDE) -Wno-deprecated -c $< -o $@

.c.o:
	$(CXX64) $(CXXFLAG) -c -o $@ $<

PROGRAM=exeTest

all: $(PROGRAM)

$(PROGRAM): $(OBJ_BIZ) $(OBJ_OTHER)
	$(CXX64) -o $@ $(LINK_CXXFLAG) $^ 
	
clean:
	rm -f *.o $(OBJ_BIZ) $(OBJ_OTHER)
	rm -f *.d
	rm -f $(PROGRAM)
