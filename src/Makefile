# Project properties
PROGRAM = app

#source options
SRCDIR =
C_FILES := $(wildcard $(SRCDIR)/*.c)

#build options
BUILDDIR = build
OBJS := application.c datalink.c packet.c

all: $(PROGRAM)

$(PROGRAM): .depend $(OBJS)
	$(CC) $(OBJS) -o $(PROGRAM)

# Dependency management

depend: .depend

.depend: cmd = gcc -MM -MF depend $(var); echo -n "$(BUILDDIR)/" >> .depend; cat depend >> .depend;
.depend:
	@echo "Generating dependencies..."
	@$(foreach var, $(C_FILES), $(cmd))
	@rm -f depend

-include .depend

$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(BUILDDIR)
	$(CC) -c $< -o $@

%: %.c
	$(CC) -o $@ $<


clean:
	rm -f .depend $(PROGRAM)
	rm -rf $(BUILDDIR)

$(BUILDDIR):
	mkdir $(BUILDDIR)

.PHONY: clean depend
