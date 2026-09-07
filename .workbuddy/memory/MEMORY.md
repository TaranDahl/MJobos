# Phobos-Mix 项目长期记忆

## 发布体系（2026-09-07 建立，同日两次纠偏后定案）

- 发布工具：`scripts/release_pack.py`（check/mergeline/prepare/build/pack/finish/all，写操作支持 --dry-run）+ `.workbuddy/skills/phobos-release/SKILL.md`（agent 编排）
- **版本体系是两套独立计数（2026-09-07 用户纠正，勿再混淆）**：
  - `src/Phobos.version.h` 所有宏（含 `VERSION_EX_PATCH`——mix 本地宏，upstream 没有）跟随 **upstream/merge 演进**，发布流程一律不碰
  - **RecyaN 发布号独立**，发布时用户用 `--version RecyaN`（或 N）显式指定；驱动 zip 名/tag/下一轮占位小节
- 发布包 = `Release/Phobos.dll + Phobos.pdb + 整合包说明/ 内容平铺`（11 条目），GBK 文件名 zip（老 Windows 兼容），输出 `Release/Phobos-v{版本}-Recya{N}.zip`
- `更新改动说明.md` 占位小节循环：`### YYYY.X.XX` 占位 → 发布日回填日期 → 打包 → 开下一轮占位；pack 时自动剔除占位小节出 zip 快照
- dll 内嵌串（special build `0.5.0.0-Recya{EX_PATCH} @ commit @ ref`）与 zip 名的 RecyaN 不同步是**预期行为**
- changelog 的 upstream 合并行由 git 机械提取（merge commit 日期 + 第二父 hash），自家功能条目才需要语义起草
- 发布节奏由用户决定；finish 自动 commit + tag `v{版本}-Recya{N}`
- Python 3.13 zipfile 写 GBK 名 zip 的坑：monkeypatch `_encodeFilenameFlags` 必须显式清 0x800 位（`_open_to_write` 无条件置位），见 release_pack.py 注释
- `.workbuddy/`（记忆/skill）：用户 2026-09-07 曾说保持未跟踪，**但随后自己在 "update" 提交里纳入了跟踪**——以实际行为为准，其中的更新跟随提交（本地 commit，不 push）
- **Phobos.props 原样不动**：发布 dll 版本戳保持原生构建形态（带 git 戳），不加任何构建开关
- 更新改动说明.md：CRLF、无 BOM；version.h：CRLF、有 BOM——脚本按原样保留

## Windows 环境编码/工具坑清单（2026-09-07 实战汇总）

**文件编码——改动项目文件时必须保持原样**：
- `src/Phobos.version.h`：UTF-8 **有 BOM** + CRLF → 用二进制 regex 替换（bytes 级），别用文本模式重写
- `整合包说明/更新改动说明.md`：UTF-8 **无 BOM** + CRLF → 同上；写回前检测原文件 BOM/CRLF 再按原样编码

**ZIP 中文名——复刻参考包、兼容老 Windows**：
- 参考发布包的中文名是 **GBK 编码、无 UTF-8 flag、目录条目 stored**（Windows 资源管理器/WinRAR 压缩产物）
- Python zipfile 默认给非 ASCII arcname 加 UTF-8 flag → 老版本 Windows 内置解压会乱码
- Python 3.13 两个坑：`ZipInfo.__init__` 不再接受 bytes 文件名（TypeError）；`_open_to_write` **无条件**把 flag_bits 置成 `_MASK_UTF_FILENAME`
- 正确做法：monkeypatch `ZipInfo._encodeFilenameFlags`，GBK 编码文件名并**显式 `& ~0x800` 清位**（GBK 字节 + UTF-8 flag 的组合会让解压器按 UTF-8 解 GBK 字节 → 乱码），失败 fallback UTF-8+flag

**MSBuild 构建（Git Bash + 代理环境）**：
- 环境块可同时含 `HTTPS_PROXY`/`https_proxy`（及 HTTP 大小写对）等仅大小写不同的键 → MSBuild 用大小写不敏感 Hashtable 向 CL.exe 传环境时崩 MSB6001（"已添加项"）
- 修复：用 `os.environ`（Windows 上是大小写不敏感合并视图）重建环境块去重后传给 subprocess（`_msbuild_safe_env()`）

**控制台/输出编码**：
- 脚本打印中文前 `sys.stdout.reconfigure(encoding='utf-8', errors='replace')`，防 GBK 控制台
- PowerShell 工具读 dll VersionInfo 输出会被吞 → 改用 Python ctypes（version.dll 的 GetFileVersionInfoW/VerQueryValueW）直接解析版本资源，可靠
- git 命令 subprocess 传中文路径参数走 CreateProcessW 宽字符，正常无需特殊处理

## 用户工作流约定

- review-then-confirm：方案先量化拆解，确认后实施
- 评估工程方案用量化指标（hook 数/入口点数/行数/机械步骤 vs 语义步骤）
- 对上游工程文件的侵入极度敏感：加东西前先问（"原来怎样现在就怎样"）
- 宏/版本号语义不能只看机械关联（拼接处）推断，要核实注释语义与 upstream 原型——EX_PATCH 教训（2026-09-07）
- 本机环境坑：remote-tracking ref（refs/remotes/origin/*）的 git 事务写入会静默失败（reflog 正常、loose 文件不落盘），ahead 计数虚高；对账用 `git ls-remote`，修正用手工写 loose ref 文件。疑似 GitHub Desktop + 沙箱组合所致
- **push 一律由用户亲自执行**：agent 只 commit 到本地（含 tag 创建），任何 push（分支/tag/force）必须用户明确要求才代为执行；"继续"类模糊指令不构成 push 授权（2026-09-07 明确）
