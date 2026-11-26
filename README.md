# SplitColors

根据一张原图和多张掩码生成图层。每张掩码独立处理，图层保留原图像素颜色；掩码可以重叠。

## 输入

在 `SC_INPUT_DIR` 中放入一张彩图和至少一张掩码。程序默认自动识别彩图；若原图接近灰色、存在多张彩图或自动识别错误，请将 `SC_SOURCE_IMAGE` 设置为原图文件名。

也可以在输入目录中为每组图片创建一个子目录，程序会分别处理这些子目录。设置 `SC_SOURCE_IMAGE` 后，每组都要有同名的原图；若各组文件名不同，请留空并使用自动识别。

不透明的黑白掩码默认以黑色为选区。白色为选区时设置 `MASK_FOREGROUND=white`。带透明区域的掩码直接使用透明度，不受这一设置影响。原图与掩码应描述同一画布，允许不同分辨率；长宽比相差超过 1% 时会报错，不会强行拉伸。

## 配置与运行

编辑 `.env`，设置输入和输出目录。相对路径以 `.env` 所在目录为准。程序优先读取当前工作目录的 `.env`；若没有，再读取可执行文件旁边的 `.env`。也可通过命令行传入配置文件路径：

```text
SplitColors.exe path/to/.env
```

`MASK_EDGES=soft` 保留灰度及透明边缘，并对不透明黑白掩码的两端做轻微截断，减少 JPEG 近黑或近白的噪声。需要硬边缘时使用 `MASK_EDGES=binary`，通过 `MASK_THRESHOLD` 调整选区阈值（1–255）。

## 输出

每组图片生成一个独立目录，其中 `layers/` 保存透明背景的彩色图层。文件名中的编号和掩码名称对应输入掩码；软边缘会保留半透明像素。各掩码可以重叠，对应图层也可以重叠。

已有结果不会被覆盖：重复处理会创建带数字后缀的新目录。输出目录不能位于输入目录内。

## 构建

需要 CMake、C++20 编译器以及与编译器匹配的 OpenCV（core、imgcodecs、imgproc）：

```text
cmake -S . -B build
cmake --build build --config Release
```

## Windows 安装包

`SplitColors-Setup-x64.exe` 是面向 x64 Windows 的离线安装包，包含 Release 程序、OpenCV DLL 和应用本地的 MSVC 运行库。默认安装到当前用户的本地程序目录，无需管理员权限；输入和输出目录也位于安装目录中，并有开始菜单快捷方式。Windows 10/11 标准版包含所需的通用 C 运行库；其他系统环境需要另行验证。应用本地运行库不会随系统中的 VC++ Redistributable 自动更新，更新工具链后应重新打包。

重新制作安装包：安装 x64-windows 的 OpenCV 和 Inno Setup，在 x64 Visual Studio Developer PowerShell 中运行：

```powershell
.\packaging\build.ps1 -VcpkgRoot D:\vcpkg -InnoCompiler 'C:\path\to\ISCC.exe'
```

安装脚本保留已有 `.env`、输入和输出文件夹，不会在卸载时清除用户数据。安装程序尚未进行代码签名，Windows 可能提示未知发布者。
