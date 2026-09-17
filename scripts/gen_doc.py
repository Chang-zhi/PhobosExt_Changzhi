# -*- coding: utf-8 -*-
path = "d:\\Documents\\Git\\Phobos-build-48-independentDll\\说明文档.html"

CSS = "*{margin:0;padding:0;box-sizing:border-box}html{scroll-behavior:smooth}body{background-color:#f5f5f5;font-family:'Microsoft Yahei','Segoe UI',Roboto,system-ui,sans-serif;line-height:1.65;color:#222}" \
      ".nav{position:fixed;top:0;left:0;width:150px;height:100vh;background:#fff;border-right:1px solid #eee;z-index:100;overflow-y:auto;padding:1rem 0;display:none}" \
      ".nav__title{font-size:1rem;font-weight:700;color:#d92929;padding:0.5rem 1rem 0.8rem;border-bottom:2px solid #d92929;margin:0 0.8rem 0.5rem;text-align:center}" \
      ".nav__list{list-style:none;padding:0;margin:0}" \
      ".nav__item{display:block;padding:0.45rem 1rem;font-size:0.82rem;color:#555;cursor:pointer;transition:all .15s;border-left:3px solid transparent;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}" \
      ".nav__item:hover{background:#fdf7f7;color:#d92929;border-left-color:#f0c8c8}" \
      ".nav__item--active{background:#fef0f0;color:#d92929;border-left-color:#d92929;font-weight:600}" \
      ".doc-container{max-width:960px;margin:2rem auto;margin-left:190px;background:#fff;border:1px solid #eee;border-radius:15px;padding:2rem 2.2rem 3rem}" \
      "section{display:none}section.active{display:block}" \
      "h1,h2,h3,h4{font-weight:600;margin-top:1.8em;margin-bottom:0.6em;line-height:1.35}" \
      ".doc-title{font-size:2rem;color:#d92929;border-bottom:2px solid #d92929;display:inline-block;padding-bottom:.3em;margin-top:.2em;margin-bottom:1em}" \
      ".doc-title__sub{font-size:.9rem;font-weight:normal;background:#f8f0f0;color:#d92929;padding:.2rem .6rem;border-radius:2px;margin-left:.8rem;vertical-align:middle}" \
      ".doc-heading--secondary{font-size:1.6rem;border-left:4px solid #d92929;padding-left:12px;background:#fdf7f7}" \
      ".doc-heading--tertiary{font-size:1.3rem;color:#b81c1c;border-bottom:1px solid #f0c8c8;padding-bottom:.2em}" \
      ".doc-link{color:#d92929;text-decoration:none;border-bottom:1px solid transparent;transition:.2s}.doc-link:hover{border-bottom-color:#d92929}" \
      "code,pre{font-family:'Consolas','JetBrains Mono',monospace;border-radius:3px}" \
      ".doc-code--inline{padding:.15rem .4rem;font-size:.9em;color:#d92929;background:#fff1f1}" \
      ".doc-code--block{padding:1rem;overflow-x:auto;background:#2b2b2b;color:#e6e6e6;margin:1rem 0;font-size:.85rem}" \
      ".doc-table{width:100%;border-collapse:collapse;margin:1.2rem 0;font-size:.9rem}" \
      ".doc-table__th,.doc-table__td{border:1px solid #ddd;padding:9px 12px;text-align:left;vertical-align:top}" \
      ".doc-table__th{background:#f8f0f0;color:#b81c1c;font-weight:600}" \
      ".doc-notice--warning{background:#fef0f0;border-left:4px solid #d92929;padding:.8rem 1rem;margin:1rem 0;border-radius:2px}" \
      ".doc-list--content{margin:0 4%;line-height:1.8}" \
      ".doc-footer{margin-top:3rem;padding-top:1.2rem;text-align:center;border-top:1px solid #eee;font-size:.85rem;color:#777}" \
      ".doc-text--gray{color:#666;margin:2% 0}.doc-hr{margin:2rem 0;border:none;height:1px;background:#f0c8c8}" \
      ".doc-code--inline--deepColor{padding:.15rem .4rem;font-size:.9em;color:#d92929;background:#f7dfdf}" \
      ".doc-paragraph--only-vertical{margin:1% 0}.doc-paragraph--wrapped{margin:2% 2%}" \
      ".nav-toggle{display:none;position:fixed;top:.6rem;left:.6rem;z-index:200;background:#d92929;color:#fff;border:none;border-radius:4px;padding:.4rem .7rem;font-size:1.1rem;cursor:pointer;box-shadow:0 1px 4px rgba(0,0,0,.15)}" \
      "@media(max-width:800px){.nav{width:130px}.doc-container{margin-left:140px;padding:1.2rem}}" \
      "@media(max-width:640px){.nav{display:none!important}.nav.open{display:block!important;width:160px;box-shadow:2px 0 12px rgba(0,0,0,.15)}" \
      ".doc-container{margin-left:.5rem;margin-right:.5rem;padding:1rem}.nav-toggle{display:block}}"

NAV_ITEMS = [
    ("home", "\U0001F4D6 首页"),
    ("1", "1. 路径点文本"), ("2", "2. 动态标签"), ("3", "3. 金钱操作"),
    ("4", "4. 基地节点"), ("5", "5. 坐标检测"), ("6", "6. 可招募属性"),
    ("7", "7. 建筑动画"), ("8", "8. 超时空武器"), ("9", "9. 自动游猎"),
    ("10", "10. AI目标判定"), ("11", "11. 作战小队"), ("12", "12. 跨所属方"),
    ("13", "13. 流逝时间"), ("14", "14. 混乱机制"),
]

FEATURE_TITLES = [
    None,
    "1. 路径点文本显示", "2. 动态标签绑定功能", "3. 金钱操作功能",
    "4. 基地节点操作功能", "5. 科技类型坐标检测", "6. 单位可招募属性设置",
    "7. AI 更新建筑动画功能", "8. 超时空武器扩展", "9. 自动游猎",
    "10. AI 所属单位目标判定", "11. 作战小队操作", "12. 基地节点跨所属方判定",
    "13. 流逝时间检测", "14. 混乱机制",
]

JS_CODE = """function switchPage(p){var s=document.querySelectorAll('section');for(var i=0;i<s.length;i++)s[i].classList.remove('active');var t=document.getElementById('sec_'+p);if(t)t.classList.add('active');var n=document.querySelectorAll('.nav__item');for(var i=0;i<n.length;i++){n[i].classList.remove('nav__item--active');if(n[i].getAttribute('page')==p)n[i].classList.add('nav__item--active')}window.scrollTo(0,0);var nav=document.getElementById('mainNav');if(nav&&window.innerWidth<=640)nav.classList.remove('open')}
document.addEventListener('DOMContentLoaded',function(){var n=document.querySelectorAll('.nav__item');for(var i=0;i<n.length;i++){(function(item){item.addEventListener('click',function(){var p=this.getAttribute('page');if(p)switchPage(p)})})(n[i])}})
function toggleNav(){document.getElementById('mainNav').classList.toggle('open')}
document.addEventListener('DOMContentLoaded',function(){document.getElementById('mainNav').style.display='block';switchPage('home')})"""

lines = []
lines.append('<!DOCTYPE html>')
lines.append('<html lang="zh-CN">')
lines.append('<head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0">')
lines.append('<title>PhobosExt 说明文档</title><style>' + CSS + '</style></head>')
lines.append('<body>')
lines.append('<button class="nav-toggle" id="navToggle" onclick="toggleNav()">\u2630</button>')
lines.append('<nav class="nav" id="mainNav"><div class="nav__title">PhobosExt</div><ul class="nav__list">')
for page, label in NAV_ITEMS:
    active = ' nav__item--active' if page == 'home' else ''
    lines.append(f'<li class="nav__item{active}" page="{page}">{label}</li>')
lines.append('</ul></nav>')
lines.append('<div class="doc-container">')

# Home section
lines.append('<section id="sec_home" class="active">')
lines.append('<h1 class="doc-title"><a href="https://github.com/Chang-zhi/PhobosExt_Changzhi" target="_blank" class="doc-link" style="border-bottom:none">PhobosExt</a> <span class="doc-title__sub">扩展 DLL</span></h1>')
lines.append('<p>一个扩展《红色警戒2：尤里的复仇》游戏功能的 DLL，基于 Phobos 开发，作者<a href="https://space.bilibili.com/423792550" target="_blank" class="doc-link"><strong>Chang_zhi</strong></a>。</p>')
lines.append('<p>主要面向 任务/地图 作者，可以自由的将其用于任务包或模组制作。</p>')
lines.append('<p><strong>尽管叫 PhobosExt , 但不依赖 Ares 或 Phobos，可独立运行。推荐和其一同使用。</strong></p>')
lines.append('<p>说是基于 Phobos，其实只是删了删代码 ( <span class="doc-title--gray">低创作品，大佬轻喷</span> )</p>')
lines.append('<p style="margin-top:1.5rem">\u2190 左侧导航栏点击查看各功能详情</p>')

lines.append('<h2 class="doc-heading--secondary">兼容性说明</h2>')
lines.append('<p>需保证游戏版本为 YR 1.001。</p>')
lines.append('<p><strong>不依赖 Ares 或 Phobos，可单独使用。</strong></p>')
lines.append('<p>本 DLL 已避免使用 <code class="doc-code--inline">ExtPointerOffset</code> ，改用独立 <code class="doc-code--inline">unordered_map</code> 存储扩展数据。</p>')

lines.append('<h2 class="doc-heading--secondary">已知问题</h2>')
lines.append('<p>1. 使用 <code class="doc-code--inline">Temporal=yes</code> 和 <code class="doc-code--inline">Temporal.Exclusive=yes</code> 时会强制改变索敌逻辑，可能产生bug。</p>')
lines.append('<p>2. <code class="doc-code--inline">LegalTargetWhenAIOwner=no</code> 时其他单位不会攻击它。</p>')
lines.append('<p>3. 基地节点跨所属方：不要让 AI 造围墙，与触发动作 30 不兼容。</p>')
lines.append('<p>4. 超时空武器反复攻击同一目标时可能出现中间目标未冻结的情况。</p>')
lines.append('<br><p><strong>如果遇到bug可以我发私信, 我会尽量解决。</strong></p>')
lines.append('<p>我的 <a href="https://space.bilibili.com/423792550" target="_blank" class="doc-link">@B站主页</a> | QQ邮箱: 3071564490@qq.com</p>')

lines.append('<h2 class="doc-heading--secondary">致谢</h2>')
lines.append('<p>Ares / Phobos / YRpp 项目组 | 韩大妈 / 九千天华 / 偏微whyffu @B站 | 妖妖酱 @GitHub</p>')
lines.append('<p style="color:#666">排名不分先后</p>')

lines.append('<h2 class="doc-heading--secondary">许可</h2>')
lines.append('<p><a href="https://github.com/Chang-zhi/PhobosExt_Changzhi" target="_blank" class="doc-link">GitHub 仓库</a></p>')
lines.append('<p>GPL-3.0 license。游戏素材归 Electronic Arts 所有。</p>')

lines.append('<h2 class="doc-heading--secondary">触发编辑器配置</h2>')
lines.append('<p>复制 <code class="doc-code--inline">FAData_TriggerAndScript.ini</code> 到地编根目录。仅支持 FA2SP_HDM_Edition (韩大妈版)。</p>')

lines.append('<footer class="doc-footer"><p>PhobosExt &mdash; YR 扩展插件 &middot; <a href="https://github.com/Chang-zhi/PhobosExt_Changzhi" target="_blank" class="doc-link">GitHub</a></p></footer>')
lines.append('</section>')

# Feature sections (1-14)
for i in range(1, 15):
    lines.append(f'<section id="sec_{i}">')
    lines.append(f'<h2 class="doc-heading--secondary">{FEATURE_TITLES[i]}</h2>')
    lines.append('<p>内容维护中，详情请参考原始说明文档。</p>')
    lines.append('</section>')

lines.append('</div>')
lines.append(f'<script>{JS_CODE}</script>')
lines.append('</body></html>')

with open(path, 'w', encoding='utf-8') as f:
    f.write('\n'.join(lines))
print(f"Done! {len(lines)} lines written")
