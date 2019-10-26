# Project properties
PROGRAM = app

#source options
SRCDIR =
C_FILES := $(wildcard $(SRCDIR)/*.c)

#build options
BUILDDIR = build
OBJS := application.c datalink.c packet.c

#compiler options
CFLAGS = -Wall -pedantic
LDFLAGS = -lpthread
LDLIBS = -D_REENTRANT

all: $(PROGRAM) 

$(PROGRAM): .depend $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $(PROGRAM) $(LDLIBS)

# Dependency management

depend: .depend

.depend: cmd = gcc -MM -MF depend $(var); echo -n "$(BUILDDIR)/" >> .depend; cat depend >> .depend;
.depend:
	@echo "Generating dependencies..."
	@$(foreach var, $(C_FILES), $(cmd))
	@rm -f depend

-include .depend

$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

%: %.c
	$(CC) $(CFLAGS) -o $@ $<


clean:
	rm -f .depend $(PROGRAM)
	rm -rf $(BUILDDIR)

$(BUILDDIR):
	mkdir $(BUILDDIR)

.PHONY: clean depend
