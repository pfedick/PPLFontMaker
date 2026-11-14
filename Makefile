###############################################################################
# This file is part of PPLFontMaker by Patrick Fedick
# Web: https://github.com/pfedick/pplfontmaker
###############################################################################
# Zentrale Build-Steuerung. Bildet CMake in eigenen Build-Ordnern pro
# Build-Variante ab, damit release, debug und asan unabhaengig nebeneinander
# existieren.
#
#   make            -> release + debug
#   make release    -> Release-Build   (build/release)
#   make debug      -> Debug-Build     (build/debug)
#   make asan       -> Sanitizer-Build bauen und das Programm damit starten
#   make run        -> Release-Build starten
#   make run-debug  -> Debug-Build starten
#   make test       -> Tests bauen und ausfuehren
#   make coverage   -> Tests mit Coverage-Auswertung
#   make clean      -> alle Build-Ordnung entfernen
#
# Optionen:
#   -j N            Anzahl paralleler Compilierprozesse (Standard: 8)
#   GENERATOR=Ninja Generator erzwingen (Standard: Ninja, falls vorhanden)
#   APP_ARGS="..."  Argumente fuer das zu startende Programm
#   ASAN_SANITIZERS="address,undefined"
###############################################################################

CMAKE ?= cmake

ifeq ($(OS),Windows_NT)
    EXE = .exe
    ASAN_LEAKS ?= 0    # LeakSanitizer unterstuetzt unter Windows/MSYS kein Detektieren
else
    EXE =
    ASAN_LEAKS ?= 1
endif

# Ninja verwenden, wenn vorhanden; sonst den Standard-Generator von CMake.
GENERATOR ?= $(shell command -v ninja >/dev/null 2>&1 && echo Ninja)
ifneq ($(GENERATOR),)
    GEN_FLAG := -G "$(GENERATOR)"
else
    GEN_FLAG :=
endif

JOBS ?= 8

BUILD_RELEASE := build/release
BUILD_DEBUG := build/debug
BUILD_ASAN := build/asan
BUILD_COVERAGE := build/coverage

APP_RELEASE := $(BUILD_RELEASE)/pplfontmaker$(EXE)
APP_DEBUG := $(BUILD_DEBUG)/pplfontmaker$(EXE)
APP_ASAN := $(BUILD_ASAN)/pplfontmaker$(EXE)

# UBSan braucht -fno-sanitize-recover, damit Undefiniertes Verhalten gemeldet,
# aber nicht abgebrochen wird; ASan meldet dafuer umso genauer.
ASAN_SANITIZERS ?= address,undefined
ASAN_OPTIONS ?= detect_leaks=$(ASAN_LEAKS):halt_on_error=0:max_errors=30:allocator_may_return_null=1
UBSAN_OPTIONS ?= print_stacktrace=1:halt_on_error=0
ASAN_LOG := build/asan/sanitizer.log

.PHONY: all release debug build_asan asan run-asan run run-debug test build_tests coverage clean help

all: release debug

release:
	$(CMAKE) -S . -B $(BUILD_RELEASE) $(GEN_FLAG) -DCMAKE_BUILD_TYPE=Release
	$(CMAKE) --build $(BUILD_RELEASE) -j $(JOBS)
	-ln -sf $(BUILD_RELEASE)/compile_commands.json compile_commands.json
	cp $(APP_RELEASE) .

debug:
	$(CMAKE) -S . -B $(BUILD_DEBUG) $(GEN_FLAG) -DCMAKE_BUILD_TYPE=Debug
	$(CMAKE) --build $(BUILD_DEBUG) -j $(JOBS)
	-ln -sf $(BUILD_DEBUG)/compile_commands.json compile_commands.json
	cp $(APP_DEBUG) .

build_asan:
	$(CMAKE) -S . -B $(BUILD_ASAN) $(GEN_FLAG) -DCMAKE_BUILD_TYPE=Debug -DPPLFONTMAKER_SANITIZER=$(ASAN_SANITIZERS)
	$(CMAKE) --build $(BUILD_ASAN) -j $(JOBS)

# Build starten und die Ausgabe des Programms mitgeschnitten. Die
# Sanitizer-Meldungen stehen im Log, am Ende eine Zusammenfassung.
# SHELL=bash, damit PIPESTATUS den Exit-Code des Programms hinter 'tee' liefert.
asan: build_asan
asan: SHELL := /bin/bash
asan:
	@mkdir -p $(dir $(ASAN_LOG)); \
	ASAN_OPTIONS=$(ASAN_OPTIONS) UBSAN_OPTIONS=$(UBSAN_OPTIONS) ./$(APP_ASAN) $(APP_ARGS) 2>&1 | tee $(ASAN_LOG); \
	rc=$${PIPESTATUS[0]}; \
	sed -i 's/\x1b\[[0-9;]*[A-Za-z]//g' $(ASAN_LOG); \
	found=$$(grep -cE 'ERROR: (Address|Leak)Sanitizer|runtime error:' $(ASAN_LOG) || true); \
	echo ""; \
	echo "=== Sanitizer-Report ($(ASAN_SANITIZERS), Log: $(ASAN_LOG)) ==="; \
	echo "Gefundene Meldungen: $$found    Exit-Code des Programms: $$rc"; \
	grep -E 'ERROR: (Address|Leak)Sanitizer|SUMMARY: (Address|Leak)Sanitizer|runtime error:' $(ASAN_LOG) | sort -u | sed 's|^$(CURDIR)/||' | head -40; \
	[ $$rc -eq 0 ] || exit 1

run-asan: asan

run: release
	./$(APP_RELEASE) $(APP_ARGS)

run-debug: debug
	./$(APP_DEBUG) $(APP_ARGS)

build_tests:
	$(CMAKE) -S . -B $(BUILD_DEBUG) $(GEN_FLAG) -DCMAKE_BUILD_TYPE=Debug -DPPLFONTMAKER_BUILD_TESTS=ON
	$(CMAKE) --build $(BUILD_DEBUG) -j $(JOBS) --target test_all

# Die Tests arbeiten mit Dateien in tests/tmp, deshalb von tests/ aus starten.
test: build_tests
	cd tests && ../$(BUILD_DEBUG)/tests/test_all$(EXE) --gtest_color=yes $(TEST_ARGS)

coverage:
	$(CMAKE) -S . -B $(BUILD_COVERAGE) $(GEN_FLAG) -DCMAKE_BUILD_TYPE=Debug -DPPLFONTMAKER_BUILD_TESTS=ON -DPPLFONTMAKER_ENABLE_COVERAGE=ON
	$(CMAKE) --build $(BUILD_COVERAGE) -j $(JOBS) --target test_all
	-ln -sf $(BUILD_COVERAGE)/compile_commands.json compile_commands.json
	cd tests && ../$(BUILD_COVERAGE)/tests/test_all$(EXE) --gtest_color=yes $(TEST_ARGS)
	mkdir -p coverage_html
	gcovr --root . build/coverage --medium-threshold 70 --source-encoding UTF-8 --exclude-throw-branches \
		--html-details coverage_html/index.html --xml-pretty -o coverage.xml --exclude 'tests/.*' --exclude 'build/coverage/_deps'
	@echo "Report: coverage_html/index.html"

clean:
	-rm -rf build
	-rm -f compile_commands.json coverage.xml
	-rm -rf coverage_html tests/tmp

help:
	@echo "Targets: release debug build_asan asan run run-debug build_tests test coverage clean"
