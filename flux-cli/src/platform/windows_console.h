#pragma once

#ifdef _WIN32

namespace FluxCLI::Platform {
    /**
     * 启用 Windows 控制台的 UTF-8 支持
     * 这确保了中文和其他 Unicode 字符能正确显示
     */
    void enableUTF8Console();
    
    /**
     * 启用 Windows 控制台的 ANSI 颜色支持
     * 用于彩色日志输出和进度条
     */
    void enableANSIColors();
    
    /**
     * 获取控制台窗口的宽度
     * 用于自适应进度条宽度
     */
    int getConsoleWidth();
}

#endif // _WIN32

