# add this to prevent removing intermediate files.
.SECONDARY:

# Default target
all: doc test

# Config
include scripts/config.mk

# Rules for search target
include scripts/source.mk

# Rules for environment
include scripts/envs.mk

# Rules for tool and utilities
include scripts/tools.mk

# Rules for build documentation
include scripts/doc.mk

# Rules for build and unit test
include scripts/test.mk

doc: $(DOCUMENT_TARGET)

test: $(TEST_TARGET_LOGS)
	@echo done!

defconfig: $(BUILD_DIR)
	@-mv -f .config .config.old
	@-rm -f .config
	@genconfig --header-path=$(AUTOCONF_PATH) --config-out=.config

menuconfig: $(BUILD_DIR)
	@menuconfig
	@genconfig --header-path=$(AUTOCONF_PATH)

clean:
	@rm -rf $(BUILD_DIR)

help:
	@echo "Usage: make [target]"
	@echo "  target: menuconfig     - configure the project."
	@echo "  target: defconfig      - Make a .config file and set to default."
	@echo "  target: doc            - generate documentation for this project"
	@echo "  target: clean          - clean all generated files"
	@echo "  target: all            - build all target"
	@echo "  target: help           - display this help message"

.PHONY: test clean defconfig menuconfig
