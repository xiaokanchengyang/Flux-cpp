# Flux GUI Application
QT += core widgets concurrent network

TARGET = FluxGUI
TEMPLATE = app

# C++ 标准
CONFIG += c++20

# 源文件
SOURCES += \
    src/main.cpp \
    src/main_window.cpp \
    src/views/welcome_view.cpp \
    src/views/pack_view.cpp \
    src/views/extract_view.cpp \
    src/views/browse_view.cpp \
    src/utils/file_utils.cpp \
    src/models/archive_model.cpp

# 头文件
HEADERS += \
    src/main_window.h \
    src/views/welcome_view.h \
    src/views/pack_view.h \
    src/views/extract_view.h \
    src/views/browse_view.h \
    src/utils/file_utils.h \
    src/models/archive_model.h

# 包含路径
INCLUDEPATH += src

# 资源文件
RESOURCES += resources/resources.qrc

# Windows 特定设置
win32 {
    CONFIG += windows
    RC_FILE = resources/flux.rc
}

# 输出目录
DESTDIR = bin
OBJECTS_DIR = build/obj
MOC_DIR = build/moc
RCC_DIR = build/rcc
UI_DIR = build/ui

# 编译器设置
QMAKE_CXXFLAGS += -std=c++20

# 链接 flux-core 库 (如果可用)
# LIBS += -L../flux-core/lib -lflux-core
# INCLUDEPATH += ../flux-core/include




