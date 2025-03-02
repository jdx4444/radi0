CXX = g++
CXXFLAGS = -std=c++17 -I. -Iimgui -Ibackends -Imodules -DGL_SILENCE_DEPRECATION \
           -I/usr/include/SDL2 -I/usr/include/dbus-1.0 -I/usr/lib/aarch64-linux-gnu/dbus-1.0/include
LDFLAGS = -L/usr/lib -lSDL2 -ldbus-1 -lGL

SOURCES = main.cpp \
          modules/BluetoothAudioManager.cpp \
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
	$(CXX) $(CXXFLAGS) -v $(SOURCES) $(LDFLAGS) -o $(OUTPUT)

deps:
	@echo "Checking for required dependencies..."
	@dpkg -s libsdl2-dev libdbus-1-dev > /dev/null 2>&1 || { \
	    echo "Missing dependencies. Installing..."; \
	    sudo apt update && sudo apt install -y libsdl2-dev libdbus-1-dev; \
	}

clean:
	rm -f $(OUTPUT)

.PHONY: all clean deps