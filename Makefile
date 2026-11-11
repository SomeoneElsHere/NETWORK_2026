include MF_global.mk

TRACKER := $(DIR_BUILD)/tracker/tracker
PEER := $(DIR_BUILD)/peer/peer
TESTING := $(DIR_BUILD)/testing/testing

PEER_FILES := $(patsubst $(DIR_COPY)/peer/%,$(DIR_BUILD)/peer/%,$(wildcard $(DIR_COPY)/peer/*))
TRACKER_FILES := $(patsubst $(DIR_COPY)/tracker/%,$(DIR_BUILD)/tracker/%,$(wildcard $(DIR_COPY)/tracker/*))

# -- Default build target --
# --------------------------
.PHONY: default
default: $(TRACKER) $(PEER)
	cp $(TRACKER) -t .
	cp $(wildcard $(DIR_COPY)/tracker/*) -t .

	mkdir -p peer1
	mkdir -p peer2
	mkdir -p peer3

	cp $(PEER) -t ./peer1
	cp $(wildcard $(DIR_COPY)/peer/*) -t ./peer1
	cp ./peer1/* -t ./peer2
	cp ./peer1/* -t ./peer3

# -- Default clean target --
# --------------------------
.PHONY: clean
clean:
	rm ./tracker
	rm $(patsubst $(DIR_COPY)/tracker/%,./%,$(wildcard $(DIR_COPY)/tracker/*))

	rm -rf ./peer1
	rm -rf ./peer2
	rm -rf ./peer3

# -- Build Targets --
# -------------------
.PHONY: build_all
build_all: build_tracker build_peer
#	Nothing else to do here.

$(TRACKER): build_tracker

.PHONY: build_tracker
build_tracker:
	$(MAKE) -f MF_tracker.mk build

$(PEER): build_peer

.PHONY: build_peer
build_peer:
	$(MAKE) -f MF_peer.mk build

.PHONY: build_testing
build_testing:
	$(MAKE) -f MF_testing.mk build DEBUG="true"

# -- Build Debug Targets --
# -------------------------
.PHONY: build_all_debug
build_all_debug: build_tracker_debug build_peer_debug
#	Nothing else to do here.

.PHONY: build_tracker_debug
build_tracker_debug:
	$(MAKE) -f MF_tracker.mk build DEBUG="true"

.PHONY: build_peer_debug
build_peer_debug:
	$(MAKE) -f MF_peer.mk build DEBUG="true"

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

.PHONY: clean_testing
clean_testing:
	$(MAKE) -f MF_testing.mk clean

# -- Run Targets --
# -----------------
.PHONY: run_tracker
run_tracker: $(TRACKER)
	cd $(DIR_BUILD)/tracker && ./tracker

.PHONY: run_peer
run_peer: $(PEER)
	cd $(DIR_BUILD)/peer && ./peer

.PHONY: run_testing
run_testing: $(TESTING)
	cd $(DIR_BUILD)/testing && ./testing