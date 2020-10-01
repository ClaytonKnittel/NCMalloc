include ../common.mk

ARDIR=lib
ARNAME=liblib.a

SDIR=.
ODIR=.obj

SRC=$(shell find $(SDIR) -type f -name '*.cc')
OBJ=$(patsubst $(SDIR)/%.cc,$(ODIR)/%.o,$(SRC))
DEP=$(wildcard $(SDIR)/*.h)

DIRS=$(shell find $(SDIR) -type d)
OBJDIRS=$(patsubst $(SDIR)/%,$(ODIR)/%,$(DIRS))

$(shell mkdir -p $(ODIR))
$(shell mkdir -p $(OBJDIRS))

DEPFILES=$(SRC:$(SDIR)/%.cc=$(ODIR)/%.d)

$(shell mkdir -p $(ARDIR))

.SECONDARY:

.PHONY: all
all: archive

archive: $(OBJ)
	$(AR) -rcs $(ARDIR)/$(ARNAME) $(OBJ)

$(ODIR)/%.o: $(SDIR)/%.cc
	$(CC) $(CFLAGS) $< -c -o $@ $(IFLAGS) -I$(BASE_DIR)

$(BDIR)/%: $(ODIR)/$(TEST_SRC_DIR)/%.o
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(ODIR)/$(TEST_SRC_DIR)/%.o: $(TEST_SRC_DIR)/%.cc
	$(CC) $(CFLAGS) $< -c -o $@ $(IFLAGS)

-include $(wildcard $(DEPFILES))

.PHONY: clean
clean:
	find $(ODIR) -type f -name '*.[od]' -delete
	rm -rf $(ODIR) $(ARDIR)
