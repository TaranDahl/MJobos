#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Phobos-Mix 整合包发布打包工具。

发布包结构（对齐 Phobos-v0.5.0.0-Recya1.zip）:
    Phobos.dll / Phobos.pdb      <- Release 构建产物
    整合包说明/ 目录内容平铺      <- 底部选择栏模板/ + 抛体案例参考.ini + 两个说明 md

子命令（按发布日顺序）:
    check      查看状态: version.h 状态、占位小节、upstream 合并行、构建产物、git 状态
    mergeline  生成 upstream 合并 changelog 行（--insert 幂等插入占位小节）
    prepare    校验占位小节 -> 回填发布日期（--version RecyaN）
    build      调 scripts/build_release.bat 构建 Release 并校验 dll 版本戳
    pack       组装 zip 到 Release/Phobos-v{版本}-Recya{N}.zip（--version RecyaN）
    finish     开下一轮占位小节 -> git commit -> git tag（--version RecyaN）
    all        prepare -> build -> pack -> finish 一条龙（--version RecyaN）

约定:
    - VERSION_EX_PATCH 是 mix 本地宏，语义为"upstream merge 相关状态"，发布流程不碰它；
      version.h 的所有宏跟随 upstream/merge 演进，Recya 发布序列与之无关
    - RecyaN 发布号发布时用 --version RecyaN 手动指定（接受 "3" 或 "Recya3"）
    - zip 名 = Phobos-v{MAJOR.MINOR.REV.PATCH}-Recya{N}.zip；tag = v{...}-Recya{N}
    - 整合包说明/更新改动说明.md 用 `### YYYY.X.XX` 占位小节循环:
      开发期攒条目 -> 发布日回填日期 -> 打包 -> 开下一轮占位小节
      占位小节标题里的目标版本（`RecyaN -> RecyaN+1`）需与 --version 一致
    - zip 内文档必须是"日期已回填、无占位小节"的快照, 故 pack 必须在 prepare 之后、finish 之前
    - 不碰 PRERELEASE_SUFFIX（upstream 的 alpha 标记与 Recya 版本无关）

写操作均支持 --dry-run。
"""

import argparse
import datetime
import os
import re
import subprocess
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
VERSION_H = ROOT / 'src' / 'Phobos.version.h'
CHANGELOG = ROOT / '整合包说明' / '更新改动说明.md'
DOCS_DIR = ROOT / '整合包说明'
BUILD_SCRIPT = ROOT / 'scripts' / 'build_release.bat'
RELEASE_DIR = ROOT / 'Release'

PLACEHOLDER_HEAD = re.compile(r'^### \d{4}\.X\.XX\b')
VERSIONED_HEAD = re.compile(r'^### \d{4}\.\d')


def die(msg):
    print(f'错误: {msg}', file=sys.stderr)
    raise SystemExit(1)


def info(msg):
    print(msg)


def disp(p):
    try:
        return p.relative_to(ROOT)
    except ValueError:
        return p


def git(*args, check=True):
    r = subprocess.run(['git', *args], cwd=ROOT, capture_output=True,
                       text=True, encoding='utf-8', errors='replace')
    if check and r.returncode != 0:
        die(f"git {' '.join(args)} 失败: {r.stderr.strip()}")
    return r.stdout.strip()


# ---------- version.h ----------

def read_version():
    text = VERSION_H.read_bytes().decode('utf-8-sig')
    v = {}
    for key in ('MAJOR', 'MINOR', 'REVISION', 'PATCH', 'EX_PATCH'):
        m = re.search(rf'#define VERSION_{key} (\d+)', text)
        if not m:
            die(f'Phobos.version.h 中找不到 VERSION_{key}')
        v[key] = int(m.group(1))
    m = re.search(r'#define PRERELEASE_SUFFIX "([^"]*)"', text)
    v['PRERELEASE'] = m.group(1) if m else ''
    return v


def long_str(v):
    return f"{v['MAJOR']}.{v['MINOR']}.{v['REVISION']}.{v['PATCH']}"


def recya(v):
    """version.h 内嵌的 Recya 串（EX_PATCH = merge 状态），仅用于展示/构建校验，
    与 RecyaN 发布号无关。"""
    return f"v{long_str(v)}-Recya{v['EX_PATCH']}"


def parse_release_number(s):
    """--version 参数: 接受 "3" 或 "Recya3"（大小写不限），返回 int。"""
    m = re.fullmatch(r'(?:recya)?(\d+)', s.strip(), re.I)
    if not m:
        die(f'版本号格式无法解析: {s!r}（期望 3 或 Recya3）')
    return int(m.group(1))


def zip_name(long, n):
    return f"Phobos-v{long}-Recya{n}.zip"


# ---------- 更新改动说明.md ----------

def load_changelog():
    return CHANGELOG.read_bytes().decode('utf-8-sig').replace('\r\n', '\n')


def save_changelog(text):
    raw = CHANGELOG.read_bytes()
    bom = raw.startswith(b'\xef\xbb\xbf')
    if b'\r\n' in raw:
        text = text.replace('\n', '\r\n')
    data = text.encode('utf-8')
    if bom:
        data = b'\xef\xbb\xbf' + data
    CHANGELOG.write_bytes(data)


def find_placeholder(lines):
    for i, l in enumerate(lines):
        if PLACEHOLDER_HEAD.match(l):
            return i
    return -1


def section_end(lines, start):
    for j in range(start + 1, len(lines)):
        if lines[j].startswith('### ') or lines[j].startswith('## '):
            return j
    return len(lines)


def placeholder_entries(lines, i):
    end = section_end(lines, i)
    return [l for l in lines[i + 1:end] if l.strip()]


def strip_placeholder_sections(text):
    """剔除所有 `### YYYY.X.XX` 占位小节（含其内容与其标题前的空行），
    返回可直接作为 zip 快照的文本。任何时间点打包都能产出干净快照。"""
    out = []
    skip = False
    for l in text.split('\n'):
        if PLACEHOLDER_HEAD.match(l):
            skip = True
            while out and out[-1].strip() == '':
                out.pop()
            continue
        if skip:
            if l.startswith('### ') or l.startswith('## '):
                skip = False
                out.append('')
                out.append(l)
            continue
        out.append(l)
    return '\n'.join(out)


# ---------- 子命令 ----------

def cmd_check(args):
    v = read_version()
    lines = load_changelog().split('\n')
    info(f'version.h   : {long_str(v)}-Recya{v["EX_PATCH"]} (EX_PATCH=merge 状态, 发布号无关; PRERELEASE_SUFFIX={v["PRERELEASE"]!r})')
    i = find_placeholder(lines)
    if i >= 0:
        entries = placeholder_entries(lines, i)
        info(f'占位小节   : {lines[i].strip()}')
        info(f'             条目 {len(entries)} 条' + ('  [!] 占位小节为空，发布前需补充 changelog' if not entries else ''))
        m = re.search(r'->\s*`Phobos v([\w.\-]+)`', lines[i])
        if m:
            info(f'             本次发布目标: {m.group(1)}（发布时用 --version 指定）')
        if not any('commit ' in e for e in entries):
            info('             尚未记录 upstream 合并行，可运行: release_pack.py mergeline --insert')
    else:
        info('占位小节   : 不存在（可能已 prepare，或未开下一轮占位小节）')
    dll, pdb = RELEASE_DIR / 'Phobos.dll', RELEASE_DIR / 'Phobos.pdb'
    for f in (dll, pdb):
        info(f'构建产物   : {disp(f)} ' + (f'存在 ({datetime.datetime.fromtimestamp(f.stat().st_mtime):%Y-%m-%d %H:%M})' if f.exists() else '缺失'))
    status = git('status', '--porcelain')
    info(f'git        : 分支 {git("rev-parse", "--abbrev-ref", "HEAD")}, ' + ('工作树干净' if not status else f'{len(status.splitlines())} 个未提交改动'))


def merge_commit_info():
    log = git('log', '--first-parent', '--merges', '-30', '--format=%H%x1f%cI%x1f%s')
    for line in log.splitlines():
        h, date_iso, subject = line.split('\x1f', 2)
        if 'upstream/develop' in subject:
            d = datetime.datetime.fromisoformat(date_iso)
            parent2 = git('rev-parse', f'{h}^2')
            return f"{d.year}.{d.month}.{d.day}", parent2
    return None, None


def cmd_mergeline(args):
    date, parent2 = merge_commit_info()
    if not parent2:
        die('最近 30 个合并提交中没有 upstream/develop 合并')
    line = f'> - `( 0 )` 合并 Phobos-develop 近期所有改动（*{date} - `commit {parent2}`*）'
    content = load_changelog()
    if parent2 in content:
        info('该 upstream 合并已记录在 changelog 中，跳过：')
        info(f'  {line}')
        return
    if not args.insert:
        info('建议插入占位小节（加 --insert 写入）：')
        info(f'  {line}')
        return
    lines = content.split('\n')
    i = find_placeholder(lines)
    if i < 0:
        die('占位小节不存在，先手动或由 agent 开一轮占位小节再插入')
    end = section_end(lines, i)
    j = i + 1
    while j < end and not lines[j].strip():
        j += 1
    lines.insert(j, line)
    save_changelog('\n'.join(lines))
    info(f'已插入合并行到占位小节（{lines[i].strip()}）：')
    info(f'  {line}')


def cmd_prepare(args):
    n = parse_release_number(args.version)
    v = read_version()
    lines = load_changelog().split('\n')
    i = find_placeholder(lines)
    if i < 0:
        die('占位小节 ### YYYY.X.XX 不存在：要么已 prepare 过，要么还没为下一轮开占位小节')
    entries = placeholder_entries(lines, i)
    if not entries:
        die('占位小节没有条目，先补充 changelog（mergeline --insert / 手写 / agent 起草）')
    m = re.match(r'^### \d{4}\.X\.XX\s+`Phobos v([\w.\-]+)`\s*->\s*`Phobos v([\w.\-]+)`\s*$', lines[i])
    if not m:
        die(f'占位小节标题格式无法解析: {lines[i].strip()!r}\n期望: ### YYYY.X.XX  `Phobos v旧` -> `Phobos v新`')
    expect_next = f"{long_str(v)}-Recya{n}"
    if m.group(2) != expect_next:
        die(f"占位小节目标版本 {m.group(2)} 与 --version 指定的 {expect_next} 不一致；"
            f"请核对占位小节标题或 --version 参数")
    today = datetime.date.today()
    new_head = re.sub(r'^### \d{4}\.X\.XX', f'### {today.year}.{today.month}.{today.day}', lines[i])
    info(f'发布 {m.group(1)} -> {m.group(2)}，日期 {today.year}.{today.month}.{today.day}')
    info(f'  回填: {lines[i].strip()}')
    info(f'   ->   {new_head.strip()}')
    if args.dry_run:
        info('[dry-run] 未写任何文件')
        return
    lines[i] = new_head
    save_changelog('\n'.join(lines))
    info('已写回: 整合包说明/更新改动说明.md（日期）。version.h 不参与发布，未改动。')


def _msbuild_safe_env():
    """Windows 环境块可同时含仅大小写不同的键（如 HTTPS_PROXY / https_proxy，
    常见于 Git Bash + 代理组合）。MSBuild 用大小写不敏感的 Hashtable 向工具
    进程传环境时遇到重复键会崩（MSB6001 已添加项）。os.environ 在 Windows 上
    是大小写不敏感的合并视图，用它重建环境块即可去重。"""
    return dict(os.environ)


def cmd_build(args):
    if not BUILD_SCRIPT.exists():
        die(f'找不到 {BUILD_SCRIPT}')
    v = read_version()
    info(f'开始 Release 构建（version.h 内嵌 {recya(v)}，与发布号无关，输出透传，可能需要几分钟）...')
    r = subprocess.run([str(BUILD_SCRIPT)], cwd=ROOT, env=_msbuild_safe_env())
    if r.returncode != 0:
        die(f'构建失败，退出码 {r.returncode}')
    dll, pdb = RELEASE_DIR / 'Phobos.dll', RELEASE_DIR / 'Phobos.pdb'
    for f in (dll, pdb):
        if not f.exists():
            die(f'构建产物缺失: {f}')
    pv = subprocess.run(
        ['powershell', '-NoProfile', '-Command',
         f"(Get-Item -LiteralPath '{dll}').VersionInfo.ProductVersion"],
        capture_output=True, text=True).stdout.strip()
    expect = recya(v)
    if pv and not pv.startswith(expect):
        die(f'dll 版本戳 {pv!r} 与 version.h 期望 {expect!r} 不符 —— version.h 是否在构建后又变动？')
    info(f'构建完成: {disp(dll)} / {disp(pdb)}  (ProductVersion={pv})')


def _gbk_zip_patch():
    """让 zipfile 以 GBK 编码写中文文件名且不设 UTF-8 flag，复刻参考包
    （Phobos-v0.5.0.0-Recya1.zip 是 Windows 资源管理器/WinRAR 式 GBK 名 zip，
    这样老版本 Windows 内置解压也不会乱码）。返回恢复函数。

    注意 Python 3.13 起 ZipFile._open_to_write 会无条件把 flag_bits 置成
    _MASK_UTF_FILENAME，因此这里必须显式清位而不是原样返回。"""
    orig = zipfile.ZipInfo._encodeFilenameFlags

    def gbk(self):
        if isinstance(self.filename, str):
            try:
                return self.filename.encode('gbk'), self.flag_bits & ~0x800
            except UnicodeEncodeError:
                return self.filename.encode('utf-8'), self.flag_bits | 0x800
        return self.filename, self.flag_bits

    zipfile.ZipInfo._encodeFilenameFlags = gbk
    return lambda: setattr(zipfile.ZipInfo, '_encodeFilenameFlags', orig)


def cmd_pack(args):
    n = parse_release_number(args.version)
    v = read_version()
    lines = load_changelog().split('\n')
    # 快照校验：不允许存在"指向本次发布版本"的占位小节（说明本次还没 prepare）。
    # 下一轮的占位小节（目标版本更新的）无妨，打包时会自动剔除。
    this_target = f'{long_str(v)}-Recya{n}'
    for l in lines:
        if PLACEHOLDER_HEAD.match(l):
            m = re.search(r'->\s*`Phobos v([\w.\-]+)`', l)
            if m and m.group(1) == this_target:
                die(f'更新改动说明.md 仍含指向本次发布的占位小节（{this_target}），'
                    f'zip 快照必须先回填日期（先跑 prepare --version {n}）')
    dll, pdb = RELEASE_DIR / 'Phobos.dll', RELEASE_DIR / 'Phobos.pdb'
    for f in (dll, pdb):
        if not f.exists():
            die(f'构建产物缺失: {f}（先跑 build）')
    # 更新改动说明.md 的 zip 快照：剔除占位小节后按原 BOM/CRLF 习惯编码
    raw = CHANGELOG.read_bytes()
    bom = raw.startswith(b'\xef\xbb\xbf')
    crlf = b'\r\n' in raw
    snap_text = strip_placeholder_sections(raw.decode('utf-8-sig').replace('\r\n', '\n'))
    if crlf:
        snap_text = snap_text.replace('\n', '\r\n')
    snap = snap_text.encode('utf-8')
    if bom:
        snap = b'\xef\xbb\xbf' + snap
    # 条目清单：dll/pdb 在前，整合包说明/ 内容平铺（子目录写 stored 目录条目，同参考包）
    items = [('file', dll, 'Phobos.dll'), ('file', pdb, 'Phobos.pdb')]
    seen_dirs = set()

    def add_file(src, arc):
        parts = arc.split('/')
        for k in range(1, len(parts)):
            d = '/'.join(parts[:k]) + '/'
            if d not in seen_dirs:
                seen_dirs.add(d)
                items.append(('dir', None, d))
        items.append(('file', src, arc))

    for p in sorted(DOCS_DIR.rglob('*')):
        if p.is_file():
            add_file(p, p.relative_to(DOCS_DIR).as_posix())
    out = RELEASE_DIR / zip_name(long_str(v), n)
    if args.dry_run:
        info(f'[dry-run] 将生成 {disp(out)}:')
        for kind, _, arc in items:
            info(f'  {arc}' + ('' if kind == 'file' else '   (目录条目)'))
        return
    restore = _gbk_zip_patch()
    try:
        with zipfile.ZipFile(out, 'w', zipfile.ZIP_DEFLATED) as z:
            for kind, src, arc in items:
                if kind == 'dir':
                    zi = zipfile.ZipInfo(arc)
                    zi.external_attr = 0x10
                    z.writestr(zi, '', compress_type=zipfile.ZIP_STORED)
                elif src == CHANGELOG:
                    zi = zipfile.ZipInfo(arc)
                    zi.compress_type = zipfile.ZIP_DEFLATED
                    z.writestr(zi, snap)
                else:
                    z.write(src, arc)
    finally:
        restore()
    info(f'已生成 {disp(out)} ({out.stat().st_size / 1024:.0f} KB), 共 {len(items)} 个条目:')
    for kind, _, arc in items:
        info(f'  {arc}' + ('' if kind == 'file' else '   (目录条目)'))


def cmd_finish(args):
    n = parse_release_number(args.version)
    v = read_version()
    lines = load_changelog().split('\n')
    if find_placeholder(lines) >= 0:
        die('仍存在 X.XX 占位小节，说明尚未 prepare —— finish 只在发布完成后开下一轮占位小节')
    zip_path = RELEASE_DIR / zip_name(long_str(v), n)
    if not zip_path.exists():
        die(f'发布包不存在: {zip_path}（先跑 pack）')
    d = datetime.date.today()
    old, new = f"{long_str(v)}-Recya{n}", f"{long_str(v)}-Recya{n + 1}"
    head = f'### {d.year}.X.XX  `Phobos v{old}` -> `Phobos v{new}`'
    insert_at = next((k for k, l in enumerate(lines) if VERSIONED_HEAD.match(l)), None)
    if insert_at is None:
        die('找不到任何 ### YYYY.M.D 版本小节，无法定位插入点')
    files = [CHANGELOG, *args.extra]
    git_cmds = [
        ['git', 'add', '--', *(str(f.relative_to(ROOT)) for f in files)],
        ['git', 'commit', '-m', f'Release Phobos v{old}'],
        ['git', 'tag', '-a', f'v{old}', '-m', f'Phobos v{old}'],
    ]
    info(f'新占位小节: {head}')
    for c in git_cmds:
        info(f'  $ {" ".join(c)}')
    if args.dry_run:
        info('[dry-run] 未写文件、未执行 git')
        return
    lines[insert_at:insert_at] = [head, '']
    save_changelog('\n'.join(lines))
    info('已写回: 整合包说明/更新改动说明.md（新占位小节）')
    for c in git_cmds:
        r = subprocess.run(c, cwd=ROOT, capture_output=True, text=True, encoding='utf-8', errors='replace')
        if r.returncode != 0:
            die(f"{' '.join(c)} 失败: {r.stderr.strip()}")
        if r.stdout.strip():
            info(f'  {r.stdout.strip().splitlines()[0]}')
    info(f'发布完成: {disp(zip_path)}，tag v{old}')


def cmd_all(args):
    cmd_prepare(args)
    if args.dry_run:
        info('[dry-run] 跳过 build，后续仅打印计划')
        cmd_pack(args)
        cmd_finish(args)
        return
    cmd_build(args)
    cmd_pack(args)
    cmd_finish(args)


def main():
    if sys.stdout.encoding and sys.stdout.encoding.lower() != 'utf-8':
        try:
            sys.stdout.reconfigure(encoding='utf-8', errors='replace')
            sys.stderr.reconfigure(encoding='utf-8', errors='replace')
        except Exception:
            pass
    ap = argparse.ArgumentParser(description='Phobos-Mix 整合包发布打包工具')
    sub = ap.add_subparsers(dest='cmd', required=True)
    sub.add_parser('check', help='查看版本/占位小节/构建产物/git 状态')
    p = sub.add_parser('mergeline', help='生成 upstream 合并 changelog 行')
    p.add_argument('--insert', action='store_true', help='幂等插入占位小节')
    for name, help_ in [('prepare', '日期回填（不碰 version.h）'), ('build', 'Release 构建'),
                        ('pack', '组装 zip'), ('finish', '开新占位小节 + commit + tag'),
                        ('all', 'prepare->build->pack->finish')]:
        p = sub.add_parser(name, help=help_)
        p.add_argument('--dry-run', action='store_true', help='只打印将执行的动作')
        if name != 'build':
            p.add_argument('--version', required=True,
                           help='发布号，如 Recya3 或 3')
        if name in ('finish', 'all'):
            p.add_argument('--extra', nargs='*', default=[], type=Path,
                           help='额外加入 commit 的文件（如 额外功能说明.md）')
    args = ap.parse_args()
    {'check': cmd_check, 'mergeline': cmd_mergeline, 'prepare': cmd_prepare,
     'build': cmd_build, 'pack': cmd_pack, 'finish': cmd_finish,
     'all': cmd_all}[args.cmd](args)


if __name__ == '__main__':
    main()
