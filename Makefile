SKETCH_DIR = crow
SKETCH = $(SKETCH_DIR)/crow.ino
BAUDRATE = 921600
BUILD_DIR = build
TARGET = $(BUILD_DIR)/$(notdir $(SKETCH)).bin

compile: clean
	@cd $(SKETCH_DIR)
	arduino-cli compile --profile crow --output-dir $(BUILD_DIR) $(SKETCH)

upload: compile
	@cd $(SKETCH_DIR)
	arduino-cli upload --profile crow --input-dir $(BUILD_DIR) $(SKETCH)

clean:
	rm -rf $(BUILD_DIR)

monitor:
	@cd $(SKETCH_DIR)
	arduino-cli monitor --profile crow $(SKETCH)

.PHONY: compile upload clean monitor
