CC = gcc
CFLAGS = -O2 -Wall
INSTALL_DIR = /usr/local/bin
SCRIPT_INSTALL_DIR = /usr/local/share/gt

EXECUTABLE = gts
SCRIPT = gts.sh

.PHONY: all clean install uninstall

all: $(EXECUTABLE)

$(EXECUTABLE): gts.c
	$(CC) $(CFLAGS) -o $@ $<

install: $(EXECUTABLE)
	@echo "Installing gt..."
	@sudo mkdir -p $(INSTALL_DIR)
	@sudo cp $(EXECUTABLE) $(INSTALL_DIR)/$(EXECUTABLE)
	@sudo chmod 755 $(INSTALL_DIR)/$(EXECUTABLE)
	@sudo mkdir -p $(SCRIPT_INSTALL_DIR)
	@sudo cp $(SCRIPT) $(SCRIPT_INSTALL_DIR)/$(SCRIPT)
	@sudo chmod 644 $(SCRIPT_INSTALL_DIR)/$(SCRIPT)
	@echo "Installation complete."
	@echo ""
	@echo "To enable gt completion, add the following line to your .zshrc or .bash_profile:"
	@echo "\n\tsource $(SCRIPT_INSTALL_DIR)/$(SCRIPT)\n"

uninstall:
	@echo "Uninstalling gt..."
	@sudo rm -f $(INSTALL_DIR)/$(EXECUTABLE)
	@sudo rm -f $(SCRIPT_INSTALL_DIR)/$(SCRIPT)
	@sudo rm -rf $(SCRIPT_INSTALL_DIR)
	@echo "Uninstallation complete."

clean:
	rm -f $(EXECUTABLE)