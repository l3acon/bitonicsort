# OpenCL FingerPrints
# EB May 2011

# archive data
CURRENT_DATE := $(shell date +%Y%m%d_%Hh%M)
CURRENT_DIR := $(notdir $(shell /bin/pwd))
CURRENT_MACHINE := $(shell uname -m)

# output directories
DEPDIR := deps
OBJDIR := objs
BINDIR := bin

# targets
PROGS = t_sort
BINS = $(patsubst %,bin/%,$(PROGS))
OBJS = $(patsubst %,$(OBJDIR)/%.o,$(PROGS) GeneratedOpenCLKernelSource TestFunctions) $(LIBOBJS)
DEPS = $(patsubst $(OBJDIR)/%.o, $(DEPDIR)/%.d, $(OBJS))

# flags
CXXFLAGS = -DLinux -g -mtune=nocona -Wall -I src -I /opt/AMDAPP/include -I MiniCL/src
LDFLAGS = -lOpenCL

all: $(BINS)

src/GeneratedOpenCLKernelSource.cpp: src/SortKernels.cl MiniCL/file_to_string.py
	MiniCL/file_to_string.py -i src/SortKernels.cl -o "$@" -s OpenCLKernelSource

bin/t_sort: $(OBJS)
	c++ -o $@ $(OBJDIR)/t_sort.o $(OBJDIR)/TestFunctions.o $(OBJDIR)/GeneratedOpenCLKernelSource.o $(LDFLAGS)

dos2unix:
	dos2unix Makefile *.cpp *.h *.cl *.py

clean:
	find . \( -name "*.o" -o -name "*~" \) -exec /bin/rm {} \;
	find . \( -name "*.cpp" -o -name "*.h" -o -name "Makefile" \) -exec chmod 644 {} \;
	find . \( -name "*.py" \) -exec chmod 755 {} \;
	/bin/rm -rf $(DEPDIR) $(OBJDIR) $(BINDIR) src/GeneratedOpenCLKernelSource.cpp

archive: clean
	@echo "ARCHIVE $(CURRENT_DATE)"
	tar czf "../OpenCLfingerprints-$(CURRENT_DATE).tar.gz" -C.. --exclude=".svn" $(CURRENT_DIR)

# Dependencies
$(DEPDIR)/%.d: src/%.cpp
	@[ -d $(DEPDIR) ] || mkdir -p $(DEPDIR)
	@[ -d $(BINDIR) ] || mkdir -p $(BINDIR)
	@/bin/echo -e "DEPS \033[32m$*\033[0m"
	@$(CXX) $(CXXFLAGS) -o $@ -MM -MT '$(OBJDIR)/$*.o $@' $<
$(DEPDIR)/%.d: generated/%.cpp
	@[ -d $(DEPDIR) ] || mkdir -p $(DEPDIR)
	@[ -d $(BINDIR) ] || mkdir -p $(BINDIR)
	@/bin/echo -e "DEPS \033[32m$*\033[0m"
	@$(CXX) $(CXXFLAGS) -o $@ -MM -MT '$(OBJDIR)/$*.o $@' $<

# Compilation
$(OBJDIR)/%.o: src/%.cpp
	@[ -d $(OBJDIR) ] || mkdir -p $(OBJDIR)
	@/bin/echo -e "C++  \033[34m$*\033[0m"
	@$(CXX) $(CXXFLAGS) -c -o $@ $<
$(OBJDIR)/%.o: generated/%.cpp
	@[ -d $(OBJDIR) ] || mkdir -p $(OBJDIR)
	@/bin/echo -e "C++  \033[34m$*\033[0m"
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

-include $(DEPS)
