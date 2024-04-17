MAKEFLAGS += --silent

VERSION := v$(shell cat ../version.txt)

RUNTIME_PATH = $$HOME/shoggoth

.PHONY: target-dir clean version

all: package-release

include makefiles/build.mk
include makefiles/tools.mk
include makefiles/run.mk
include makefiles/install.mk
include makefiles/package.mk
