# [PhobosExt](https://github.com/Chang-zhi/PhobosExt_Changzhi)

一个扩展《红色警戒2：尤里的复仇》游戏功能的 DLL，基于 Phobos 开发，作者[*Chang_zhi*](https://space.bilibili.com/423792550)。  
主要面向 任务/地图 作者，可以自由的将其用于任务包或模组制作。

**尽管叫 PhobosExt , 但不依赖 Ares 或 Phobos，可独立运行。推荐和其一同使用。**

**具体新增内容请见说明文档.html**

说是基于 Phobos，其实只是删了删代码 (
<span style="color: gray;">低创作品，大佬轻喷</span>

---

### 兼容性说明

需保证游戏版本为 YR 1.001。  
**不依赖 Ares 或 Phobos，可单独使用**，与 Ares、Phobos 共存不会冲突。  
仅有部分功能依赖 Phobos：触发行为 `653`、`654`（需 Phobos v0.5-alpha 及以上）。  
本 DLL 已避免使用 `ExtPointerOffset` ，改用独立 `unordered_map` 存储扩展数据，不会与 Ares 或 Phobos 的原生扩展数据冲突。(严格来说可能变慢, 但对现代cpu的影响应该可以忽略不计) 

---

### 已知问题

1. 使用 `Temporal=yes` + `Temporal.Exclusive=yes` 的武器会改变单位的选敌逻辑，可能出现异常。
2. `LegalTargetWhenAIOwner=no` 的 AI 单位会改变其他单位的选敌逻辑，可能出现异常。
3. 基地节点跨所属方判定开启后：与触发行为 `30`（自动建设基地）不兼容，且不要让 AI 造围墙。

---

### 反馈

遇到 bug 或兼容性问题欢迎反馈：B站私信 <https://space.bilibili.com/423792550> ｜ 邮箱 3071564490@qq.com

---

### 致谢
[*Ares*](https://github.com/Ares-Developers/Ares) 项目组   
[*Phobos*](https://github.com/Phobos-developers/Phobos) 项目组   
[*YRpp*](https://github.com/Phobos-developers/YRpp) 项目组  
韩大妈 [*@B站主页*](https://space.bilibili.com/2229647)   
九千天华 [*@B站主页*](https://space.bilibili.com/362533219)   
偏微whyffu [*@B站主页*](https://space.bilibili.com/41073096)    
妖妖酱 [*@GitHub*](https://github.com/yaoyaojiang)   

<span style="color: gray;">排名不分先后</span>

---

### 许可
Github 仓库: [**PhobosExt_Changzhi**](https://github.com/Chang-zhi/PhobosExt_Changzhi)    
本项目代码采用与 Phobos 相同的 `GPL-3.0 license` 许可证。     
《红色警戒2：尤里的复仇》及其相关素材归 Electronic Arts 所有。  

---

## 触发编辑器配置

为了在触发编辑器中使用新的触发动作，您需要复制压缩包里面的 `FAData_Customized.ini` 到地编的根目录下。

仅支持[FA2SP_HDM_Edition](https://github.com/handama/FA2sp)(韩大妈版本)
其他版本请自行移植。
