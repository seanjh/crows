.PHONY: all build upload uploadfs flahs monitor clean

all: build

build:
	pio run

upload:
	pio run -t upload

uploadfs:
	pio run -t uploadfs

flash: upload uploadfs

monitor:
	pio device monitor -b 115200

clean:
	pio run -t clean
