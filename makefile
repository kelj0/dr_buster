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

kelj0:
	g++ -O3 -Werror -Wall -shared -std=c++11 -fPIC `python3 -m pybind11 --includes` -I /usr/include/python3.7 -I . src/main.cpp  -o dr_buster/_core -L. -Wl,-rpath,.

pipajme:
	pip3 install ../dr_buster
