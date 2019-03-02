LIBNAME := vag-bap

BUILDDIR  := build
STATICLIB := $(BUILDDIR)/lib$(LIBNAME).a

INCLUDEDIR := include
HEADERS    := $(shell find $(INCLUDEDIR)/)

SRCDIR  := src
SOURCES := $(shell find $(SRCDIR)/ -iname "*.c")
OBJS    := $(patsubst %.c, $(BUILDDIR)/%.o, $(SOURCES))

INCLUDES := $(patsubst %, -I%, $(INCLUDEDIR))
CFLAGS   := -Wall -g

TOOLDIR     := tools
TOOLSOURCES := $(shell find $(TOOLDIR)/ -iname "*.c")
TOOLEXES    := $(patsubst %.c, $(BUILDDIR)/%, $(TOOLSOURCES))
TOOLLIBS    := -lncurses





.PHONY: tools
tools: $(TOOLEXES)


$(BUILDDIR)/$(TOOLDIR)/%: $(TOOLDIR)/%.c $(HEADERS) $(STATICLIB)
	@if [ ! -d $(dir $@) ] ; then mkdir -p $(dir $@) ; fi
	$(CC) $(INCLUDES) $(CFLAGS) -o $@ $< $(STATICLIB) $(TOOLLIBS)



.PHONY: lib
lib: $(STATICLIB)

$(STATICLIB): $(OBJS)
	@if [ ! -d $(BUILDDIR) ] ; then echo "Error: Build dir '$(BUILDDIR)' does not exist." ; false ; fi
	ar rcs $@ $^


$(BUILDDIR)/$(SRCDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	@if [ ! -d $(dir $@) ] ; then mkdir -p $(dir $@) ; fi
	$(CC) $(INCLUDES) $(CFLAGS) -c -o $@ $<



.PHONY: clean
clean:
	rm -f $(STATICLIB)
	rm -f $(OBJS)
	rm -f $(TOOLEXES)
	rm -rf $(BUILDDIR)/


.PHONY: distclean
distclean: clean
	find . -xdev -name "*~" -exec rm {} \;
	find . -xdev -name "core" -exec rm {} \;
