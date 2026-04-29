all: build

build:
	$(MAKE) -C src all

run:
	$(MAKE) -C src run

clean:
	$(MAKE) -C src clean

rebuild:
	$(MAKE) -C src clean
	$(MAKE) -C src all
