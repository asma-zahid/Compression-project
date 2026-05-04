CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c99
LDFLAGS =

TARGET  = bzip2_impl
SRCDIR  = src
INCDIR  = include
OBJDIR  = obj

SOURCES = $(SRCDIR)/main.c    \
          $(SRCDIR)/rle.c     \
          $(SRCDIR)/bwt.c     \
          $(SRCDIR)/block.c   \
          $(SRCDIR)/mtf.c     \
          $(SRCDIR)/huffman.c \
          $(SRCDIR)/config.c

OBJECTS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))

.PHONY: all clean windows test dirs

all: dirs $(TARGET)

dirs:
	@mkdir -p $(OBJDIR) results benchmarks

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
	@echo ""
	@echo "Build successful -> $(TARGET)"
	@echo "Usage:  ./$(TARGET) compress   <input>  <output.bz>"
	@echo "        ./$(TARGET) decompress <input.bz>  <output>"

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

# ── quick correctness test ────────────────────────────────────────
test: all
	@echo "--- Running self-test ---"
	@# build a few KB of repetitive English-ish text so MTF + RLE-2
	@# actually have something to chew on
	@printf '' > _test_in.txt
	@i=0; while [ $$i -lt 60 ]; do \
		printf 'the quick brown fox jumps over the lazy dog. ' >> _test_in.txt; \
		printf 'pack my box with five dozen liquor jugs. ' >> _test_in.txt; \
		printf 'AAAAAABBBBBBCCCCCC...000111222333... ' >> _test_in.txt; \
		i=$$((i+1)); \
	done
	./$(TARGET) compress   _test_in.txt  _test.bz
	./$(TARGET) decompress _test.bz      _test_out.txt
	@if diff -q _test_in.txt _test_out.txt > /dev/null 2>&1; then \
		echo ""; echo "*** TEST PASSED – roundtrip OK ***"; \
	else \
		echo ""; echo "*** TEST FAILED – files differ ***"; \
	fi
	@rm -f _test_in.txt _test.bz _test_out.txt

# ── cross-compile for Windows (requires mingw-w64) ────────────────
windows:
	$(MAKE) CC=x86_64-w64-mingw32-gcc TARGET=$(TARGET).exe all

clean:
	rm -rf $(OBJDIR) $(TARGET) $(TARGET).exe
