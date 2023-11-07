
####
# ==== Feedback Arc Set ====
# - Benjamin Mandl -
# _    OSVU 2023   _
####


LIBS = -lm						# Libraries
CC = gcc						# Compiler

CFLAGS = -Wall -pedantic		# -Wall for warnings
CFLAGS += -std=c99  			# define c stadard and debugging
CFLAGS += -D_DEFAULT_SOURCE -D_BSD_SOURCE
CFLAGS += -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS += -c -g

LFLAGS = -g						# linking flags
TARGET = fb_arc_set

TEST_LIBS = -lcunit # Libraries needed for the test
TEST_TARGET = test

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
SOURCES = $(wildcard *.c)
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@


all: $(OBJECTS)
	echo "Linking..."
	$(CC) $(LFLAGS) $(OBJECTS) $(LIBS) -o $(TARGET)


debug: CFLAGS += -DDEBUG -g
debug:
	@echo "Debugging..."
	make all

.SILENT:
clean:
	echo "Cleaning..."
	-rm -f *.out
	-rm -f *.o
	-rm -f vgcore.*
	-rm -f $(TARGET)
	-rm -rf doc/_output
	-rm -f $(TEST_TARGET)
	-rm -f $(TARGET)_mandl.tar.gz

rebuild: clean all

test: clean --test-pre  --test

--test-pre: 
	echo "Pre Build Test..."
	# adding following test flags to the CFLAGS
	$(eval CFLAGS += -DCTEST) 

# private target, cannot be called from outside, please call test
--test: $(OBJECTS)
	echo "Linking Test..."
	$(CC) $(CFLAGS) $(OBJECTS) $(LIBS) $(TEST_LIBS) -o $(TEST_TARGET)

check:
	echo "Doxygen Version: $(shell doxygen --version)"

format:
	clang-format -i *.c

docs:
	echo "Generating doxygen documentation"
	@doxygen doc/doxy.dox


deploy:
	echo "Creating archive..."
	tar -cvzf "$(TARGET)_mandl.tar.gz" $(SOURCES) $(HEADERS) makefile
	echo "Done."

