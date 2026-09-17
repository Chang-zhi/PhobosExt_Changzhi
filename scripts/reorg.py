# -*- coding: utf-8 -*-
"""Reorganize 说明文档.html into categorized sections."""
import re

path = r"d:\Documents\Git\Phobos-build-48-independentDll\说明文档.html"
with open(path, 'r', encoding='utf-8') as f:
    html = f.read()

# Extract nav items and sections
# Current nav items: <li class="nav__item ..." page="home|1|2|...|14">
# Current sections: <section id="sec_home|sec_1|...|sec_14">

# Category definitions: (page_id, nav_label, section_ids)
categories = [
    ("actions", "触发器新动作", [1, 2, 3, 4, 6, 7, 11]),
    ("events", "触发器新事件", [5, 13]),
    ("gameplay", "游戏扩展机制", [8, 9, 10, 12, 14]),
]

# Feature title mapping for sub-TOC
feature_titles = {
    1: "路径点文本显示", 2: "动态标签绑定功能", 3: "金钱操作功能",
    4: "基地节点操作功能", 5: "科技类型坐标检测", 6: "单位可招募属性设置",
    7: "更新建筑动画功能", 8: "超时空武器扩展", 9: "自动游猎",
    10: "AI 所属单位目标判定", 11: "作战小队操作",
    12: "基地节点跨所属方判定", 13: "流逝时间检测", 14: "混乱机制",
}

# 1. Replace nav items
new_nav = ""
for page_id, label, _ in categories:
    new_nav += f'<li class="nav__item" page="{page_id}">{label}</li>\n'
new_nav += '<li class="nav__item" page="home">首页</li>\n'

# Extract existing sections content
sections_content = {}
for i in range(1, 15):
    pattern = rf'<section id="sec_{i}">(.*?)</section>'
    m = re.search(pattern, html, re.DOTALL)
    if m:
        sections_content[i] = m.group(1).strip()
    else:
        print(f"WARNING: Section {i} not found!")
        sections_content[i] = f"<h2>{feature_titles[i]}</h2><p>内容丢失</p>"

# Build new combined sections
new_sections = ""
for page_id, label, ids in categories:
    new_sections += f'<section id="sec_{page_id}">\n'
    new_sections += f'<h2 class="doc-heading--secondary">{label}</h2>\n'
    # Mini TOC
    new_sections += '<div style="background:#fdf7f7;border-left:3px solid #d92929;border-radius:3px;padding:12px 16px;margin:1rem 0">\n'
    new_sections += '<p style="color:#d92929;font-weight:600;margin-bottom:8px">目录：</p>\n'
    new_sections += '<ul style="list-style:none;padding:0;margin:0;line-height:1.8">\n'
    for fid in ids:
        new_sections += f'<li style="margin:4px 0"><a href="javascript:scrollToFeature(\'{page_id}_{fid}\')" style="color:#d92929;text-decoration:none;padding:2px 6px;border-radius:2px">{fid}. {feature_titles[fid]}</a></li>\n'
    new_sections += '</ul>\n</div>\n'
    # Feature content
    for fid in ids:
        content = sections_content[fid]
        # Replace h2 with h3 for features inside combined sections
        content = content.replace('<h2 class="doc-heading--secondary">', '<h3 class="doc-heading--tertiary" style="margin-top:2em">')
        content = content.replace('</h2>', '</h3>')
        new_sections += f'<div id="{page_id}_{fid}">\n{content}\n</div>\n'
    new_sections += '</section>\n'

# Build new nav HTML
new_nav_html = '<ul class="nav__list">\n'
new_nav_html += '<li class="nav__item nav__item--active" page="home">首页</li>\n'
for page_id, label, _ in categories:
    new_nav_html += f'<li class="nav__item" page="{page_id}">{label}</li>\n'
new_nav_html += '</ul>'

# Replace nav list
html = re.sub(
    r'<ul class="nav__list">.*?</ul>',
    new_nav_html,
    html,
    flags=re.DOTALL
)

# Remove all existing sec_1..sec_14 sections and their content
for i in range(1, 15):
    html = re.sub(
        rf'<section id="sec_{i}">.*?</section>\n*',
        '',
        html,
        flags=re.DOTALL
    )

# Insert combined sections before the script tag
html = re.sub(
    r'(</div>\s*<script>)',
    new_sections + r'\1',
    html,
    flags=re.DOTALL
)

# Add scrollToFeature function to JS
scroll_js = """
function scrollToFeature(id){var el=document.getElementById(id);if(el){el.scrollIntoView({behavior:'smooth',block:'start'});el.style.background='#fff9f9';setTimeout(function(){el.style.background='transparent'},2000)}}
"""
# Insert before the first existing function
html = html.replace(
    'function switchPage(p)',
    scroll_js + 'function switchPage(p)'
)

with open(path, 'w', encoding='utf-8') as f:
    f.write(html)

print("Reorganization complete!")
print(f"Categories: {len(categories)}")
for pid, label, ids in categories:
    print(f"  {pid}: {label} ({len(ids)} features: {ids})")
