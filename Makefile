# Makefile

# Compiler and Flags
CXX = g++
CXXFLAGS_COMMON = -std=c++17 -I. -Iimgui -Ibackends -Imodules -DGL_SILENCE_DEPRECATION
CXXFLAGS_MAC = $(CXXFLAGS_COMMON) -stdlib=libc++ -I/opt/homebrew/include/SDL2 -I/opt/homebrew/Cellar/dbus/1.14.10/include/dbus-1.0 -I/opt/homebrew/Cellar/dbus/1.14.10/lib/dbus-1.0/include
# Update for Raspberry Pi: add the correct DBus include directory for ARM64
CXXFLAGS_PI = $(CXXFLAGS_COMMON) -I/usr/include/SDL2 -I/usr/include/dbus-1.0 -I/usr/lib/aarch64-linux-gnu/dbus-1.0/include
LDFLAGS_MAC = -L/opt/homebrew/lib -lSDL2 -ldbus-1 -framework OpenGL
LDFLAGS_PI = -L/usr/lib -lSDL2 -ldbus-1 -lGL

# Source Files
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

# Output Binaries
OUTPUT_MAC = radioBT_mac
OUTPUT_PI = radioBT_pi
OUTPUT_DBUS_MAC = radioBT_mac_dbus
OUTPUT_DBUS_PI = radioBT_pi_dbus

# Default Target (macOS mock version)
all: $(OUTPUT_MAC)

# Build for macOS (mock version: with -DNO_DBUS)
$(OUTPUT_MAC): $(SOURCES)
	$(CXX) $(CXXFLAGS_MAC) -DNO_DBUS -v $(SOURCES) $(LDFLAGS_MAC) -o $(OUTPUT_MAC)

# Build for macOS with DBus enabled
build_dbus_mac: $(SOURCES)
	$(CXX) $(CXXFLAGS_MAC) -v $(SOURCES) $(LDFLAGS_MAC) -o $(OUTPUT_DBUS_MAC)

# Build for Raspberry Pi (mock version)
build_pi: $(SOURCES)
	$(CXX) $(CXXFLAGS_PI) -DNO_DBUS -v $(SOURCES) $(LDFLAGS_PI) -o $(OUTPUT_PI)

# Build for Raspberry Pi with DBus enabled
build_dbus_pi: $(SOURCES)
	$(CXX) $(CXXFLAGS_PI) -v $(SOURCES) $(LDFLAGS_PI) -o $(OUTPUT_DBUS_PI)

# Clean Build Artifacts
clean:
	rm -f $(OUTPUT_MAC) $(OUTPUT_PI) $(OUTPUT_DBUS_MAC) $(OUTPUT_DBUS_PI)

.PHONY: all build_pi build_dbus_mac build_dbus_pi clean
