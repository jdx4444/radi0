# Makefile

# Compiler and Flags
CXX = g++
CXXFLAGS_COMMON = -std=c++17 \
                  -I. -Iimgui -Ibackends -Imodules \
                  -DGL_SILENCE_DEPRECATION

CXXFLAGS_MAC = $(CXXFLAGS_COMMON) \
               -stdlib=libc++ \
               -I/opt/homebrew/include/SDL2 \
               -I/opt/homebrew/Cellar/dbus/1.14.10/include/dbus-1.0 \
               -I/opt/homebrew/Cellar/dbus/1.14.10/lib/dbus-1.0/include

CXXFLAGS_PI = $(CXXFLAGS_COMMON) \
              -I/usr/include/SDL2 \
              -I/usr/include/dbus-1.0

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

# Default Target
all: $(OUTPUT_MAC)

# Build for macOS
$(OUTPUT_MAC): $(SOURCES)
	$(CXX) $(CXXFLAGS_MAC) -DNO_DBUS -v $(SOURCES) $(LDFLAGS_MAC) -o $(OUTPUT_MAC)

# Build for Raspberry Pi
build_pi: $(SOURCES)
	$(CXX) $(CXXFLAGS_PI) -DNO_DBUS -v $(SOURCES) $(LDFLAGS_PI) -o $(OUTPUT_PI)

# Clean Build Artifacts
clean:
	rm -f $(OUTPUT_MAC) $(OUTPUT_PI)

.PHONY: all build_pi clean
