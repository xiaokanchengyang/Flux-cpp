# Flux GUI - 图形界面归档管理器

现代化的归档文件管理工具，提供直观的图形界面。

## 功能特性

- 🎨 现代化的用户界面
- 📦 支持多种归档格式 (ZIP, 7Z, TAR.GZ, TAR.XZ, TAR.ZSTD)
- 🚀 高性能的压缩和解压
- 📁 直观的文件浏览器
- 🔍 快速搜索功能
- 🎯 拖放支持
- 📋 最近文件列表

## 构建要求

- Qt 6.0 或更高版本
- C++20 兼容的编译器
- CMake 3.22+ 或 qmake

## 构建方法

### 使用 CMake

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### 使用 qmake

```bash
qmake flux-gui.pro
make
```

## 项目结构

```
flux-gui/
├── src/                    # 源代码
│   ├── main.cpp           # 程序入口
│   ├── main_window.*      # 主窗口
│   ├── views/             # 视图组件
│   │   ├── welcome_view.* # 欢迎页面
│   │   ├── pack_view.*    # 打包视图
│   │   ├── extract_view.* # 解压视图
│   │   └── browse_view.*  # 浏览视图
│   ├── utils/             # 工具类
│   │   └── file_utils.*   # 文件工具
│   └── models/            # 数据模型
│       └── archive_model.* # 归档模型
├── resources/             # 资源文件
│   └── resources.qrc      # Qt 资源文件
├── CMakeLists.txt         # CMake 配置
└── flux-gui.pro          # qmake 配置
```

## 使用说明

1. **欢迎页面**: 提供快速操作入口和最近文件列表
2. **创建归档**: 选择文件和文件夹，设置压缩参数
3. **打开归档**: 浏览归档内容，提取文件
4. **解压归档**: 批量解压归档文件

## 开发状态

当前版本为开发预览版，包含以下已实现功能：

- ✅ 基础 GUI 框架
- ✅ 欢迎页面
- ✅ 打包视图界面
- ✅ 文件工具类
- ✅ 归档数据模型
- 🚧 解压功能 (开发中)
- 🚧 浏览功能 (开发中)
- 🚧 实际压缩/解压逻辑 (待实现)

## 许可证

本项目采用 MIT 许可证。详见 LICENSE 文件。




