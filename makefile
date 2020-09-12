.DEFAULT_GOAL := build
.PHONY: install-required clean

install-required:
	sudo python -m pip install --upgrade pip setuptools wheel

build:
	@ echo building the module...
	python3 setup.py bdist_wheel

clean:
	-rm -rf dr_buster.egg-info
	-rm -rf build/
	-rm -rf dist/
	-rm -rf _build/
	-rm -rf _generate/
	-rm *.so
	-rm *.py[cod]
	-rm *.egg-info