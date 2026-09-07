---
name: phobos-release
description: Phobos-Mix 整合包发布流程。当用户要发布新版本（Recya 版本）、打发布包、为更新改动说明的占位小节补充条目、回填发布日期时使用。覆盖 changelog 起草、upstream 合并行提取、版本递增、Release 构建、zip 打包、commit + tag 全流程。
---

# Phobos-Mix 整合包发布流程

## 核心约定（结构不变量，全部由脚本保证，agent 不要手改）

- **版本号唯一定义点**：`src/Phobos.version.h` 的 `VERSION_EX_PATCH` = RecyaN。`version.rc` 等全引用宏。
- **zip 名**：`Phobos-v{MAJOR.MINOR.REVISION.PATCH}-Recya{EX_PATCH}.zip`，输出到 `Release/`。
- **发布包结构**（11 条目，对齐参考包 Phobos-v0.5.0.0-Recya1.zip）：`Phobos.dll` + `Phobos.pdb` + `整合包说明/` 内容平铺（底部选择栏模板/ 5 个 shp + 抛体案例参考.ini + 更新改动说明.md + 额外功能说明.md）。GBK 文件名、无 UTF-8 flag、目录条目 stored——脚本已复刻。
- **占位小节循环**：`整合包说明/更新改动说明.md` 里 `### YYYY.X.XX  \`Phobos v旧\` -> \`Phobos v新\`` 是开发期占位小节；发布日回填日期，打包后由脚本开下一轮占位小节。
- **不碰 `PRERELEASE_SUFFIX`**——那是 upstream 的 alpha 标记，与 Recya 版本无关。
- zip 内文档必须是"日期已回填、无占位小节"的快照，因此顺序固定：prepare → build → pack → finish。

## 工具：scripts/release_pack.py

```
python scripts/release_pack.py check                 # 状态总览：版本/占位小节/产物/git
python scripts/release_pack.py mergeline --insert    # upstream 合并行（幂等，自动从 git 提取日期+hash）
python scripts/release_pack.py prepare [--dry-run]   # 校验占位小节 + EX_PATCH 递增 + 日期回填
python scripts/release_pack.py build                 # Release 构建，校验 dll 版本戳
python scripts/release_pack.py pack [--dry-run]      # 组装 zip
python scripts/release_pack.py finish [--dry-run] [--extra 文件...]  # 开新占位小节 + commit + tag
python scripts/release_pack.py all [--dry-run]       # 一条龙（review 场景不要用，分步跑）
```

## 构建说明

- 发布构建（`build` 子命令）与日常开发构建同路径（`build_release.bat`），产物带 git 戳
  （`v0.5.0.0-RecyaN @ commit-dirty @ refs/heads/...`）。Phobos.props 保持原样，不加任何构建开关。
- 构建环境坑：Git Bash + 代理下环境块可能同时含 `HTTPS_PROXY`/`https_proxy`（仅大小写不同），
  MSBuild 会崩 MSB6001——脚本已用 `_msbuild_safe_env()` 去重，无需手动处理。

## Agent 的语义工作（脚本干不了的）

1. **自家功能的 changelog 条目起草**：`git log --first-parent` 取上次发布以来的非 merge 提交，提炼成中文用户向条目，写入占位小节，格式对齐现有条目（`> - \`( N )\` 描述`，带 INI 变化的标注 `rulesmd.ini` 标签名）。起草后必须给用户 review，确认再写入。
2. **upstream 合并行**：不要手写，跑 `mergeline --insert`（数据 = 最新 upstream/develop merge commit 的日期与第二父 hash，格式 `> - \`( 0 )\` 合并 Phobos-develop 近期所有改动（*{日期} - \`commit {hash}\`*）`）。
3. **"目前已知问题"小节维护**：本次修复的 bug 删行，新发现的问题加行（`( N )` 标记沿用文件内既有语义）。
4. **额外功能说明.md**：若本轮有新功能/新 INI 标签，先更新该文档，`finish` 时用 `--extra "整合包说明/额外功能说明.md"` 带进 commit。

## 发布日流程（review-then-confirm，全程逐步确认）

1. `python scripts/release_pack.py check` —— 汇报状态给用户。
2. 检查占位小节：缺合并行 → `mergeline --insert`；缺自家条目 → 起草并给用户 review 后写入。
3. `python scripts/release_pack.py prepare --dry-run` —— 把输出 diff 给用户确认，再去掉 `--dry-run` 实跑。
4. `python scripts/release_pack.py build` —— 后台运行（全量编译需数分钟），完成后检查版本戳校验是否通过。
5. `python scripts/release_pack.py pack` —— 展示条目清单。
6. `python scripts/release_pack.py finish --dry-run`（如有额外文件加 `--extra`）—— 确认后实跑（commit + tag `v{版本}-Recya{N}`）。
7. 用 present_files 展示 `Release/Phobos-v{版本}-Recya{N}.zip`。

## 边界

- **发布节奏由用户决定**，agent 只在被明确要求发布时走流程，平时不主动触发。
- 任何一步脚本报错就停下向用户报告，不要绕过脚本手工补救（比如手改 version.h、手工打 zip）。
- 上游同步（merge upstream/develop）是独立操作，发生在开发期，不属于发布流程；但每次 merge 之后应提醒用户：下次发布会由 mergeline 自动记录。
