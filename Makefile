PYTHON ?= python3

.PHONY: setup-local setup-local-dry

setup-local:
	$(PYTHON) tools/setup_local_env.py --workspace .

setup-local-dry:
	$(PYTHON) tools/setup_local_env.py --workspace .
	@echo "Generated .env.local and updated .vscode config with local values."
