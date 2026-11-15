# Host-side unit tests for the ADBMS6830 driver (Unity + fff).
#
# Runs on your laptop with normal gcc/clang. No STM32 or BMS hardware needed.
#
#   make            build and run every test suite
#   make known-issues  run tests for the bugs in docs/known_issues.md
#                      (these are EXPECTED to fail until the bugs are fixed)
#   make clean      delete build output
#
# The first run downloads Unity and fff (pinned versions) into vendor/.
# Requires: make, a C compiler, git.

CC        ?= gcc
UNITY_TAG := v2.6.1
FFF_TAG   := v1.1

DRIVER    := ..
VENDOR    := vendor
BUILD     := build
UNITY_DIR := $(VENDOR)/unity
FFF_DIR   := $(VENDOR)/fff

# stubs/ comes FIRST so its empty main.h / stm32g4xx_hal.h / can_management.h
# are used instead of the real STM32 ones.
INCLUDES := -Istubs -Ifakes -Isupport \
            -I$(DRIVER)/lib/inc -I$(DRIVER)/program/inc \
            -I$(UNITY_DIR)/src -I$(FFF_DIR)

# AddressSanitizer + UndefinedBehaviorSanitizer make out-of-bounds writes and
# other silent C bugs crash the test loudly instead of corrupting memory.
# Set SANITIZE= (empty) if your compiler doesn't support them (e.g. MinGW).
SANITIZE ?= -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all
CFLAGS   ?= -std=c11 -g -O0 -Wall -Wextra -Wno-unused-parameter
CFLAGS   += $(SANITIZE) -DUNITY_INCLUDE_DOUBLE
LDFLAGS  += $(SANITIZE) -lm

# Driver code under test (the real files, unmodified) + the fake hardware layer.
DRIVER_SRC := $(DRIVER)/lib/src/adBms6830GenericType.c \
              $(DRIVER)/lib/src/adBms6830ParseCreate.c \
              fakes/mcuWrapper_fake.c \
              $(UNITY_DIR)/src/unity.c

SUITES       := test_pec test_parse_create test_spi_protocol

.PHONY: all test known-issues clean vendor
all: test

vendor: $(UNITY_DIR)/src/unity.c $(FFF_DIR)/fff.h

$(UNITY_DIR)/src/unity.c:
	git clone --quiet --depth 1 --branch $(UNITY_TAG) -c advice.detachedHead=false \
	    https://github.com/ThrowTheSwitch/Unity $(UNITY_DIR)

$(FFF_DIR)/fff.h:
	git clone --quiet --depth 1 --branch $(FFF_TAG) -c advice.detachedHead=false \
	    https://github.com/meekrosoft/fff $(FFF_DIR)

$(BUILD)/%: %.c $(DRIVER_SRC) | vendor
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDES) $< $(DRIVER_SRC) -o $@ $(LDFLAGS)

# Run every suite even if one fails, then fail at the end if any did.
test: $(addprefix $(BUILD)/,$(SUITES))
	@fail=0; for t in $^; do echo; echo "=== $$t ==="; ./$$t || fail=1; done; \
	if [ $$fail -ne 0 ]; then echo; echo "SOME TESTS FAILED"; exit 1; fi; \
	echo; echo "ALL TESTS PASSED"

# Each known-issue test runs in its own process, because an AddressSanitizer
# report ends the program. Always exits 0: these failures are expected.
KNOWN_ISSUE_CASES := rdcvall rdcsall
known-issues: $(BUILD)/test_known_issues
	@for c in $(KNOWN_ISSUE_CASES); do \
	    echo; echo "=== known issue: $$c (expected to FAIL until fixed) ==="; \
	    ASAN_OPTIONS=detect_leaks=0 ./$< $$c 2>&1 | grep -E "PASS|FAIL|ERROR: AddressSanitizer|SUMMARY|#[0-4] .*adBms" ; \
	done; true

clean:
	rm -rf $(BUILD)
