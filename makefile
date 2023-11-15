
####
# ==== Feedback Arc Set ====
# - Benjamin Mandl -
# _    OSVU 2023   _
####


LIBS = -lm -lrt				# Libraries
CC = gcc						# Compiler

CFLAGS = -Wall -pedantic		# -Wall for warnings
CFLAGS += -std=c99  			# define c stadard and debugging
CFLAGS += -D_DEFAULT_SOURCE -D_BSD_SOURCE
CFLAGS += -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS += -c -g

LFLAGS = -g -pthread 		# linking flags
TARGET = fb_arc_set

TEST_LIBS = -lcunit # Libraries needed for the test
TEST_TARGET = test

ALL_OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))

# remove the other "main application file" from the sources, so the correct one will get used as the entry point
GEN_OBJS = $(filter-out supervisor.o, $(ALL_OBJECTS))
SUP_OBJS = $(filter-out generator.o, $(ALL_OBJECTS))

SOURCES = $(wildcard *.c)
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

generator: $(GEN_OBJS)
	$(CC) $(LFLAGS) $^ -o $@ $(LIBS) 

supervisor: $(SUP_OBJS)
	$(CC) $(LFLAGS) $^ -o $@ $(LIBS) 

all: generator supervisor


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
	-rm -f supervisor
	-rm -f generator #TODO: make more generic
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
	echo $(shell clang-format --version)

format:
	clang-format -i *.c

docs:
	echo "Generating doxygen documentation"
	@doxygen doc/doxy.dox


deploy:
	echo "Creating archive..."
	tar -cvzf "$(TARGET)_mandl.tar.gz" $(SOURCES) $(HEADERS) makefile
	echo "Done."

