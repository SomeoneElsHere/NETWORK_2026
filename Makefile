include MF_global.mk

TRACKER = $(DIR_BUILD)/tracker/tracker
PEER = $(DIR_BUILD)/peer/peer

PEER_FILES = $(patsubst $(wildcard $(DIR_COPY)/peer/*),$(DIR_COPY)/peer/%,$(DIR_BUILD)/peer/%)
TRACKER_FILES = $(patsubst $(wildcard $(DIR_COPY)/tracker/*),$(DIR_COPY)/tracker/%,$(DIR_BUILD)/tracker/%)

# -- Build Targets --
# -------------------
.PHONY: build_all
build_all: build_tracker build_peer
#	Nothing else to do here.

.PHONY: build_tracker
build_tracker: $(TRACKER) $(TRACKER_FILES)
#	Nothing else to do here.

$(TRACKER) $(TRACKER_FILES):
	$(MAKE) -f MF_tracker.mk build

.PHONY: build_peer
build_peer: $(PEER)
#	Nothing else to do here.

$(PEER) $(PEER_FILES):
	$(MAKE) -f MF_peer.mk build

# -- Clean Targets --
# -------------------
.PHONY: clean_all
clean_all: clean_tracker clean_peer
	
.PHONY: clean_tracker
clean_tracker:
	$(MAKE) -f MF_tracker.mk clean
	
.PHONY: clean_peer
clean_peer:
	$(MAKE) -f MF_peer.mk clean

# -- Run Targets --
# -----------------
.PHONY: run_tracker
run_tracker: $(TRACKER)
	cd $(DIR_BUILD)/tracker && ./tracker

.PHONY: run_peer
run_peer: $(PEER)
	cd $(DIR_BUILD)/peer && ./peer