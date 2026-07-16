.DEFAULT_GOAL := help
PROJECT := tesla-ble
TARGET ?= tesla-ble.local.yml
HOST_SUFFIX := ""

compile: .esphome/build/$(PROJECT)/build/firmware.ota.bin ## Read the configuration and compile the binary.

.esphome/build/$(PROJECT)/$(TARGET).touchfile: .venv/touchfile $(TARGET) packages/**/*.yml ## Validate the configuration and create a binary.
	. .venv/bin/activate; esphome compile $(TARGET)
	mkdir -p .esphome/build/$(PROJECT)
	touch .esphome/build/$(PROJECT)/$(TARGET).touchfile

.esphome/build/$(PROJECT)/build/firmware.ota.bin: .esphome/build/$(PROJECT)/$(TARGET).touchfile ## Create the binary.

upload: .esphome/build/$(PROJECT)/build/firmware.ota.bin ## Validate the configuration, create a binary, upload it, and start logs.
	if [ "$(HOST_SUFFIX)" = "" ]; then \
		. .venv/bin/activate; esphome run $(TARGET); \
	else \
		. .venv/bin/activate; esphome run $(TARGET) --device $(PROJECT)$(HOST_SUFFIX); \
	fi

logs: .venv/touchfile
	if [ "$(HOST_SUFFIX)" = "" ]; then \
		. .venv/bin/activate; esphome logs $(TARGET); \
	else \
		. .venv/bin/activate; esphome logs $(TARGET) --device $(PROJECT)$(HOST_SUFFIX); \
	fi

config: .venv/touchfile ## Validate YAML only (no full compile).
	. .venv/bin/activate; esphome config $(TARGET)

deps: .venv/touchfile ## Create the virtual environment and install the requirements.

.venv/touchfile: requirements.txt
	test -d .venv || python -m venv .venv
	. .venv/bin/activate && pip install -Ur requirements.txt
	touch .venv/touchfile

.PHONY: clean test
clean: ## Remove the virtual environment and the esphome build directory
	rm -rf .venv
	rm -rf .esphome

test: ## Run structural package / branding tests
	python3 tests/test_package_layout.py

.PHONY: help
help: ## Show help messages for make targets
	@grep -E '^[a-zA-\/Z_-]+:.*?## .*$$' $(firstword $(MAKEFILE_LIST)) \
	| sort \
	| awk 'BEGIN {FS = ":.*?## "}; {printf "\033[32m%-30s\033[0m %s\n", $$1, $$2}'

compile_docker: ## Compile the binary using docker
	docker run --rm -v $(PWD):/config ghcr.io/esphome/esphome compile /config/$(TARGET)
