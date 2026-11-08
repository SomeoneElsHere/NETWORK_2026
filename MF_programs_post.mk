# == Include this file after setting all options ==

# -- Command formatting --
# ------------------------
COMPILE = g++ $(COMMON_FLAGS) $(COMPILE_FLAGS) -c
LINK = g++ $(COMMON_FLAGS) $(LINK_FLAGS) -o

# -- Common build targets --
# --------------------------
$(DIR_BUILD):
	mkdir -p $(DIR_BUILD)

# Compile (without linking) each .cpp file
$(DIR_BUILD)/%.out: $(DIR_SRC)/%.cpp $(DIR_SRC)/%.hpp | $(DIR_BUILD)
	$(COMPILE) $< -o $@

# -- Program-specific directories/names --
# ----------------------------------
DIR_SRC_PROGRAM = $(DIR_SRC)/$(NAME)
DIR_COPY_PROGRAM = $(DIR_COPY)/$(NAME)
DIR_BUILD_PROGRAM = $(DIR_BUILD)/$(NAME)

EXECUTABLE = $(DIR_BUILD_PROGRAM)/$(NAME)

# -- Required .out files and extra, copied files --
# -------------------------------------------------
REQ_OBJS := $(wildcard $(DIR_SRC_PROGRAM)/*.cpp) $(wildcard $(DIR_SRC)/*.cpp)
REQ_OBJS := $(filter-out $(DIR_SRC_PROGRAM)/main.cpp, $(REQ_OBJS))
REQ_OBJS := $(REQ_OBJS:$(DIR_SRC)/%.cpp=$(DIR_BUILD)/%.out) $(DIR_BUILD_PROGRAM)/main.out

REQ_COPY := $(wildcard $(DIR_COPY_PROGRAM)/*)
REQ_COPY := $(REQ_COPY:$(DIR_COPY_PROGRAM)/%=$(DIR_BUILD_PROGRAM)/%)

# -- Executable build target --
# -----------------------------
.PHONY: build
build: $(EXECUTABLE)

# Build executable
$(EXECUTABLE): $(REQ_OBJS) | $(REQ_COPY) $(DIR_BUILD_PROGRAM)
	$(LINK) $@ $^

# Compile main.cpp file without linking; main does not require a .hpp file because it is only supposed
# to contain a main() method and any utility definitions/declarations that are used only by it.
$(DIR_BUILD_PROGRAM)/main.out: $(DIR_SRC_PROGRAM)/main.cpp | $(DIR_BUILD_PROGRAM)
	$(COMPILE) $< -o $@

# Compile (without linking) every other .cpp file, ensuring their respective .hpp file exists
$(DIR_BUILD_PROGRAM)/%.out: $(DIR_SRC_PROGRAM)/%.cpp $(DIR_SRC_PROGRAM)/%.hpp | $(DIR_BUILD_PROGRAM)
	$(COMPILE) $< -o $@

# Copy extra files to build directory
$(DIR_BUILD_PROGRAM)/%: $(DIR_COPY_PROGRAM)/% | $(DIR_BUILD_PROGRAM)
	cp $< $@

# Ensure build directory exists
$(DIR_BUILD_PROGRAM): 
	mkdir -p $(DIR_BUILD_PROGRAM)

# -- Clean target --
# ------------------
.PHONY: clean
clean:
	rm -f $(EXECUTABLE) $(REQ_OBJS) $(REQ_COPY)
	rmdir $(DIR_BUILD_PROGRAM)