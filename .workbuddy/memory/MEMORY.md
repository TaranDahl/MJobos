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
- `.workbuddy/`（记忆/skill）保持未跟踪：不入库、不加 .gitignore，原来怎样就怎样（用户 2026-09-07 明确）
- **Phobos.props 原样不动**：发布 dll 版本戳保持原生构建形态（带 git 戳），不加任何构建开关
- 更新改动说明.md：CRLF、无 BOM；version.h：CRLF、有 BOM——脚本按原样保留

## 用户工作流约定

- review-then-confirm：方案先量化拆解，确认后实施
- 评估工程方案用量化指标（hook 数/入口点数/行数/机械步骤 vs 语义步骤）
- 对上游工程文件的侵入极度敏感：加东西前先问（"原来怎样现在就怎样"）
- 宏/版本号语义不能只看机械关联（拼接处）推断，要核实注释语义与 upstream 原型——EX_PATCH 教训（2026-09-07）
