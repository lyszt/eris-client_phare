SHELL := /bin/bash

BUILD_DIR ?= build
CMAKE     ?= cmake
JOBS      ?= $(shell nproc 2>/dev/null || echo 1)
CONFIG    ?= Debug
BIN_NAME  ?= eris
BIN       := $(BIN_NAME)
PREFIX    ?= /usr/local
DESTDIR   ?=
PPA       ?= ppa:lyszt/eris
SERIES    ?= resolute
GPG_KEY   ?= ADB85BD99D45665D

.PHONY: all configure build run install uninstall deploy clean distclean check-deps deps help

all: build

# ── dependency checks ────────────────────────────────────────────────────────

check-deps:
	@echo "Checking build dependencies..."
	@MISSING=""; \
	command -v cmake  >/dev/null 2>&1 || MISSING="$$MISSING cmake"; \
	command -v gcc    >/dev/null 2>&1 || MISSING="$$MISSING gcc"; \
	command -v g++    >/dev/null 2>&1 || MISSING="$$MISSING g++"; \
	command -v make   >/dev/null 2>&1 || MISSING="$$MISSING make"; \
	command -v pkg-config >/dev/null 2>&1 || MISSING="$$MISSING pkg-config"; \
	pkg-config --exists libcurl 2>/dev/null   || MISSING="$$MISSING libcurl-dev"; \
	pkg-config --exists openssl 2>/dev/null   || MISSING="$$MISSING libssl-dev"; \
	pkg-config --exists libgit2 2>/dev/null   || MISSING="$$MISSING libgit2-dev"; \
	pkg-config --exists readline 2>/dev/null  || MISSING="$$MISSING readline-dev"; \
	pkg-config --exists swipl 2>/dev/null     || MISSING="$$MISSING swi-prolog"; \
	if [ -n "$$MISSING" ]; then \
		echo "Missing:$$MISSING"; \
		echo "Installing..."; \
		if [ ! -f /etc/os-release ]; then \
			echo "Cannot detect OS. Install manually:$$MISSING"; exit 1; \
		fi; \
		. /etc/os-release; \
		case "$$ID" in \
			ubuntu|debian) \
				sudo apt-get update -qq && \
				sudo apt-get install -y build-essential cmake libcurl4-openssl-dev libssl-dev libgit2-dev libreadline-dev swi-prolog pkg-config ;; \
			fedora|rhel|centos) \
				sudo dnf install -y gcc gcc-c++ make cmake libcurl-devel openssl-devel libgit2-devel readline-devel swi-prolog pkgconfig ;; \
			arch|manjaro) \
				sudo pacman -S --needed --noconfirm base-devel cmake curl openssl libgit2 readline swi-prolog pkg-config ;; \
			*) \
				echo "Unsupported OS: $$ID. Install manually:$$MISSING"; exit 1 ;; \
		esac; \
	else \
		echo "All dependencies present."; \
	fi

deps: check-deps
	@mkdir -p vendor
	@if [ ! -f vendor/libgit2-v1.8.5.zip ]; then \
		wget -O vendor/libgit2-v1.8.5.zip https://github.com/libgit2/libgit2/archive/refs/tags/v1.8.5.zip; \
	fi
	@if [ ! -d vendor/libgit2-1.8.5 ]; then \
		unzip -o vendor/libgit2-v1.8.5.zip -d vendor/; \
	fi

# ── build targets ────────────────────────────────────────────────────────────

# libgit2 is resolved by CMake (system lib preferred, vendored source as fallback),
# so the normal build no longer force-downloads it. Run `make deps` to pre-vendor.
configure: check-deps
	@echo "Configuring (BUILD_DIR=$(BUILD_DIR), CONFIG=$(CONFIG))..."
	@$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(CONFIG) $(CMAKE_OPTIONS)

build: configure
	@echo "Building ($(JOBS) jobs) in $(BUILD_DIR)..."
	@$(CMAKE) --build $(BUILD_DIR) -- -j$(JOBS)

run: build
	@echo "Running $(BIN)"
	@./$(BIN)

# ── install ──────────────────────────────────────────────────────────────────

# Eris loads its Prolog router (eris_router.pl) and command modules at runtime,
# relative to the binary. So the binary and its lib/ tree are installed together
# under $(PREFIX)/lib/eris, and $(PREFIX)/bin/eris is a symlink into it — readlink
# of /proc/self/exe resolves the symlink and finds the .pl files alongside.
# Use DESTDIR for staged/package installs. Needs root for system prefixes (sudo).
install: build
	@echo "Installing eris to $(DESTDIR)$(PREFIX) ..."
	install -Dm755 $(BIN) $(DESTDIR)$(PREFIX)/lib/eris/$(BIN)
	install -Dm644 lib/parser/eris_router.pl $(DESTDIR)$(PREFIX)/lib/eris/lib/parser/eris_router.pl
	@find lib/commands -name '*.pl' -exec sh -c 'install -Dm644 "$$1" "$(DESTDIR)$(PREFIX)/lib/eris/$$1"' _ {} \;
	install -d $(DESTDIR)$(PREFIX)/bin
	ln -sf $(PREFIX)/lib/eris/$(BIN) $(DESTDIR)$(PREFIX)/bin/$(BIN)
	@echo "Installed. Make sure $(PREFIX)/bin is on your PATH, then run: eris help"

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(BIN)
	rm -rf $(DESTDIR)$(PREFIX)/lib/eris
	@echo "Uninstalled eris."

# ── deploy (Launchpad PPA → apt install) ─────────────────────────────────────
# Builds a signed source package and uploads it to the PPA (Launchpad builds the
# .deb). Runs in an Ubuntu container so it works from any host; signs with your
# GnuPG key (~/.gnupg). One-time setup: a Launchpad account, your GPG key
# uploaded to it, and that key present locally. Users then:
#   sudo add-apt-repository $(PPA) && sudo apt install eris
deploy:
	@PPA="$(PPA)" SERIES="$(SERIES)" GPG_KEY="$(GPG_KEY)" packaging/deb/deploy.sh

# ── misc ─────────────────────────────────────────────────────────────────────

ctest: build
	@cd $(BUILD_DIR) && ctest --output-on-failure || true

clean:
	@if [ -d $(BUILD_DIR) ]; then $(CMAKE) --build $(BUILD_DIR) --target clean || true; fi
	@if [ -d $(BUILD_DIR) ]; then rm -f $(BUILD_DIR)/CMakeCache.txt; fi
	@$(MAKE) configure

distclean:
	@echo "Removing build directory $(BUILD_DIR)"
	@rm -rf $(BUILD_DIR)

help:
	@echo "Targets:"
	@echo "  all          (default) configure + build"
	@echo "  check-deps   Detect and install missing system deps"
	@echo "  configure    Run cmake configure"
	@echo "  build        Build the project"
	@echo "  run          Build and run eris"
	@echo "  install      Install to /usr/local/bin + add shell alias"
	@echo "  ctest        Run tests"
	@echo "  clean        Clean build artifacts"
	@echo "  distclean    Remove build directory entirely"
	@echo "Vars: BUILD_DIR CONFIG CMAKE JOBS CMAKE_OPTIONS"
