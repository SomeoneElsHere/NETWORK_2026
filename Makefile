# -- Configuration options --
# ---------------------------
SRC_DIR = src/
COPY_DIR = copy/
BUILD_DIR = build/
CPP_VERSION = 17
COMMON_FLAGS = -g -std=c++$(CPP_VERSION)

PEER_EXE = $(BUILD_DIR)peer
TRACKER_EXE = $(BUILD_DIR)tracker
COPY_FILES = $(patsubst $(COPY_DIR)%,$(BUILD_DIR)%,$(wildcard $(COPY_DIR)*))

COMMON_HEADERS = $(SRC_DIR)common.hpp $(SRC_DIR)global_includes.hpp

EXE_NAMES = $(PEER_EXE) $(TRACKER_EXE)

# -- Command formatting --
# ------------------------

# Command format prefixes
COMPILE = g++ $(COMMON_FLAGS) -c
LINK = g++ $(COMMON_FLAGS) -o
LIBS = #-lmd #TODO: check this with TA/professor, look at lib/util.hpp for more info

# -- Main compile target --
# -------------------------

# Ensure all executables are built and included files are copied when running "make"
all: $(EXE_NAMES) $(COPY_FILES)

# Create build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Copy all included files to build directory
$(BUILD_DIR)%: $(COPY_DIR)% | $(BUILD_DIR)
	cp $< $@

# -- Common code --
# -----------------
COMMON_OBJS = $(BUILD_DIR)common.out

# common.out
$(BUILD_DIR)common.out: $(SRC_DIR)common.cpp $(COMMON_HEADERS) | $(BUILD_DIR)
	$(COMPILE) $(SRC_DIR)common.cpp -o $(BUILD_DIR)common.out

# -- Peer program --
# ------------------
PEER_OBJS = $(BUILD_DIR)peer.out $(COMMON_OBJS)

# Main executable
$(PEER_EXE): $(PEER_OBJS) | $(BUILD_DIR)clientThreadConfig.cfg
	$(LINK) $(PEER_EXE) $(PEER_OBJS) $(LIBS)

# peer.out
$(BUILD_DIR)peer.out: $(SRC_DIR)peer.cpp $(COMMON_HEADERS) | $(BUILD_DIR)
	$(COMPILE) $(SRC_DIR)peer.cpp -o $(BUILD_DIR)peer.out

# -- Tracker program --
# ---------------------
TRACKER_OBJS = $(BUILD_DIR)tracker.out $(COMMON_OBJS)

# Main executable
$(TRACKER_EXE): $(TRACKER_OBJS) | $(BUILD_DIR)sconfig
	$(LINK) $(TRACKER_EXE) $(TRACKER_OBJS) $(LIBS)

# tracker.out
$(BUILD_DIR)tracker.out: $(SRC_DIR)tracker.cpp $(COMMON_HEADERS) | $(BUILD_DIR)
	$(COMPILE) $(SRC_DIR)tracker.cpp -o $(BUILD_DIR)tracker.out

# -- Additional Actions --
# ------------------------

# Run the tracker server
.PHONY: run_tracker
run_tracker: $(TRACKER_EXE) $(BUILD_DIR)sconfig
	cd ./build && ./tracker

# Run the peer
.PHONY: run_peer
run_peer: $(PEER_EXE) $(BUILD_DIR)clientThreadConfig.cfg
	cd ./build && ./peer

# Clean the build folder
.PHONY: clean
clean:
	rm -f $(BUILD_DIR)*.out
	rm -f $(EXE_NAMES)
	rm -f $(COPY_FILES)
	rmdir $(BUILD_DIR)