# Phobos-Mix 项目长期记忆

## 发布体系（2026-09-07 建立）

- 发布工具：`scripts/release_pack.py`（check/mergeline/prepare/build/pack/finish/all，支持 --dry-run）+ `.workbuddy/skills/phobos-release/SKILL.md`（agent 编排）
- 版本号唯一定义点 `src/Phobos.version.h`：`VERSION_EX_PATCH` = RecyaN；`PRERELEASE_SUFFIX` 是 upstream alpha 标记，发布时不碰
- 发布包 = `Release/Phobos.dll + Phobos.pdb + 整合包说明/ 内容平铺`（11 条目），GBK 文件名 zip（老 Windows 兼容），输出 `Release/Phobos-v{版本}-Recya{N}.zip`
- `更新改动说明.md` 占位小节循环：`### YYYY.X.XX` 占位 → 发布日回填日期 + EX_PATCH 递增 → 打包 → 开下一轮占位
- changelog 的 upstream 合并行由 git 机械提取（merge commit 日期 + 第二父 hash），自家功能条目才需要语义起草
- 发布节奏由用户决定；finish 自动 commit + tag `v{版本}-Recya{N}`
- Python 3.13 zipfile 写 GBK 名 zip 的坑：monkeypatch `_encodeFilenameFlags` 必须显式清 0x800 位（`_open_to_write` 无条件置位），见 release_pack.py 注释
- `.workbuddy/`（记忆/skill）保持未跟踪：不入库、不加 .gitignore，原来怎样就怎样（用户 2026-09-07 明确）
- **Phobos.props 原样不动**：发布 dll 版本戳保持原生构建形态（带 git 戳 `@ commit-dirty @ ref`），不加 SkipGitStamp 之类构建开关（用户明确要求 props"原来怎样现在就怎样"，曾短暂加过后于 3e31765a8 前 amend 移除）

## 用户工作流约定

- review-then-confirm：方案先量化拆解，确认后实施
- 评估工程方案用量化指标（hook 数/入口点数/行数/机械步骤 vs 语义步骤）
