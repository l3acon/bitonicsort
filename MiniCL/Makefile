# MiniCL
# EB May 2011

# archive data
CURRENT_DATE := $(shell date +%Y%m%d_%Hh%M)
CURRENT_DIR := $(notdir $(shell /bin/pwd))
CURRENT_MACHINE := $(shell uname -m)

# output directories
DEPDIR := deps
OBJDIR := objs

# targets
PROGS = t_minicl
OBJS = $(patsubst %,$(OBJDIR)/%.o,$(PROGS) TestFunctions ) $(LIBOBJS)
DEPS = $(patsubst $(OBJDIR)/%.o, $(DEPDIR)/%.d, $(OBJS))

# flags
CXXFLAGS = -DLinux -g -mtune=nocona -Wall -I src -I $(CUDA_INSTALL_PATH)/include
LDFLAGS = -lOpenCL

all: $(PROGS)

t_minicl: $(OBJS) $(LIB)
	c++ -o $@ $(OBJDIR)/t_minicl.o $(OBJDIR)/TestFunctions.o $(LDFLAGS)

dos2unix:
	dos2unix Makefile *.cpp *.h *.cl *.py

clean:
	find . \( -name "*.o" -o -name "*~" \) -exec /bin/rm {} \;
	find . \( -name "*.cpp" -o -name "*.h" -o -name "Makefile" \) -exec chmod 644 {} \;
	find . \( -name "*.py" \) -exec chmod 755 {} \;
	/bin/rm -rf $(DEPDIR) $(OBJDIR)
	/bin/rm -f $(PROGS)

archive: clean
	@echo "ARCHIVE $(CURRENT_DATE)"
	tar czf "../MiniCL-$(CURRENT_DATE).tar.gz" -C.. --exclude=".svn" $(CURRENT_DIR)

# Dependencies
$(DEPDIR)/%.d: src/%.cpp
	@[ -d $(DEPDIR) ] || mkdir -p $(DEPDIR)
	@/bin/echo -e "DEPS \033[32m$*\033[0m"
	@$(CXX) $(CXXFLAGS) -o $@ -MM -MT '$(OBJDIR)/$*.o $@' $<
$(DEPDIR)/%.d: test/%.cpp
	@[ -d $(DEPDIR) ] || mkdir -p $(DEPDIR)
	@/bin/echo -e "DEPS \033[32m$*\033[0m"
	@$(CXX) $(CXXFLAGS) -o $@ -MM -MT '$(OBJDIR)/$*.o $@' $<

# Compilation
$(OBJDIR)/%.o: src/%.cpp
	@[ -d $(OBJDIR) ] || mkdir -p $(OBJDIR)
	@/bin/echo -e "C++  \033[34m$*\033[0m"
	@$(CXX) $(CXXFLAGS) -c -o $@ $<
$(OBJDIR)/%.o: test/%.cpp
	@[ -d $(OBJDIR) ] || mkdir -p $(OBJDIR)
	@/bin/echo -e "C++  \033[34m$*\033[0m"
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

-include $(DEPS)
