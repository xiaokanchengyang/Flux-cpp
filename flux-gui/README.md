# Flux Archive Manager - GUI Application

A modern, cross-platform archive management application built with Qt6 and C++23.

## Features

### 🎨 Modern UI Design
- **Dark & Light Themes**: Beautiful Material Design 3 inspired themes
- **Responsive Layout**: Adaptive interface that works on different screen sizes
- **Smooth Animations**: Polished transitions and visual feedback
- **Drag & Drop Support**: Intuitive file handling

### 📦 Archive Management
- **Multi-Format Support**: ZIP, 7Z, RAR, TAR, GZ, BZ2, XZ, and more
- **Compression Presets**: Optimized settings for different use cases
- **Batch Operations**: Handle multiple files and archives efficiently
- **Progress Tracking**: Real-time operation progress with cancellation

### ⚙️ Advanced Configuration
- **Comprehensive Settings**: Fine-tune every aspect of the application
- **System Integration**: Context menu integration (Windows/Linux/macOS)
- **Keyboard Shortcuts**: Efficient workflow with customizable hotkeys
- **Recent Files**: Quick access to recently used archives

## Architecture

### Directory Structure
```
flux-gui/
├── src/
│   ├── main.cpp                    # Application entry point
│   ├── ui/                         # User Interface Layer
│   │   ├── main_window.{h,cpp}     # Main application window
│   │   ├── widgets/                # Main UI widgets
│   │   │   ├── welcome_widget.*    # Welcome/home screen
│   │   │   ├── archive_explorer.*  # Archive browsing interface
│   │   │   ├── compression_widget.* # Archive creation interface
│   │   │   ├── extraction_widget.*  # Archive extraction interface
│   │   │   └── settings_widget.*    # Application settings
│   │   ├── components/             # Reusable UI components
│   │   │   ├── modern_toolbar.*    # Modern navigation toolbar
│   │   │   ├── status_bar.*        # Enhanced status bar
│   │   │   ├── file_tree_view.*    # File system tree view
│   │   │   └── archive_info_panel.* # Archive information display
│   │   └── dialogs/                # Modal dialogs
│   │       ├── about_dialog.*      # About application dialog
│   │       └── preferences_dialog.* # Settings dialog
│   ├── core/                       # Core Business Logic
│   │   ├── theme/                  # Theme management system
│   │   │   └── theme_manager.*     # Theme switching and customization
│   │   ├── config/                 # Configuration management
│   │   │   └── settings_manager.*  # Application settings
│   │   └── archive/                # Archive operations
│   │       └── archive_manager.*   # Archive handling logic
│   ├── platform/                   # Platform-specific code
│   │   └── system_integration.*    # OS integration features
│   ├── utils/                      # Utility classes
│   │   ├── file_utils.*           # File system utilities
│   │   ├── resource_manager.*     # Resource management
│   │   └── ui_utils.*             # UI helper functions
│   └── models/                     # Data models
│       ├── archive_model.*         # Archive content model
│       └── file_system_model.*     # File system model
├── resources/                      # Application resources
│   ├── themes/                     # Theme stylesheets
│   │   ├── dark.qss               # Dark theme styles
│   │   └── light.qss              # Light theme styles
│   ├── icons/                      # Application icons
│   ├── images/                     # Images and graphics
│   ├── fonts/                      # Custom fonts
│   ├── translations/               # Internationalization files
│   ├── config/                     # Configuration templates
│   │   ├── default-settings.json  # Default application settings
│   │   ├── mime-types.json        # MIME type definitions
│   │   └── compression-presets.json # Compression presets
│   └── resources.qrc              # Qt resource file
└── CMakeLists.txt                 # Build configuration
```

### Design Patterns

#### 1. **Singleton Pattern**
Core managers (ThemeManager, SettingsManager, ArchiveManager) use singleton pattern for global access.

#### 2. **Observer Pattern**
Settings and theme changes are propagated through Qt's signal-slot mechanism.

#### 3. **Strategy Pattern**
Different compression algorithms and archive formats are handled through pluggable strategies.

#### 4. **Model-View Pattern**
File listings and archive contents use Qt's model-view architecture.

### Key Components

#### Theme System
- **ThemeManager**: Centralized theme management with hot-swapping
- **Material Design 3**: Modern color schemes and typography
- **Custom Properties**: CSS-like styling with dynamic variables
- **Animation Support**: Smooth transitions between themes

#### Settings Management
- **Hierarchical Configuration**: JSON-based settings with nested structure
- **Type Safety**: Strongly-typed setting access with validation
- **Import/Export**: Settings backup and sharing functionality
- **Live Updates**: Real-time settings application without restart

#### Archive Operations
- **Asynchronous Processing**: Non-blocking archive operations
- **Progress Reporting**: Detailed progress with cancellation support
- **Format Detection**: Automatic archive format recognition
- **Error Handling**: Comprehensive error reporting and recovery

## Building

### Prerequisites
- **Qt 6.5+**: Modern Qt framework with C++23 support
- **CMake 3.22+**: Build system generator
- **C++23 Compiler**: MSVC 2022, GCC 12+, or Clang 15+

### Windows (Visual Studio)
```bash
# Configure
cmake -G "Visual Studio 17 2022" -A x64 -B build-gui flux-gui

# Build
cmake --build build-gui --config Release

# Run
./build-gui/Release/FluxGUI.exe
```

### Linux (GCC/Clang)
```bash
# Configure
cmake -B build-gui flux-gui -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build-gui -j$(nproc)

# Run
./build-gui/FluxGUI
```

### macOS (Xcode)
```bash
# Configure
cmake -G Xcode -B build-gui flux-gui

# Build
cmake --build build-gui --config Release

# Run
open build-gui/Release/FluxGUI.app
```

## Configuration

### Default Settings
The application ships with comprehensive default settings in `resources/config/default-settings.json`:

- **UI Preferences**: Theme, font, scaling, animations
- **Compression Settings**: Default formats, levels, presets
- **System Integration**: Context menus, file associations
- **Performance Tuning**: Memory limits, thread counts
- **Security Options**: Password handling, encryption settings

### Custom Themes
Create custom themes by:
1. Copying an existing `.qss` file from `resources/themes/`
2. Modifying colors, fonts, and styling properties
3. Adding the theme to the resource file
4. Registering it in the ThemeManager

### Compression Presets
Add custom compression presets in `resources/config/compression-presets.json`:
```json
{
  "id": "my_preset",
  "name": "My Custom Preset",
  "description": "Optimized for my use case",
  "format": "7z",
  "level": 7,
  "method": "LZMA2",
  "solidArchive": true
}
```

## Development

### Code Style
- **Modern C++**: Use C++23 features and best practices
- **Qt Conventions**: Follow Qt naming and coding standards
- **Documentation**: Comprehensive inline documentation
- **Error Handling**: RAII and exception-safe code

### Adding New Features
1. **UI Components**: Add to appropriate `ui/` subdirectory
2. **Business Logic**: Implement in `core/` with proper separation
3. **Platform Code**: Use `platform/` for OS-specific functionality
4. **Resources**: Add assets to `resources/` and update `.qrc`

### Testing
- **Unit Tests**: Test core functionality in isolation
- **Integration Tests**: Test component interactions
- **UI Tests**: Automated UI testing with Qt Test framework
- **Manual Testing**: Cross-platform compatibility verification

## Roadmap

### Version 2.1
- [ ] Plugin system for custom archive formats
- [ ] Cloud storage integration (Google Drive, OneDrive)
- [ ] Advanced search and filtering
- [ ] Batch rename and organize tools

### Version 2.2
- [ ] Archive comparison and diff tools
- [ ] Automated backup scheduling
- [ ] Archive integrity monitoring
- [ ] Performance profiling and optimization

### Version 3.0
- [ ] Web interface for remote management
- [ ] Mobile companion app
- [ ] AI-powered file organization
- [ ] Advanced compression algorithms

## Contributing

1. **Fork** the repository
2. **Create** a feature branch (`git checkout -b feature/amazing-feature`)
3. **Commit** your changes (`git commit -m 'Add amazing feature'`)
4. **Push** to the branch (`git push origin feature/amazing-feature`)
5. **Open** a Pull Request

## License

This project is licensed under the MIT License - see the [LICENSE](../LICENSE) file for details.

## Acknowledgments

- **Qt Framework**: Cross-platform application development
- **Material Design**: Google's design system inspiration
- **7-Zip**: Archive format support and algorithms
- **Contributors**: All developers who have contributed to this project