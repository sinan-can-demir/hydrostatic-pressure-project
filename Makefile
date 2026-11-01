# Convenience wrapper over the CMake presets (CMakePresets.json stays the source of truth;
# CI runs the same commands). `make help` lists the targets.

.DEFAULT_GOAL := help
.PHONY: help build test clang asan coverage lint format trace clean

help: ## Show this help
	@grep -E '^[a-z-]+:.*## ' $(MAKEFILE_LIST) | awk -F':.*## ' '{printf "  make %-9s %s\n", $$1, $$2}'

build: ## Configure and build (preset default)
	cmake --preset default
	cmake --build --preset default -j

test: build ## Build and run all tests
	ctest --preset default -j

clang: ## Build and test with Clang
	cmake --preset clang
	cmake --build --preset clang -j
	ctest --preset clang -j

asan: ## Build and test with AddressSanitizer and UBSan
	cmake --preset asan-ubsan
	cmake --build --preset asan-ubsan -j
	ctest --preset asan-ubsan -j

coverage: ## Build, test and print the line-coverage report (advisory)
	cmake --preset coverage
	cmake --build --preset coverage -j
	ctest --preset coverage -j
	python3 scripts/coverage.py build-coverage

lint: ## Check formatting and run clang-tidy (same as the CI job)
	scripts/lint.sh

format: ## Apply clang-format to all sources
	git ls-files '*.cpp' '*.hpp' | xargs clang-format -i

trace: ## Check that every Must requirement is named in a test
	python3 scripts/traceability.py

clean: ## Remove all build directories
	rm -rf build build-clang build-asan build-coverage build-lint
