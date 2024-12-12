# Makefile

# Compiler and Flags
CXX = clang++
CXXFLAGS = -std=c++17 -stdlib=libc++ \
           -I. -Iimgui -Ibackends -Imodules \
           -I/opt/homebrew/include/SDL2 \
           -I/opt/homebrew/Cellar/dbus/1.14.10/include/dbus-1.0 \
           -I/opt/homebrew/Cellar/dbus/1.14.10/lib/dbus-1.0/include \
           -DGL_SILENCE_DEPRECATION

# Linker Flags
LDFLAGS = -L/opt/homebrew/lib -lSDL2 -ldbus-1 -framework OpenGL

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

# Default Target (Build for macOS with NO_DBUS)
all: $(OUTPUT_MAC)

# Build for macOS (with NO_DBUS)
$(OUTPUT_MAC): $(SOURCES)
	$(CXX) $(CXXFLAGS) -DNO_DBUS $(SOURCES) $(LDFLAGS) -o $(OUTPUT_MAC)

# Build for Raspberry Pi (without NO_DBUS)
build_pi: $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) $(LDFLAGS) -o $(OUTPUT_PI)

# Clean Build Artifacts
clean:
	rm -f $(OUTPUT_MAC) $(OUTPUT_PI)

.PHONY: all build_pi clean
