# -- Configuration options --
# ---------------------------
SRC_DIR = src/
BUILD_DIR = build/
CPP_VERSION = 11
COMMON_FLAGS = -g -std=c++$(CPP_VERSION)

PEER_EXE = $(BUILD_DIR)peer
TRACKER_EXE = $(BUILD_DIR)tracker

EXE_FILES = $(PEER_EXE) $(TRACKER_EXE)

# -- Command formatting --
# ------------------------

# Command format prefixes
COMPILE = g++ $(COMMON_FLAGS) -c
LINK = g++ $(COMMON_FLAGS) -o

# -- Main compile target --
# -------------------------

# Ensure all executables are built when running "make"
all: $(EXE_FILES)

$(BUILD_DIR): 
	mkdir -p $(BUILD_DIR)

# -- Peer program --
# ------------------
PEER_OBJS = $(BUILD_DIR)peer.out

# Main executable
$(PEER_EXE): $(PEER_OBJS) | $(BUILD_DIR)
	$(LINK) $(PEER_EXE) $(PEER_OBJS)

# peer.out
$(BUILD_DIR)peer.out: $(SRC_DIR)peer.cpp | $(BUILD_DIR)
	$(COMPILE) $(SRC_DIR)peer.cpp -o $(BUILD_DIR)peer.out

# -- Tracker program --
# ---------------------
TRACKER_OBJS = $(BUILD_DIR)tracker.out

# Main executable
$(TRACKER_EXE): $(TRACKER_OBJS) | $(BUILD_DIR)
	$(LINK) $(TRACKER_EXE) $(TRACKER_OBJS)

# tracker.out
$(BUILD_DIR)tracker.out: $(SRC_DIR)tracker.cpp | $(BUILD_DIR)
	$(COMPILE) $(SRC_DIR)tracker.cpp -o $(BUILD_DIR)tracker.out

# -- Additional Actions --
# ------------------------

# Run tracker in background, wait one second, then run peer
.PHONY: run
run: $(EXE_FILES)
	$(TRACKER_EXE) & sleep 1; $(PEER_EXE)

# Clean the build folder
.PHONY: clean
clean:
	rm -f $(BUILD_DIR)*.out $(EXE_FILES)