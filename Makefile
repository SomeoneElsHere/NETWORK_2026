# -- Configuration options --
# ---------------------------
SRC_DIR = src/
BUILD_DIR = build/
CPP_VERSION = 17
COMMON_FLAGS = -g -std=c++$(CPP_VERSION)

PEER_EXE = $(BUILD_DIR)peer
TRACKER_EXE = $(BUILD_DIR)tracker

EXE_NAMES = $(PEER_EXE) $(TRACKER_EXE)

# -- Command formatting --
# ------------------------

# Command format prefixes
COMPILE = g++ $(COMMON_FLAGS) -c
LINK = g++ $(COMMON_FLAGS) -o
LIBS = #-lmd #TODO: check this with TA/professor, look at lib/util.hpp for more info

# -- Main compile target --
# -------------------------

# Ensure all executables are built when running "make"
all: $(EXE_NAMES)

# -- Common code --
# -----------------
COMMON_OBJS = $(BUILD_DIR)common.out

# common.out
$(BUILD_DIR)common.out: $(SRC_DIR)common.cpp
	$(COMPILE) $(SRC_DIR)common.cpp -o $(BUILD_DIR)common.out

# -- Peer program --
# ------------------
PEER_OBJS = $(BUILD_DIR)peer.out $(COMMON_OBJS)

# Main executable
$(PEER_EXE): $(PEER_OBJS)
	$(LINK) $(PEER_EXE) $(PEER_OBJS) $(LIBS)

# peer.out
$(BUILD_DIR)peer.out: $(SRC_DIR)peer.cpp
	$(COMPILE) $(SRC_DIR)peer.cpp -o $(BUILD_DIR)peer.out

# -- Tracker program --
# ---------------------
TRACKER_OBJS = $(BUILD_DIR)tracker.out $(COMMON_OBJS)

# Main executable
$(TRACKER_EXE): $(TRACKER_OBJS)
	$(LINK) $(TRACKER_EXE) $(TRACKER_OBJS) $(LIBS)

# tracker.out
$(BUILD_DIR)tracker.out: $(SRC_DIR)tracker.cpp
	$(COMPILE) $(SRC_DIR)tracker.cpp -o $(BUILD_DIR)tracker.out

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
	rm -f $(BUILD_DIR)$(EXE_NAMES)
