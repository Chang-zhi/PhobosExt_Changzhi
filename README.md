# [PhobosExt](https://github.com/Chang-zhi/PhobosExt_Changzhi)

一个扩展《红色警戒2：尤里的复仇》游戏功能的 DLL，基于 Phobos 开发，作者[*Chang_zhi*](https://space.bilibili.com/423792550)。

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

在地图路径点上绘制自定义文本。

**特性**：
- 可自定义文本内容、显示宽度（像素）、背景不透明度。
- 可分别设置文本颜色和边框颜色（预定义 9 种颜色）。
- 鼠标移动到文本框上时，文本会暂时停止绘制（避免遮挡操作）。

**触发动作说明**：
- `550` – 在指定路径点绘制文本...（参数：路径点索引，文本内容，宽度，不透明度，颜色）
- `551` – 清除指定路径点的文本...（参数：路径点索引）
- `552` – 清除所有路径点文本...

---

### 2. 动态标签绑定功能
<span style="color: red;">注意!   
触发系统事实上很复杂，需读者了解其组成和运行机制。  
这里默认读者已经具备了相关知识，可以阅读 `Handama` 的 `触发系统.docx` 以了解相关知识。
</span>

允许将游戏中的科技类型实例与标签（Tag）动态关联，便于后续通过触发器执行逻辑。  
若标签不存在/已销毁，则会根据标签类型（TagType）创建一个新标签，确保目标会与标签关联。  

<span style="color: gray;">事实上，触发也会在小队创建时实例化，因此动作 `553` 和 `554` 实际上可能并没有必要，  
只需要在 `作战小队` 的 `挂载标签` 上挂载所需标签即可以实现与该动作类似的效果，但仍保留以备不时之需。   
（写都写了.jpg，不过确实可能有不同之处）  
</span>

**触发动作说明**：
- `553` – 将指定小队全部成员关联到指定标签...（参数：小队索引，标签索引）
- `554` – 将指定所属方的指定小队全部成员关联到指定标签...（参数：小队索引，标签索引，所属方国家索引）
- `555` – 将特定科技类型全部关联到指定标签...（参数：科技类型ID，标签索引）
- `556` – 将指定所属方的特定科技类型全部关联到指定标签...（参数：科技类型ID，标签索引，所属方国家索引）

<span style="color: red;">注意!   
1.
使用本触发时，会更新指定标签的 `InstanceCount` 。   
2.
哪怕想要绑定的科技类型正在建造中也会被关联到指定标签。  
3.
通过此触发给小队成员关联的的标签，小队解散后标签不会消失。  
4.
只有一开始就存在的标签可以被摧毁，由本触发创建的标签被摧毁会弹框。  
5.
当指定对象的已经有标签时，新标签会覆盖原有的标签，原有标签的 `InstanceCount` 可能不会更新。  
</span> 

---

### 3. 金钱操作功能
允许动态调整指定所属方的资金数额。  
所有操作均以所属方国家索引作为目标标识，金额单位为游戏内货币单位（即“钱”）。

**触发动作说明**：
- `557` – 为指定所属方添加金钱数额...（参数：所属方国家索引，金额数额）
- `558` – 为指定所属方扣除金钱数额...（参数：所属方国家索引，金额数额）
- `559` – 设置指定所属方的金钱数额...（参数：所属方国家索引，金额数额）

---

### 4. 基地节点操作功能
允许动态添加或移除指定所属方的AI基地节点。  

**触发动作说明**：

- `560` – 在指定路径点添加基地节点...（参数：所属方国家索引，路径点编号，建筑类型索引）
- `561` – 移除指定路径点上的所有基地节点...（参数：所属方国家索引，路径点编号）
- `562` – 移除特定所属方所有指定建筑类型的基地节点...（参数：所属方国家索引，建筑类型索引）

---

### 5. 科技类型坐标检测
在指定路径点上或附近距离，检测是否存在触发所属方的科技类型。

**触发事件说明**：   
- `550` – 路径点附近存在所属方的科技类型... （参数：距离，路径点索引）  
- `551` – 路径点附近不存在所属方的科技类型... （参数：距离，路径点索引）  
**注**: 对于行为 `550`,`551`, 使用使用切比雪夫距离判断是否在方形范围内。
- `552` – 路径点上存在所属方的指定科技类型... （参数：路径点索引，科技类型）  
- `553` – 路径点上不存在所属方的指定科技类型... （参数：路径点索引，科技类型）  
**注**: 对于行为 `552`,`553`, 若科技类型为建筑，则每一格都会检测，不支持ares的自定义Foundation。

---


### 6. 超时空武器互斥锁定

为 `Temporal=yes` 的弹头增加互斥锁定机制。

```ini
[WarheadType]
TemporalExclusive=              ; boolean（布尔值），默认 false
```
当弹头同时设置 `Temporal=yes` 和 `TemporalExclusive=yes` 时，  
该武器无法瞄准已被其他 `TemporalClass` 实例锁定的单位（即正在遭受超时空冻结的单位）。

普通的超时空武器（ `Temporal=yes` 但未设置 `TemporalExclusive` ）仍可以攻击已被互斥武器锁定的单位。

互斥武器不能攻击被普通超时空武器冻结的目标。

---

### 7. 自动游猎
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
编辑 `FAData.ini`，需在 `[EventsRA2]` 和 `[ActionsRA2]` 小节中添加以下内容：


```ini
[EventsRA2]
...
550=路径点附近存在所属方的任意科技类型... (PhobosExt by Chang_zhi),48,30,0,0,当指定路径点的指定距离内存在触发所属方任意科技类型时，此事件被满足。,0,1,550,1
551=路径点附近不存在所属方的任意科技类型... (PhobosExt by Chang_zhi),48,30,0,0,当指定路径点的指定距离内不存在触发所属方任意科技类型时，此事件被满足。,0,1,551,1
552=路径点上存在所属方的指定科技类型... (PhobosExt by Chang_zhi),49,46,0,0,当路径点上存在触发所属方的指定科技类型时，此事件被满足。(若科技类型为建筑，则每一格都会检测，不支持ares的自定义Foundation),0,1,552,1
553=路径点上不存在所属方的指定科技类型... (PhobosExt by Chang_zhi),49,46,0,0,当路径点上不再存在触发所属方的指定科技类型时，此事件被满足。(若科技类型为建筑，则每一格都会检测，不支持ares的自定义Foundation),0,1,553,1
...
```

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
560=在指定路径点添加基地节点...,0,0,2,30,8,0,0,0,0,在指定路径点添加指定所属方的基地节点。,0,1,560,1
561=移除指定路径点上的所有基地节点...,0,0,2,30,0,0,0,0,0,移除指定路径点上指定所属方的所有基地节点。,0,1,561,1
562=移除特定所属方所有指定建筑类型的基地节点...,0,0,2,8,0,0,0,0,0,移除特定所属方所有指定建筑类型的基地节点。,0,1,562,1

...
```

### [*FA2SP_HDM_Edition*](https://github.com/handama/FA2sp)（韩大妈版本）
需同时编辑 `FAData_TriggerAndScript.ini` 中的   
`[English-EventsRA2]`, `[Chinese-EventsRA2]`, `[English-ActionsRA2]`, `[Chinese-ActionsRA2]` 小节。

添加以下内容:

```ini
[English-EventsRA2]
...
550=There is a techno type of the house near the waypoint... (PhobosExt by Chang_zhi),48,30,0,0,This event is satisfied when there is any techno type of the triggering house within the specified distance of the specified waypoint.,0,1,550,1
551=There is no techno type of the house near the waypoint... (PhobosExt by Chang_zhi),48,30,0,0,This event is satisfied when there is no techno type of the triggering house within the specified distance of the specified waypoint.,0,1,551,1
552=Specified techno type of house exists at waypoint... (PhobosExt by Chang_zhi),49,46,0,0,This event is satisfied when the specified techno type of the triggering house exists at the waypoint. (If the techno type is a building，every cell of its foundation will be checked. Does not support Ares' custom Foundation shapes.),0,1,552,1
553=Specified techno type of house does NOT exist at waypoint... (PhobosExt by Chang_zhi),49,46,0,0,This event is satisfied when the specified techno type of the triggering house does NOT exist at the waypoint. (If the techno type is a building，every cell of its foundation will be checked. Does not support Ares' custom Foundation shapes.),0,1,553,1
...
```

```ini
[Chinese-EventsRA2]
...
550=路径点附近存在所属方的任意科技类型... (PhobosExt by Chang_zhi),48,30,0,0,当指定路径点的指定距离内存在触发所属方任意科技类型时，此事件被满足。,0,1,550,1
551=路径点附近不存在所属方的任意科技类型... (PhobosExt by Chang_zhi),48,30,0,0,当指定路径点的指定距离内不存在触发所属方任意科技类型时，此事件被满足。,0,1,551,1
552=路径点上存在所属方的指定科技类型... (PhobosExt by Chang_zhi),49,46,0,0,当路径点上存在触发所属方的指定科技类型时，此事件被满足。(若科技类型为建筑，则每一格都会检测，不支持ares的自定义Foundation),0,1,552,1
553=路径点上不存在所属方的指定科技类型... (PhobosExt by Chang_zhi),49,46,0,0,当路径点上不再存在触发所属方的指定科技类型时，此事件被满足。(若科技类型为建筑，则每一格都会检测，不支持ares的自定义Foundation),0,1,553,1
...
```


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
560=Add base node at waypoint...,0,0,2,30,8,0,0,0,0,Add base node for specified house at waypoint.,0,1,560,1
561=Remove all base nodes at waypoint...,0,0,2,30,0,0,0,0,0,Remove all base nodes for specified house at waypoint.,0,1,561,1
562=Remove all base nodes of specified building type for house...,0,0,2,8,0,0,0,0,0,Remove all base nodes of specified building type for house.,0,1,562,1
...
```

```ini
[Chinese-ActionsRA2]
...
550=在指定路径点绘制文本... (PhobosExt by Chang_zhi),-4,13,30,6,6,6,0,0,0,在指定路径点绘制文本。\n第三个数值是宽度(像素，1~1000，默认250)。\n第四个数值是背景不透明度(百分比，0~100)。\n第五个数值是颜色(金=0，白=1，红=2，蓝=3，绿=4，黄=5，紫=6，粉=7，淡蓝=8),0,1,902,1
551=清除指定路径点的文本... (PhobosExt by Chang_zhi),0,0,30,0,0,0,0,0,0,清除指定路径点的文本。,0,1,903,1
552=清除所有路径点文本... (PhobosExt by Chang_zhi),0,0,0,0,0,0,0,0,0,清除屏幕上所有已设置的路径点文本。,0,1,904,1
553=将指定小队全部成员关联到指定标签... (PhobosExt by Chang_zhi),0,0,7,38,0,0,0,0,0,将指定小队内所有成员关联到指定标签。若标签（Tag）不存在/已销毁，则会根据标签类型（TagType）创建一个新标签（Tag），确保目标会与标签关联。(详细细节请见readme.md里的相关介绍),0,1,553,1
554=将指定所属方的指定小队全部成员关联到指定标签... (PhobosExt by Chang_zhi),0,0,7,38,2,0,0,0,0,将指定所属方下的指定小队所有成员关联到指定标签。若标签（Tag）不存在/已销毁，则会根据标签类型（TagType）创建一个新标签（Tag），确保目标会与标签关联。(详细细节请见readme.md里的相关介绍),0,1,554,1
555=将特定科技类型全部关联到指定标签... (PhobosExt by Chang_zhi),-4,46,38,0,0,0,0,0,0,将指定科技类型的所有实例关联到指定标签。若标签（Tag）不存在/已销毁，则会根据标签类型（TagType）创建一个新标签（Tag），确保目标会与标签关联。(详细细节请见readme.md里的相关介绍),0,1,555,1
556=将指定所属方的特定科技类型全部关联到指定标签... (PhobosExt by Chang_zhi),-4,46,38,2,0,0,0,0,0,将指定所属方下指定科技类型的所有实例关联到指定标签。若标签（Tag）不存在/已销毁，则会根据标签类型（TagType）创建一个新标签（Tag），确保目标会与标签关联。(详细细节请见readme.md里的相关介绍),0,1,556,1
557=为所属方添加金钱数额...,0,0,2,6,0,0,0,0,0,为指定所属方添加指定的金钱数额。,0,1,557,1
558=为所属方扣除金钱数额...,0,0,2,6,0,0,0,0,0,为指定所属方扣除指定的金钱数额。,0,1,558,1
559=设置所属方的金钱数额...,0,0,2,6,0,0,0,0,0,设置指定所属方的金钱数额。,0,1,559,1
560=在指定路径点添加基地节点...,0,0,2,30,8,0,0,0,0,在指定路径点添加指定所属方的基地节点。,0,1,560,1
561=移除指定路径点上的所有基地节点...,0,0,2,30,0,0,0,0,0,移除指定路径点上指定所属方的所有基地节点。,0,1,561,1
562=移除特定所属方所有指定建筑类型的基地节点...,0,0,2,8,0,0,0,0,0,移除特定所属方所有指定建筑类型的基地节点。,0,1,562,1
...
```
