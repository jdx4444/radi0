CXX = g++
CXXFLAGS = -std=c++17 -I. -Iimgui -Ibackends -Imodules -I/usr/include/SDL2 -I/usr/include/dbus-1.0 -I/usr/lib/aarch64-linux-gnu/dbus-1.0/include -I/usr/include/taglib -DGL_SILENCE_DEPRECATION
LDFLAGS = -L/usr/lib -lSDL2 -lSDL2_mixer -ldbus-1 -lGL -ltag

SOURCES = main.cpp \
          modules/BluetoothAudioManager.cpp \
          modules/USBAudioManager.cpp \
          modules/Sprite.cpp \
          modules/UI.cpp \
          imgui/imgui.cpp \
          imgui/imgui_draw.cpp \
          imgui/imgui_widgets.cpp \
          imgui/imgui_tables.cpp \
          backends/imgui_impl_sdl2.cpp \
          backends/imgui_impl_opengl3.cpp

OUTPUT = radioBT

all: deps $(OUTPUT)

$(OUTPUT): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) $(LDFLAGS) -o $(OUTPUT)

deps:
	@echo "Checking for required dependencies..."
	@dpkg -s libsdl2-dev libsdl2-mixer-dev libdbus-1-dev libtag1-dev > /dev/null 2>&1 || { \
	    echo "Missing dependencies. Installing..."; \
	    sudo apt update && sudo apt install -y libsdl2-dev libsdl2-mixer-dev libdbus-1-dev libtag1-dev; \
	}

clean:
	rm -f $(OUTPUT)

.PHONY: all clean deps
