#
#  MAKEFILE
#
#    Steven Hale
#    2020 February 16
#    Birmingham, UK
#

################################################################################

ARDUINO_CLI_VERSION := 0.28.0
ARDUINO_CLI_URL := https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh
ARDUINO_LINT_VERSION := 1.2.0
ARDUINO_LINT_URL := https://raw.githubusercontent.com/arduino/arduino-lint/main/etc/install.sh

SKETCH := bike-rear-led-strip-light
BOARD := mini
PORT := /dev/ttyACM0

CORES := arduino:avr
LIBRARIES := "Adafruit NeoPixel" "Low-Power"

# The FQBN is the first specified core, followed by the board.
FQBN := $(word 1, $(CORES)):$(BOARD)

# Treat all warnings as errors.
# AVR core 1.8.4 needs -Wno-unused-parameter in new.cpp.
WARNING_FLAGS := "compiler.warning_flags.all=-Wall -Wextra -Werror -Wno-unused-parameter"

# Pass GIT commit hash as version number
GIT_COMMIT := "$(shell git describe --always --dirty --match 'NOT A TAG')"
EXTRA_FLAGS := "compiler.cpp.extra_flags=\"-DGIT_COMMIT=\"$(GIT_COMMIT)\"\""

################################################################################

ROOT := $(PWD)
BINDIR := $(ROOT)/bin
ETCDIR := $(ROOT)/etc
SRCDIR := $(ROOT)/$(SKETCH)
VARDIR := $(ROOT)/var
LOGDIR := $(VARDIR)/log
BUILDDIR := $(VARDIR)/build
CACHEDIR := $(BUILDDIR)/cache

# Build a list of source files for dependency management.
SRCS := $(shell find $(SRCDIR) -name *.ino -or -name *.cpp -or -name *.c -or -name *.h)

# Set the location of the Arduino environment.
export ARDUINO_DATA_DIR = $(VARDIR)

################################################################################

.PHONY: all help env sketch properties lint build upload clean clean-all

all: build

################################################################################

##
# Tell the user what the targets are.
##

help:
	@echo
	@echo "Targets:"
	@echo "   env        Install the Arduino CLI environment."
	@echo "   sketch     Initialise a new sketch."
	@echo "   properties Show all build properties used instead of compiling."
	@echo "   lint       Validate the sketch with arduino-lint."
	@echo "   build      Compile the sketch."
	@echo "   upload     Upload to the board."
	@echo "   clean      Remove only files ignored by Git."
	@echo "   clean-all  Remove all untracked files."
	@echo

################################################################################

# Run in --silent mode unless the user sets VERBOSE=1 on the
# command-line.

ifndef VERBOSE
.SILENT:
endif

# curl|sh really is the documented install method.
# https://arduino.github.io/arduino-cli/latest/installation/
# https://arduino.github.io/arduino-lint/latest/installation/

$(BINDIR)/arduino-cli:
	mkdir -p $(BINDIR) $(ETCDIR)
	curl -fsSL $(ARDUINO_CLI_URL) | BINDIR=$(BINDIR) sh -s $(ARDUINO_CLI_VERSION)

$(BINDIR)/arduino-lint:
	mkdir -p $(BINDIR) $(ETCDIR)
	curl -fsSL $(ARDUINO_LINT_URL) | BINDIR=$(BINDIR) sh -s $(ARDUINO_LINT_VERSION)

$(ETCDIR)/arduino-cli.yaml:
	mkdir -p $(ETCDIR) $(VARDIR)
	$(BINDIR)/arduino-cli config init --verbose
	mv $(VARDIR)/arduino-cli.yaml $(ETCDIR)
	sed -i 's%\(  user:\)\(.*\)%\1 $(VARDIR)%' $(ETCDIR)/arduino-cli.yaml

env: $(BINDIR)/arduino-cli $(BINDIR)/arduino-lint $(ETCDIR)/arduino-cli.yaml
	mkdir -p $(VARDIR) $(LOGDIR) $(BUILDDIR)
	$(BINDIR)/arduino-cli --config-file $(ETCDIR)/arduino-cli.yaml core update-index
	$(BINDIR)/arduino-cli --config-file $(ETCDIR)/arduino-cli.yaml core install $(CORES)
ifdef LIBRARIES
	$(BINDIR)/arduino-cli --config-file $(ETCDIR)/arduino-cli.yaml lib update-index
	$(BINDIR)/arduino-cli --config-file $(ETCDIR)/arduino-cli.yaml lib install $(LIBRARIES)
endif

sketch:
	$(BINDIR)/arduino-cli --config-file $(ETCDIR)/arduino-cli.yaml sketch new $(SRCDIR)

properties:
	$(BINDIR)/arduino-cli --config-file $(ETCDIR)/arduino-cli.yaml compile \
	--build-path $(BUILDDIR) --build-cache-path $(CACHEDIR) \
	--build-property $(WARNING_FLAGS) --build-property $(EXTRA_FLAGS) \
	--warnings all --log-file $(LOGDIR)/build.log --log-level debug --verbose \
	--fqbn $(FQBN) $(SRCDIR) --show-properties

lint:
	$(BINDIR)/arduino-lint $(SRCDIR)

$(BUILDDIR)/$(SKETCH).ino.elf: $(SRCS)
	$(BINDIR)/arduino-cli --config-file $(ETCDIR)/arduino-cli.yaml compile \
	--build-path $(BUILDDIR) --build-cache-path $(CACHEDIR) \
	--build-property $(WARNING_FLAGS) --build-property $(EXTRA_FLAGS) \
	--warnings all --log-file $(LOGDIR)/build.log --log-level debug --verbose \
	--fqbn $(FQBN) $(SRCDIR)

build: $(BUILDDIR)/$(SKETCH).ino.elf
	rm -rf $(SRCDIR)/build

upload: build
	$(BINDIR)/arduino-cli --config-file $(ETCDIR)/arduino-cli.yaml upload \
	--log-file $(LOGDIR)/upload.log --log-level debug --verbose \
	--port $(PORT) --fqbn $(FQBN) --input-file $(BUILDDIR)/$(SKETCH).ino.hex

clean :
	git clean -dXf

clean-all :
	git clean -dxf

################################################################################
