include common.mk

SDIR=src
ODIR=.obj
TEST_SRC_DIR=dep

SRC=$(shell find $(SDIR) -type f -name '*.cc')
OBJ=$(patsubst $(SDIR)/%.cc,$(ODIR)/%.o,$(SRC))
DEP=$(wildcard $(SDIR)/*.h)

DIRS=$(shell find $(SDIR) -type d)
OBJDIRS=$(patsubst $(SDIR)/%,$(ODIR)/%,$(DIRS))

# test files (each source is compiled into own executable)
TEST_SRC=$(shell find $(TEST_SRC_DIR) -type f -name '*.cc')
# obj files
TEST_OBJ=$(patsubst $(TEST_SRC_DIR)/%.cc,$(ODIR)/$(TEST_SRC_DIR)/%.o,$(TEST_SRC))
# executables
TESTS=$(patsubst $(TEST_SRC_DIR)/%.cc,$(BDIR)/%,$(TEST_SRC))

$(shell mkdir -p $(ODIR))
$(shell mkdir -p $(OBJDIRS))
$(shell mkdir -p $(ODIR)/$(TEST_SRC_DIR))
$(shell mkdir -p $(BDIR))

DEPFILES=$(SRC:$(SDIR)/%.cc=$(ODIR)/%.d)

# so make doesn't delete object files for no reason
.PRECIOUS: *.o
.SECONDARY: $(TEST_OBJ)

.PHONY: all
all: util $(TESTS)

.PHONY: util
util:
	(make -C lib/)

$(ODIR)/%.o: $(SDIR)/%.cc
	$(CC) $(CFLAGS) $< -c -o $@ $(IFLAGS)

$(BDIR)/%: $(ODIR)/$(TEST_SRC_DIR)/%.o
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) -llib

$(ODIR)/$(TEST_SRC_DIR)/%.o: $(TEST_SRC_DIR)/%.cc
	$(CC) $(CFLAGS) $< -c -o $@ $(IFLAGS)

-include $(wildcard $(DEPFILES))

.PHONY: clean
clean:
	find $(ODIR) -type f -name '*.[od]' -delete
	rm -rf $(ODIR) $(BDIR)
	(make clean -C lib/)
