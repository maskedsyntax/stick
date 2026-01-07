CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 $(shell pkg-config --cflags gtkmm-3.0)
LDFLAGS = $(shell pkg-config --libs gtkmm-3.0)

SRC_DIR = src
OBJ_DIR = build
TARGET = $(OBJ_DIR)/stick

SRCS = $(wildcard $(SRC_DIR)/*.cc)
OBJS = $(patsubst $(SRC_DIR)/%.cc, $(OBJ_DIR)/%.o, $(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cc | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin
DESKTOPDIR = $(PREFIX)/share/applications
ICONDIR = $(PREFIX)/share/icons/hicolor/512x512/apps

install: all
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/stick
	install -d $(DESTDIR)$(DESKTOPDIR)
	install -m 644 stick.desktop $(DESTDIR)$(DESKTOPDIR)/stick.desktop
	install -d $(DESTDIR)$(ICONDIR)
	install -m 644 assets/stick.png $(DESTDIR)$(ICONDIR)/stick.png

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/stick
	rm -f $(DESTDIR)$(DESKTOPDIR)/stick.desktop
	rm -f $(DESTDIR)$(ICONDIR)/stick.png

.PHONY: all clean install uninstall
