INSTALL_PATH=../build

all:
	python setup.py build

install:
	python setup.py install --install-platlib=$(INSTALL_PATH)

clean:
	python setup.py clean
	rm -rf build

test:
	PYTHONPATH=$(INSTALL_PATH):$(PYTHONPATH) ./run_unittests

.PHONY: all install clean test
