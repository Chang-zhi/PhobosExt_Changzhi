# [PhobosExt](https://github.com/Chang-zhi/PhobosExt_Changzhi)

一个扩展《红色警戒2：尤里的复仇》游戏功能的 DLL，基于 Phobos 开发，作者[Chang_zhi](https://space.bilibili.com/423792550)。

**理论上可以脱离 Ares 和 Phobos 单独运行，但仍建议与原版 Phobos 或 Ares 一同使用**

说是基于 Phobos，其实只是删了删代码 (  
<span style="color: gray;">低创作品，大佬轻喷。</span>

---

### 兼容性说明

需保证游戏版本为 YR 1.001。

与 Ares 或 Phobos 共存时，本 DLL 已避免使用 ExtPointerOffset，改用独立 unordered_map 存储扩展数据，不会与它们的原生扩展数据冲突。（严格来说可能变慢, 但对现代cpu的影响应该可以忽略不计）

---

### 已知问题  
当单位当前正在使用带有 `Temporal=yes` 和 `TemporalExclusive=yes` 弹头的武器时, 会强制改变单位的索敌逻辑, 可能会产生bug

---

### 致谢
[*Ares*](https://github.com/Ares-Developers/Ares) 项目组  
[*Phobos*](https://github.com/Phobos-developers/Phobos) 项目组  
偏微whyffu [*@B站主页*](https://space.bilibili.com/41073096)  
韩大妈 [*@B站主页*](https://space.bilibili.com/2229647)  
妖妖酱 [*@GitHub*](https://github.com/yaoyaojiang)

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

### 2. 动态标签绑定功能

允许将游戏中的单位（小队成员、特定科技类型）与标签（Tag）动态关联，便于后续通过触发器执行逻辑。  
若标签（Tag）不存在/已销毁，则会根据标签类型（TagType）创建一个新标签（Tag），确保目标会与标签关联。  

**触发动作说明**：
- `553` – 将指定小队全部成员关联到指定标签...（参数：小队索引，标签索引）
- `554` – 将指定所属方的指定小队全部成员关联到指定标签...（参数：小队索引，标签索引，所属方国家索引）
- `555` – 将特定科技类型全部关联到指定标签...（参数：科技类型ID，标签索引）
- `556` – 将指定所属方的特定科技类型全部关联到指定标签...（参数：科技类型ID，标签索引，所属方国家索引）

### 3. 金钱操作功能
允许动态调整指定所属方（玩家或电脑阵营）的资金数额，支持添加、扣除和直接设置。  
所有操作均以所属方国家索引（House index）作为目标标识，金额单位为游戏内货币单位（即“钱”）。

**触发动作说明**：
- `557` – 为指定所属方添加金钱数额...（参数：所属方国家索引，金额数额）
- `558` – 向指定所属方扣除金钱数额...（参数：所属方国家索引，金额数额）
- `559` – 设置指定所属方的金钱数额...（参数：所属方国家索引，金额数额）

### 4. 超时空武器互斥锁定

为 `Temporal=yes` 的弹头增加互斥锁定机制。

```ini
[WarheadType]
TemporalExclusive=              ; boolean（布尔值），默认 false
```
当弹头同时设置 `Temporal=yes` 和 `TemporalExclusive=yes` 时，  
该武器无法瞄准已被其他 `TemporalClass` 实例锁定的单位（即正在遭受超时空冻结的单位）。

普通的超时空武器（ `Temporal=yes` 但未设置 `TemporalExclusive` ）仍可以攻击已被互斥武器锁定的单位。

互斥武器不能攻击被普通超时空武器冻结的目标。

### 5. 自动游猎
```ini
[TechnoType]
AutoHunt=                       ; boolean（布尔值），默认 false
```
当科技类型设置 `AutoHunt=yes` 时，该类型的所有单位（仅限 AI 控制）会自动强制进入 `Hunt` 状态，主动搜索并攻击敌方目标。

**行为细节**  

- 仅限 AI：人类玩家控制的单位不会受此影响。
- 帧间隔：每个 `AutoHunt=yes` 的单位每 `15` 帧执行一次逻辑检查，避免性能浪费。
- 载具内无效：如果单位位于载具内，AutoHunt 逻辑不会生效（离开载具后自动生效）。
- 自动解除部署：如果单位当前处于部署状态，会先尝试解除部署，然后进入 `Hunt` 状态。
- 目标可达性检查：当单位有攻击目标时，会检查目标单元格是否可达。如果不可达，则跳过本次逻辑。
- 禁止招募：单位的 `RecruitableA` 和 `RecruitableB` 会强制被设置为`false`，并从所属队伍中释放。
- 攻击地面修正：如果单位当前攻击目标是单元格（强制攻击地面），会自动清除该目标，转为 `Hunt` 索敌。

---

## 触发编辑器配置

为了在触发编辑器（FinalAlert 2）中使用新的触发动作，需要修改 `FAData.ini` 或对应版本的配置文件。

### 标准 FinalAlert 2
编辑 `FAData.ini`，在 `[ActionsRA2]` 小节中添加以下内容：

```ini
[ActionsRA2]
...
550=在指定路径点绘制文本... (PhobosExt by Chang_zhi),-4,13,30,6,6,6,0,0,0,在指定路径点绘制文本。\n第三个数值是宽度(像素，1~1000，默认250)。\n第四个数值是背景不透明度(百分比，0~100)。\n第五个数值是颜色(金=0，白=1，红=2，蓝=3，绿=4，黄=5，紫=6，粉=7，淡蓝=8),0,1,902,1
551=清除指定路径点的文本... (PhobosExt by Chang_zhi),0,0,30,0,0,0,0,0,0,清除指定路径点的文本。,0,1,903,1
552=清除所有路径点文本... (PhobosExt by Chang_zhi),0,0,0,0,0,0,0,0,0,清除屏幕上所有已设置的路径点文本。,0,1,904,1
553=将指定小队全部成员关联到指定标签... (PhobosExt by Chang_zhi),0,0,7,38,0,0,0,0,0,将指定小队内所有成员关联到指定标签。若标签（Tag）不存在/已销毁，则会根据标签类型（TagType）创建一个新标签（Tag），确保目标会与标签关联。,0,1,553,1
554=将指定所属方的指定小队全部成员关联到指定标签... (PhobosExt by Chang_zhi),0,0,7,38,2,0,0,0,0,将指定所属方下的指定小队所有成员关联到指定标签。若标签（Tag）不存在/已销毁，则会根据标签类型（TagType）创建一个新标签（Tag），确保目标会与标签关联。,0,1,554,1
555=将特定科技类型全部关联到指定标签... (PhobosExt by Chang_zhi),-4,46,38,0,0,0,0,0,0,将指定科技类型的所有实例关联到指定标签。若标签（Tag）不存在/已销毁，则会根据标签类型（TagType）创建一个新标签（Tag），确保目标会与标签关联。,0,1,555,1
556=将指定所属方的特定科技类型全部关联到指定标签... (PhobosExt by Chang_zhi),-4,46,38,2,0,0,0,0,0,将指定所属方下指定科技类型的所有实例关联到指定标签。若标签（Tag）不存在/已销毁，则会根据标签类型（TagType）创建一个新标签（Tag），确保目标会与标签关联。,0,1,556,1
557=为所属方添加金钱数额...,0,0,2,6,0,0,0,0,0,为指定所属方添加指定的金钱数额。,0,1,557,1
558=为所属方扣除金钱数额...,0,0,2,6,0,0,0,0,0,为指定所属方扣除指定的金钱数额。,0,1,558,1
559=设置所属方的金钱数额...,0,0,2,6,0,0,0,0,0,设置指定所属方的金钱数额。,0,1,559,1
...
```

### [*FA2SP_HDM_Edition*](https://github.com/handama/FA2sp)（韩大妈版本）
需要同时编辑 `FAData_TriggerAndScript.ini` 中的 `[English-ActionsRA2]` 和 `[Chinese-ActionsRA2]` 小节。

在 `[English-ActionsRA2]` 小节中添加:
```ini
[English-ActionsRA2]
...
550=Draw Text at Specified Waypoint... (PhobosExt by Chang_zhi),-4,13,30,6,6,6,0,0,0,Draw text at the specified waypoint.\nThe third value is the width in pixels (1~1000，default: 250).\nThe fourth value is the background opacity percentage (0~100).\nThe fifth value is the color (Gold=0，White=1，Red=2，Blue=3，Green=4，Yellow=5，Purple=6，Pink=7，Light Blue=8),0,1,902,1
551=Clear Text at Specified Waypoint... (PhobosExt by Chang_zhi),0,0,30,0,0,0,0,0,0,Clear the text at the specified waypoint.,0,1,903,1
552=Clear All Waypoint Texts... (PhobosExt by Chang_zhi),0,0,0,0,0,0,0,0,0,Clear all waypoint texts displayed on screen.,0,1,904,1
553=Associate all members of the specified team to the specified tag... (PhobosExt by Chang_zhi),0,0,7,38,0,0,0,0,0,Associate all members within the specified team to the specified tag. If the tag does not exist or has been destroyed，a new tag will be created according to the tag type (TagType) to ensure the target is associated with the tag.,0,1,553,1
554=Associate all members of the specified team under the specified owner to the specified tag... (PhobosExt by Chang_zhi),0,0,7,38,2,0,0,0,0,Associate all members of the specified team under the specified owner to the specified tag. If the tag does not exist or has been destroyed，a new tag will be created according to the tag type (TagType) to ensure the target is associated with the tag.,0,1,554,1
555=Associate all instances of the specified technotype to the specified tag... (PhobosExt by Chang_zhi),-4,46,38,0,0,0,0,0,0,Associate all instances of the specified technotype to the specified tag. If the tag does not exist or has been destroyed，a new tag will be created according to the tag type (TagType) to ensure the target is associated with the tag.,0,1,555,1
556=Associate all instances of the specified technotype under the specified owner to the specified tag... (PhobosExt by Chang_zhi),-4,46,38,2,0,0,0,0,0,Associate all instances of the specified technotype under the specified owner to the specified tag. If the tag does not exist or has been destroyed，a new tag will be created according to the tag type (TagType) to ensure the target is associated with the tag.,0,1,556,1
556=Associate all instances of the specified technotype under the specified owner to the specified tag... (PhobosExt by Chang_zhi),-4,46,38,2,0,0,0,0,0,Associate all instances of the specified technotype under the specified owner to the specified tag. If the tag does not exist or has been destroyed，a new tag will be created according to the tag type (TagType) to ensure the target is associated with the tag.,0,1,556,1
557=Add a specified amount of money to the specified house...,0,0,2,6,0,0,0,0,0,Adds the specified amount of money to the specified house.,0,1,557,1
558=Deduct a specified amount of money from the specified house...,0,0,2,6,0,0,0,0,0,Deducts the specified amount of money from the specified house.,0,1,558,1
559=Set the money amount for the specified house...,0,0,2,6,0,0,0,0,0,Sets the money amount for the specified house.,0,1,559,1
...
```

在 `[Chinese-ActionsRA2]` 小节中中添加:

```ini
[Chinese-ActionsRA2]
...
550=在指定路径点绘制文本... (PhobosExt by Chang_zhi),-4,13,30,6,6,6,0,0,0,在指定路径点绘制文本。\n第三个数值是宽度(像素，1~1000，默认250)。\n第四个数值是背景不透明度(百分比，0~100)。\n第五个数值是颜色(金=0，白=1，红=2，蓝=3，绿=4，黄=5，紫=6，粉=7，淡蓝=8),0,1,902,1
551=清除指定路径点的文本... (PhobosExt by Chang_zhi),0,0,30,0,0,0,0,0,0,清除指定路径点的文本。,0,1,903,1
552=清除所有路径点文本... (PhobosExt by Chang_zhi),0,0,0,0,0,0,0,0,0,清除屏幕上所有已设置的路径点文本。,0,1,904,1
553=将指定小队全部成员关联到指定标签... (PhobosExt by Chang_zhi),0,0,7,38,0,0,0,0,0,将指定小队内所有成员关联到指定标签。若标签（Tag）不存在/已销毁，则会根据标签类型（TagType）创建一个新标签（Tag），确保目标会与标签关联。,0,1,553,1
554=将指定所属方的指定小队全部成员关联到指定标签... (PhobosExt by Chang_zhi),0,0,7,38,2,0,0,0,0,将指定所属方下的指定小队所有成员关联到指定标签。若标签（Tag）不存在/已销毁，则会根据标签类型（TagType）创建一个新标签（Tag），确保目标会与标签关联。,0,1,554,1
555=将特定科技类型全部关联到指定标签... (PhobosExt by Chang_zhi),-4,46,38,0,0,0,0,0,0,将指定科技类型的所有实例关联到指定标签。若标签（Tag）不存在/已销毁，则会根据标签类型（TagType）创建一个新标签（Tag），确保目标会与标签关联。,0,1,555,1
556=将指定所属方的特定科技类型全部关联到指定标签... (PhobosExt by Chang_zhi),-4,46,38,2,0,0,0,0,0,将指定所属方下指定科技类型的所有实例关联到指定标签。若标签（Tag）不存在/已销毁，则会根据标签类型（TagType）创建一个新标签（Tag），确保目标会与标签关联。,0,1,556,1
557=为所属方添加金钱数额...,0,0,2,6,0,0,0,0,0,为指定所属方添加指定的金钱数额。,0,1,557,1
558=为所属方扣除金钱数额...,0,0,2,6,0,0,0,0,0,为指定所属方扣除指定的金钱数额。,0,1,558,1
559=设置所属方的金钱数额...,0,0,2,6,0,0,0,0,0,设置指定所属方的金钱数额。,0,1,559,1
...
```
