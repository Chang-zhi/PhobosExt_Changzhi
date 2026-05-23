# [PhobosExt_Changzhi](https://github.com/Chang-zhi/PhobosExt_Changzhi)

一个扩展《红色警戒2：尤里的复仇》游戏功能的 DLL，基于 Phobos 开发。

**理论上可以脱离 Ares 和 Phobos 单独运行，但仍建议与原版 Phobos 或 Ares 一同使用**

说是基于 Phobos，其实只是删了删代码 (  
低创作品，大佬轻喷。

---

### 兼容性说明

与 Ares 或 Phobos 共存时，本 DLL 已避免使用 ExtPointerOffset，改用独立 unordered_map 存储扩展数据，不会与它们的原生扩展数据冲突。（严格来说可能变慢, 但对现代cpu的影响应该可以忽略不计）

需保证游戏版本为 YR 1.001。

---

### 已知问题  
当单位当前正在使用带有`TemporalExclusive=yes`的武器时, 会强制改变单位的索敌逻辑, 可能会产生bug

---

### 致谢
[*Ares*](https://github.com/Ares-Developers/Ares) 项目组  
[*Phobos*](https://github.com/Phobos-developers/Phobos) 项目组  
偏微whyffu [*@B站主页*](https://space.bilibili.com/41073096)  

<span style="color: gray;">排名不分先后</span>

---

### 许可
本项目代码采用与 Phobos 相同的许可证。（详见 LICENSE 文件）  
《红色警戒2：尤里的复仇》及其相关素材归 Electronic Arts 所有。

---

## 新增功能

### 1. 路径点文本显示

在地图路径点上绘制自定义文本，支持触发动作 550、551、552。

**特性**：
- 可自定义文本内容、显示宽度（像素）、背景不透明度。
- 可分别设置文本颜色和边框颜色（预定义 9 种颜色）。
- 鼠标移动到文本框上时，文本会暂时停止绘制（避免遮挡操作）。

**触发动作说明**：
- `550` – 在指定路径点绘制文本...（参数：路径点索引，文本内容，宽度，不透明度，颜色）
- `551` – 清除指定路径点的文本...（参数：路径点索引）
- `552` – 清除所有路径点文本...

### 2. 超时空武器互斥锁定

为 `Temporal=yes` 的弹头增加互斥锁定机制。

```ini
[WarheadType]
TemporalExclusive=              ; boolean（布尔值），默认 false
```
当弹头同时设置 `Temporal=yes` 和 `TemporalExclusive=yes` 时，  
该武器无法瞄准已被其他 `TemporalClass` 实例锁定的单位（即正在遭受超时空冻结的单位）。

普通的超时空武器（`Temporal=yes` 但未设置 `TemporalExclusive`）仍可以攻击已被互斥武器锁定的单位。

互斥武器不能攻击被普通超时空武器冻结的目标。

---

## 触发编辑器配置

为了在触发编辑器（FinalAlert 2）中使用新的触发动作（550、551、552），需要修改 `FAData.ini` 或对应版本的配置文件。

### 标准 FinalAlert 2
编辑 `FAData.ini`，在 `[ActionsRA2]` 小节中添加以下内容：

```ini
[ActionsRA2]
...
550=在指定路径点绘制文本... (PhobosExt by Chang_zhi),-4,13,30,6,6,6,0,0,0,在指定路径点绘制文本。\n第三个数值是宽度(像素，1~1000，默认250)。\n第四个数值是背景不透明度(百分比，0~100)。\n第五个数值是颜色(金=0，白=1，红=2，蓝=3，绿=4，黄=5，紫=6，粉=7，淡蓝=8),0,1,902,1
551=清除指定路径点的文本... (PhobosExt by Chang_zhi),0,0,30,0,0,0,0,0,0,清除指定路径点的文本。,0,1,903,1
552=清除所有路径点文本... (PhobosExt by Chang_zhi),0,0,0,0,0,0,0,0,0,清除屏幕上所有已设置的路径点文本。,0,1,904,1
...
```

### [*FA2SP_HDM_Edition*](https://github.com/handama/FA2sp)（韩大妈版本）
需要同时修改 FAData_TriggerAndScript.ini 中的 `[English-ActionsRA2]` 和 `[Chinese-ActionsRA2]` 小节。

在`[English-ActionsRA2]`小节中添加:
```ini
[English-ActionsRA2]
...
550=Draw Text at Specified Waypoint... (PhobosExt by Chang_zhi),-4,13,30,6,6,6,0,0,0,Draw text at the specified waypoint.\nThe third value is the width in pixels (1~1000，default: 250).\nThe fourth value is the background opacity percentage (0~100).\nThe fifth value is the color (Gold=0，White=1，Red=2，Blue=3，Green=4，Yellow=5，Purple=6，Pink=7，Light Blue=8),0,1,902,1
551=Clear Text at Specified Waypoint... (PhobosExt by Chang_zhi),0,0,30,0,0,0,0,0,0,Clear the text at the specified waypoint.,0,1,903,1
552=Clear All Waypoint Texts... (PhobosExt by Chang_zhi),0,0,0,0,0,0,0,0,0,Clear all waypoint texts displayed on screen.,0,1,904,1
...
```
在`[Chinese-ActionsRA2]`小节中中添加:
```ini
[Chinese-ActionsRA2]
...
550=在指定路径点绘制文本... (PhobosExt by Chang_zhi),-4,13,30,6,6,6,0,0,0,在指定路径点绘制文本。\n第三个数值是宽度(像素，1~1000，默认250)。\n第四个数值是背景不透明度(百分比，0~100)。\n第五个数值是颜色(金=0，白=1，红=2，蓝=3，绿=4，黄=5，紫=6，粉=7，淡蓝=8),0,1,902,1
551=清除指定路径点的文本... (PhobosExt by Chang_zhi),0,0,30,0,0,0,0,0,0,清除指定路径点的文本。,0,1,903,1
552=清除所有路径点文本... (PhobosExt by Chang_zhi),0,0,0,0,0,0,0,0,0,清除屏幕上所有已设置的路径点文本。,0,1,904,1
...
```