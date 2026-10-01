# -- Configuration options --
# ---------------------------
SRC_DIR = src/
BUILD_DIR = build/
CPP_VERSION = 11
COMMON_FLAGS = -g -std=c++$(CPP_VERSION)

PEER_EXE = $(BUILD_DIR)peer
TRACKER_EXE = $(BUILD_DIR)tracker
THREADTEST_EXE = $(BUILD_DIR)threadtest_main

EXE_NAMES = $(PEER_EXE) $(TRACKER_EXE) $(THREADTEST_EXE)

# -- Command formatting --
# ------------------------

# Command format prefixes
COMPILE = g++ $(COMMON_FLAGS) -c
LINK = g++ $(COMMON_FLAGS) -o

# -- Main compile target --
# -------------------------

# Ensure all executables are built when running "make"
all: $(EXE_NAMES)

# -- Peer program --
# ------------------
PEER_OBJS = $(BUILD_DIR)peer.out

# Main executable
$(PEER_EXE): $(PEER_OBJS)
	$(LINK) $(PEER_EXE) $(PEER_OBJS)

# peer.out
$(BUILD_DIR)peer.out: $(SRC_DIR)peer.cpp
	$(COMPILE) $(SRC_DIR)peer.cpp -o $(BUILD_DIR)peer.out

# -- Tracker program --
# ---------------------
TRACKER_OBJS = $(BUILD_DIR)tracker.out

# Main executable
$(TRACKER_EXE): $(TRACKER_OBJS)
	$(LINK) $(TRACKER_EXE) $(TRACKER_OBJS)

# tracker.out
$(BUILD_DIR)tracker.out: $(SRC_DIR)tracker.cpp
	$(COMPILE) $(SRC_DIR)tracker.cpp -o $(BUILD_DIR)tracker.out

# -- Multithreading test program --
# ---------------------------------
THREADTEST_OBJS = $(BUILD_DIR)threadtest.out $(BUILD_DIR)threadtest_main.out
THREADTEST_HEADERS = $(SRC_DIR)threadtest/threadtest.h

# Main executable
$(THREADTEST_EXE): $(THREADTEST_OBJS)
	$(LINK) $(THREADTEST_EXE) $(THREADTEST_OBJS)

# threadtest.out
$(BUILD_DIR)threadtest.out: $(THREADTEST_HEADERS)
	$(COMPILE) $(SRC_DIR)threadtest/threadtest.cpp -o $(BUILD_DIR)threadtest.out

# threadtest_main.out
$(BUILD_DIR)threadtest_main.out: $(SRC_DIR)threadtest_main.cpp $(THREADTEST_HEADERS)
	$(COMPILE) $(SRC_DIR)threadtest_main.cpp -o $(BUILD_DIR)threadtest_main.out

# -- Additional Actions --
# ------------------------

# Run tracker in background, wait one second, then run peer
.PHONY: run
run: $(EXE_NAMES)
	$(TRACKER_EXE) & sleep 1; $(PEER_EXE)

# Clean the build folder
.PHONY: clean
clean:
	rm -f $(BUILD_DIR)*.out
	rm -f $(EXE_NAMES)