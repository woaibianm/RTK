# rtkpost 解算引擎 —— 可编译工程

把 RTKLIB rtkpost 的**解算部分**（`postpos.c` / `rtkpos.c` / `pntpos.c` / `ppp.c` 等）
包装成一个命令行程序 `rtkpost_main.exe`，并保留全部 RTKLIB 源码不动，直接引用编译。

## 文件说明

| 文件 | 作用 |
|---|---|
| `rtkpost_main.c` | **解算驱动程序**（含参数标注 ◇需设置/△可更改/○自动）。配置 `prcopt_t`/`solopt_t`/`filopt_t` 后调用 `postpos()` |
| `Makefile` | MinGW-W64 gcc 的 makefile（逐文件编译 src，增量编译） |
| `build.bat` | 双击即可编译，生成 `rtkpost_main.exe` |
| `rtkpost_main.exe` | 编译好的可执行文件 |

## 编译

- 双击 `build.bat`，或命令行执行：
  ```
  mingw32-make -f Makefile
  ```
- 依赖：MinGW-W64 gcc（本机已装）。源码在 `../RTKLIB-rtklib_2.4.3/src`，无需复制、不改动。

## 运行（用 RTKLIB 自带样例数据）

```bat
:: 单点定位
rtkpost_main.exe -p 0 ..\RTKLIB-rtklib_2.4.3\test\data\rinex\07590920.05o ..\RTKLIB-rtklib_2.4.3\test\data\rinex\07590920.05n -o sp.pos

:: 静态RTK（流动站 + 基准站 + 星历，取RINEX头基准站坐标，输出XYZ）
rtkpost_main.exe -p 3 ..\RTKLIB-rtklib_2.4.3\test\data\rinex\07590920.05o ..\RTKLIB-rtklib_2.4.3\test\data\rinex\30400920.05o ..\RTKLIB-rtklib_2.4.3\test\data\rinex\07590920.05n -e -o rtk.pos
```
文件顺序：第 1 个=流动站观测，第 2 个=基准站观测（RTK 用），其余=导航/精密星历。

## 常用参数

```
-p mode  定位模式 0单点/1DGPS/2动态RTK/3静态RTK/4动基线/5固定/6PPP动态/7PPP静态
-m deg   高度角掩蔽(°)
-f n     频率数 1/2/3
-sys s   卫星系统 G:R/E/J/C/I 组合，如 -sys GEC
-v thres 整周模糊度ratio阈值(0=不做固定)
-c       前后向结合解算（更平滑）
-b       后向解算
-i       瞬时模糊度固定 / -h 固定并保持模糊度
-k file  从 RTKLIB 配置文件读入处理选项（如 rtkpost GUI 导出的 .conf）
-r x y z 基准站坐标 ECEF(m)   （-l 可用经纬高）
-o file  输出文件
-e/-a/-n 输出XYZ / ENU基线 / NMEA语句   （默认LLH）
-ts/-te  起止时刻(y/m/d h:m:s)   -ti n 采样间隔(s)
-?       打印帮助（无输入文件/非法选项时退出码=2）
```
不带任何选项运行可看完整帮助。

## 结果文件 Q 值含义

`Q=1` 固定解(FIX) · `Q=2` 浮点解(FLOAT) · `Q=3` SBAS · `Q=4` DGPS · `Q=5` 单点 · `Q=6` PPP

## 校核与验证结论（2026-08）

已对照 RTKLIB 2.4.3 官方 `rnx2rtkp.c` 逐项校核并修正：

| 项目 | 说明 |
|---|---|
| `-k` 配置文件 | 已补齐（与 rnx2rtkp 一致，配置先载入、命令行覆盖） |
| `-n` NMEA 输出 | 已补齐 |
| `-?` 帮助 / 退出码 | `-?`→0；无输入文件/非法选项→2；配置加载失败→1 |
| `postpos()` 返回码 | 0=正常完成、1=用户中止、-1=会话打开失败，直接透传（与 rnx2rtkp 一致） |
| `-sys` 解析 | 修正了官方 rnx2rtkp 中 switch 无 break 导致选择任意系统都启用全部系统的缺陷 |
| 输出文件头程序名 | `-k` 后重新写回程序名（官方 rnx2rtkp 该处显示 "RTKLIB ver.2.4.3"） |
| Makefile clean | `rm -f` 改 `$(RM)`（Windows 无 rm） |

**数据验证**（解算结果/rover.obs+base.obs+rover.nav，2026/08/13 10Hz 数据）：
与官方 rnx2rtkp.exe 同参数逐行比对——SPP/DGPS/静态RTK/动态RTK 浮点解全部一致
（差异仅 0.1 mm 级末位舍入，源于编译器不同）；前后向结合解差异 ≤1.3 cm（浮点噪声放大）。
注意：本数据若启用北斗(含 C)且电离层用广播模型(默认)，SPP 会因 RTKLIB 2.4.3
对北斗+Klobuchar 的处理缺陷全部历元失败——官方 rnx2rtkp.exe 同样如此，属引擎/数据限制，
非本驱动问题。规避：`-sys` 不含 C，或用 `-iono 0` 关闭广播电离层。

**编译注意**：MinGW gcc 安装路径含中文（如本工作区 toolchain）时链接会因路径编码损坏失败；
请使用纯 ASCII 路径的 gcc（如 WinGet 安装的 WinLibs：`C:\Users\...\WinGet\Packages\...\mingw64\bin`），
或把 build.bat / Makefile 的 `CC` 指到该 gcc。

