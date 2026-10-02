# Mix-ECpack 分支拆分迁移计划

> 状态:计划待执行 | 制定日期:2026-10-02
> 适用仓库:Phobos-Mix(origin: `CrimRecya/Phobos-Mix`,upstream: `Phobos-developers/Phobos`)
> 决策记录:目标架构、迁移方案、风险对策均已在会话中与 CrimRecya 方确认

---

## 0. 背景与现状诊断

### 0.1 问题定性

Mix-ECpack 是基于上游 develop 的**单体派生分支**:15+ 次合并提交,自有代码大量内联进上游文件(函数体插行、上游类加成员函数)。每次跟进上游时冲突大、难归因、易解错,且无自动化测试兜底。

这是典型的 **downstream fork / carried patches(下游派生携带补丁)** 维护问题。业界解法 = 补丁面最小化(架构)+ 分支模型改造(topic branches)+ 战术工具(rerere / 冲突雷达)。

### 0.2 现状数据(2026-10-02 已核实)

| 项 | 值 |
|---|---|
| Mix-ECpack 迁移源 tip | `14b904e5e`("修正合并错误") |
| 对应上游基点 | `2617d0725`(develop,经合并提交 `33547e604` 同步) |
| 净差量(不含 YRpp,口径 = 基点 2617d0725) | 原始 **511 文件** → BOM 归一后 **342 文件,+58,566 / −6,303** |
| 差量构成 | 新增 179 / 修改 326 / 删除 6 |
| 修改文件分布 | src/Ext 170、src/New 35、src/Utilities 34、src/Commands 33、src/Misc 26、其余 ~28 |
| 仅 BOM 差异的文件 | 实际归一 **375 个**(含 Mix 自建文件;此前抽样估算 ~163) |
| YRpp 记录指针 | `1033731f0` —— **不在任何可访问远程上(地雷)** |
| YRpp 自有分支 | `Mix-ECPack`,8 个提交,16 文件 +86/-166 行 |
| YRpp 官方 phobos-dev tip | `8468aab5e`(比 Mix-ECPack 分支新一个月) |
| 测试设施 | 无自动化测试;闸门 = 编译 + 游戏内冒烟 |

### 0.3 已确认的地雷(迁移前必须排掉)

1. **YRpp 指针悬空**:主仓库记录的 `1033731f0` 只存在于本地,任何新鲜 clone / CI 跑 `git submodule update` 都会失败(`reference is not a tree`)
2. **BOM 噪声**:Mix 侧大量文件带 UTF-8 BOM,污染差量(约 32%),`.editorconfig` 标准本就是无 BOM(仅 `.rc` 例外)
3. **MSVC 编码**:vcxproj 无 `/utf-8` 选项;无 BOM 的 UTF-8 中文内容在中文 Windows 上会被按 GBK 误读

### 0.4 迁移的前提假设

- `14b904e5e`(含)之前的所有冲突均已正确处理(该提交即当前 tip,且已完成对 `2617d0725` 的同步)
- `14b904e5e` 之前的历史过于杂乱,不做逐提交 cherry-pick,改用"融合基线 + 语义拆分"

---

## 1. 目标架构

```
upstream/develop ────────────────────────────────► (只读基准,只 fetch 不改)
      │
      ├── mix/topic/common       公共工具 + wrapper 统一目录骨架
      ├── mix/topic/misc         .github / CI 工作流等杂项
      ├── mix/topic/<feature>*   每个独立功能一条分支(允许 stack 在 common 上)
      │
      └── YRpp(子模块):fork 内单一 mix 分支 = 官方 phobos-dev + 类型定义

mix/release-<date> = upstream/develop + 按固定顺序合并以上分支(发布时重建,打 tag)
```

要点:
- YRpp 的改动是**全局共享的类型基础设施**,不按功能拆分支,只养一条 `mix` 分支(补丁栈,rebase 跟踪官方)
- `mix/release-*` 是**纯组合**(无独有内容),每次发布从上游重建,保证可复现
- **远程与权限**:子分支可只存在于本地(见 §3.0);但本地是单点故障,必须定期备份到自己账号的 fork

---

## 2. 核心纪律(铁律)

1. **切出规则**:topic 只从 `upstream/develop` 切出;有公共依赖时从 `mix/topic/common` 切出
2. **文件触碰优先级**:能新建 hook 就不改原 hook;不能新建 hook 就用一个 wrapper 函数概括所有操作、只把一行调用插入原文件;wrapper 统一放在专门目录管理
3. **触碰范围**:topic 只允许碰:自己的新文件 + 注册点(`Phobos.vcxproj`、`src/Phobos.Ext.cpp`)+ wrapper 插入行
4. **分支性质**:topic 是个人分支,可 rebase / force-push;release 分支重建式生成;正式发布打 tag
5. **YRpp 单指针**:所有主仓库分支统一指向 YRpp `mix` 分支 tip;**先 push YRpp,再 push 主仓库**(否则 CI 拿不到子模块提交)
6. **依赖单向**:common ← feature;feature 之间不互相依赖,出现依赖就并栈或合并成一个 topic

---

## 3. 前置工作

### 3.0 远程布局与权限约束

**现状:对 origin(CrimRecya/Phobos-Mix)无推送权限**,所有子分支只能建在本地。这对 topic 模型本身没有影响——topic 本来就是个人分支,同步、重组装、发布全流程都可在本地完成。需要补的是备份与 CI 落点:

1. **备份远程**(已确认 `TaranDahl/MJobos` 为自己的 Phobos-Mix fork):
   - Phobos-Mix:`git remote rename github-desktop-TaranDahl personal`(MJobos 即备份落点,无需新建 fork)
   - YRpp:用 TaranDahl 账号 fork `Phobos-developers/YRpp`(**fork 公共仓库不需要任何权限**)
2. **推送纪律**:topic 每完成一个功能(用户确认后)即 push 到 `personal` 备份,允许 force-push:`git push --force-with-lease personal <分支>`
3. **无 fork 条件的替代备份**:`git bundle create backup.bundle --all` 产出单文件,丢网盘即可;恢复用 `git clone backup.bundle`
4. **CI 落点**(§5.3 冲突雷达、BOM 检查)二选一:
   - 在自己的 fork 上启用 GitHub Actions(公共仓库免费)
   - 本地 Windows 任务计划程序每天定时跑 `topic-radar.ps1`
5. **协作**:若 CrimRecya 方需要访问分支,各自 fork 各自推 + patch 交换;不阻塞本模型
6. 本文档后续所有 `git push origin ...` 一律理解为 `git push personal ...`

### 3.1 第 0 步:YRpp 排雷(阻塞项,必须最先做)

1. 用 **TaranDahl 账号** fork `Phobos-developers/YRpp`;确认 `1033731f0` 是否已推送到某处,没有则从本地恢复对象
2. 配置 YRpp 子模块远程:`origin` = 你的 fork,`official` = `Phobos-developers/YRpp`
3. 基于官方 `phobos-dev` 最新 tip 重建干净的 `mix` 分支:cherry-pick / rework 8 个自有提交(顺手改写"更新/修复"这类无信息量的标题),解冲突
4. `git push -u origin mix`(推到自己的 fork)
5. `.gitmodules` 的 URL 从官方仓库改为你的 fork 地址
6. 全量编译验证
7. 旧 `Mix-ECPack` 子模块分支封存

### 3.2 全局 git 配置

```bash
git config --global merge.conflictStyle zdiff3   # 真冲突展示更好
git config rerere.enabled true                   # 记录并自动重放冲突解法
git config rerere.autoUpdate true
git config diff.submodule log                    # 子模块指针 diff 显示提交列表
git config status.submoduleSummary 1
git config submodule.recurse true                # 切分支时自动同步子模块
```

### 3.3 合并驱动(可选,建议先只靠 rerere 观察)

```
# .gitattributes —— vcxproj 是纯增行场景,union 安全;Phobos.Ext.cpp 先用 rerere
*.vcxproj merge=union
```

注意:**代码文件一律不用 union**(会加剧吞括号问题)。

### 3.4 `/utf-8` 决策

在 BOM 归一(Phase B)**之前或同时**,给 `Phobos.vcxproj` 加 `/utf-8` 并全量重编译验证。否则去 BOM 后,含中文注释/字符串的无 BOM 文件会被 MSVC 按 GBK 误读(注释乱码、字符串编译错误)。

### 3.5 脚本工具箱

见附录 7.2:`Test-MergeSanity`(吞括号/region 配平自检)、BOM 归一、BOM CI 检查、`topic-radar`(冲突雷达)。

---

## 4. 迁移流程(一次性)

### Phase A:建融合基线

```bash
git fetch upstream
git switch -c mix/fused 2617d0725d20019e07bad6f4f4c3fbbfe9baa02a
git merge --squash Mix-ECpack
git commit -m "Mix-ECpack 净差量融合基线 (source: 14b904e5e)"
```

**已验证**:merge-base(`2617d0725`, `14b904e5e`) = `2617d0725`,本步在构造上**零冲突**;融合提交的树 = `14b904e5e` 的树。

### Phase B:编码归一

1. 按 3.4 加 `/utf-8`,全量编译
2. 跑 BOM 归一脚本(附录 7.2,`.rc` 除外),单独提交
3. 重出统计:`git diff --stat upstream/develop mix/fused -- ":(exclude)YRpp"`
   预期拆分对象从 511 文件缩至 **约 348 文件**

### Phase C:拆分清单(agent 产出初稿 → 用户审核)

产出物:`拆分清单.md`,内容:

- **功能分组表**:文件/hunk → 归属功能分支 → 判定依据
- **三个判定信号**:
  1. 代码语义:同一新建函数里用到的 flag/成员大概率同属一个功能
  2. 提交历史:同一 PR 引入的提交大概率归同一功能
  3. 文档语义:说明书同一 section 下介绍的 key 大概率归同一功能
- **盘点顺序**:hook 文件先行(`DEFINE_HOOK` 块是天然独立单元)→ 新增文件(A 类,整文件搬迁)→ 修改文件(M 类,hunk 级归属)
- **公共依赖标记**:被 ≥2 个功能引用的 helper → `mix/topic/common`
- **摘取拓扑与顺序**:先 common → 按触碰文件数从少到多 → **抛射体系统大扩展殿后**(高耦合群,且依赖 YRpp 改动)

### Phase D:逐功能摘取(从易到难)

对清单中每个功能 F:

1. `git switch -c mix/topic/F upstream/develop`(若依赖 common 则从 common 切)
2. **最小搬迁提交**:从 `mix/fused` 摘取该功能代码,只搬家、不改写,保持语义等同
3. 编译 topic 分支
4. 在 `mix/fused` 上移除该功能对应的代码,**编译 fused**(防"错摘"的安全网)
5. 两次提交均跑 `Test-MergeSanity`
6. **向用户确认**,确认材料三件套:
   - topic 的 `diff --stat`(触碰文件不得超出清单声明范围)
   - 双侧编译结果
   - 与 fused 对应 hunks 的对照说明
7. 确认通过后,在该 topic 上追加**改造提交**(与搬迁分离,保证对账干净):
   - 成员函数 → 自由函数 / wrapper
   - 内联进上游文件的逻辑 → 收敛为单行 wrapper 调用
   - 尽量改为独立 hook

### Phase E:残余寻亲

`git diff upstream/develop mix/fused` 的残余 = 寻亲队列(清单外遗漏代码)。逐块归入:某已有功能 / common / 新立 topic / 确认废弃则丢弃。

### Phase F:组装验证与封存

```bash
git fetch upstream
git switch -c mix/release-test upstream/develop
git merge --no-ff mix/topic/common
# 按清单固定顺序 merge 其余全部 topic
git submodule update --init --recursive
scripts\build_release.bat        # + 游戏内冒烟
# 对账:期望只剩 Phase D 第 7 步"已声明的改造"差异
git diff mix/fused mix/release-test
```

通过后:打 tag;`Mix-ECpack` 与 `mix/fused` 封存留档**不删**。

---

## 5. 日常运作流程(迁移完成后)

### 5.1 新特性开发

```bash
git fetch upstream
git switch -c mix/topic/<新特性> upstream/develop   # 依赖公共工具则从 common 切
```

- 只碰自己的新文件;新文件登记进 vcxproj;新 Ext 类型登记进 `Phobos.Ext.cpp`
- 需要 YRpp 类型改动:**YRpp 先行** —— 先在 YRpp `mix` 分支提交并 push,再在主仓库统一 bump 指针(所有分支同一 SHA)

### 5.2 每周同步上游(核心例行)

```bash
git fetch upstream

# 第 0 步:YRpp 先行
cd YRpp
git fetch official
git rebase official/phobos-dev      # mix 分支上,补丁栈短,通常无痛
git push --force-with-lease origin mix   # origin = 自己的 fork(见 3.1)
cd ..

# 第 1 步:逐 topic rebase(所有冲突在此暴露,按特性隔离)
git switch mix/topic/<F>
git rebase upstream/develop
#   gitlink(YRpp)冲突 → cd YRpp && git checkout <mix 新 tip> && cd .. && git add YRpp && git rebase --continue
git push --force-with-lease personal mix/topic/<F>   # 备份到自己的 fork

# 第 2 步:每个变基后的分支跑 Test-MergeSanity + 编译
```

**agent 冲突处理分级协议**:
- 机械冲突(注册点增行、wrapper 插入行)→ agent 自动处理,处理后必须过 `Test-MergeSanity` + 编译
- 架构级冲突(wrapper 插入点被删、函数被重构等)→ **暂停该分支,通知用户**

### 5.3 冲突雷达 CI

`scripts/topic-radar.ps1`(附录 7.2)接入 GitHub Actions nightly(跑在**自己的 fork** 上,公共仓库免费):逐 topic 试 rebase,失败自动开 issue。无 CI 条件时用本地任务计划程序每天定时跑(见 §3.0)。冲突在上游提交新鲜时暴露,不攒到合并日。

### 5.4 发布流程

```bash
git fetch upstream
git switch -C mix/release-<date> upstream/develop
git merge --no-ff mix/topic/common
git merge --no-ff mix/topic/<F1>    # 固定顺序(先 common,按清单序)
git merge --no-ff mix/topic/<F2>
# ... 全部 topic
git submodule update --init --recursive
scripts\build_release.bat && 游戏内冒烟
git tag release-<date>
git push personal mix/release-<date> --tags   # 备份到自己的 fork
```

### 5.5 依赖 stacking 的同步

topic-B 基于 topic-A 时,先 rebase A,再把 B 挪到新 A 上:

```bash
git switch mix/topic/B
git rebase --onto mix/topic/A <A 被 rebase 前的旧顶点> mix/topic/B
```

栈多了用 [git-machete](https://github.com/VirtusLab/git-machete) 维护。

---

## 6. 已知风险与对策

| 风险 | 对策 |
|---|---|
| BOM 差异污染差量(已量化 ~163 文件) | Phase B 归一提交;CI BOM 检查防复发(`.rc` 例外保留 BOM) |
| 合并吞括号 / `#pragma endregion`(xdiff zealous 折叠,**无配置可关**) | 架构治本:topic 各自文件,共享文件只剩注册点(行内容唯一,不会被折叠);每次合并/变基后 `Test-MergeSanity` + 编译闸门;代码文件**禁用** `merge=union` |
| 注册点惯犯冲突(vcxproj / Phobos.Ext.cpp) | rerere 自动重放;必要时 vcxproj 开 union |
| 无自动化测试 | 每步编译闸门 + 发布冒烟;`Test-MergeSanity` 做机械自检 |
| YRpp 指针悬空(现存地雷) | 第 0 步排雷;此后执行 YRpp 先行铁律 |
| agent 自动解冲突出错 | 分级升级协议:机械冲突自动处理、架构冲突暂停上报;所有自动处理后必须过 `Test-MergeSanity` + 编译 |

---

## 7. 附录

### 7.1 关键 SHA 备忘

| SHA | 含义 |
|---|---|
| `14b904e5e` | Mix-ECpack 迁移源 tip("修正合并错误") |
| `2617d0725` | 对应的 develop 基点(#2384) |
| `33547e604` | 同步合并提交(parents: `74ef64d64` + `2617d0725`) |
| `1033731f0` | YRpp 内 `Mix-ECPack` 分支 tip(待排雷,2026-08-20) |
| `8468aab5e` | YRpp 官方 phobos-dev tip(2026-09-20) |
| `c6c522f7` | 本地 YRpp 当前 detached 位置 |

### 7.2 脚本

#### Test-MergeSanity(合并/变基后机械自检)

```powershell
function Test-MergeSanity {
  param([string]$From = "HEAD@{1}", [string]$To = "HEAD")
  $problems = @()
  foreach ($f in (git diff --name-only $From $To)) {
    if ($f -notmatch '\.(cpp|h|hpp|inl)$' -or -not (Test-Path $f)) { continue }
    $t = Get-Content $f -Raw
    $o = [regex]::Matches($t, '\{').Count; $c = [regex]::Matches($t, '\}').Count
    $r = [regex]::Matches($t, '(?m)^\s*#pragma\s+region').Count
    $e = [regex]::Matches($t, '(?m)^\s*#pragma\s+endregion').Count
    if ($o -ne $c) { $problems += "$f 花括号失衡: { $o vs } $c" }
    if ($r -ne $e) { $problems += "$f region 失衡: $r vs $e" }
  }
  if ($problems) { Write-Warning ($problems -join "`n"); return $false }
  return $true
}
```

粗粒度计数(字符串里的花括号可能误报)。用途:误报多看一眼,漏报才是事故;最终闸门仍是编译。

#### BOM 归一(Phase B 专用,`.rc` 除外)

```powershell
$changed = 0
foreach ($f in (git ls-files "*.cpp" "*.h" "*.hpp" "*.inl")) {
  $b = [IO.File]::ReadAllBytes($f)
  if ($b.Length -ge 3 -and $b[0] -eq 0xEF -and $b[1] -eq 0xBB -and $b[2] -eq 0xBF) {
    [IO.File]::WriteAllBytes($f, $b[3..($b.Length-1)]); $changed++
  }
}
"去 BOM: $changed 个文件"
git commit -am "chore: normalize to UTF-8 without BOM (per .editorconfig)"
```

#### BOM CI 检查(防复发,nightly/PR 均可)

```powershell
$bad = foreach ($f in (git ls-files "*.cpp" "*.h" "*.hpp" "*.inl")) {
  $b = [IO.File]::ReadAllBytes($f)
  if ($b.Length -ge 3 -and $b[0] -eq 0xEF -and $b[1] -eq 0xBB -and $b[2] -eq 0xBF) { $f }
}
if ($bad) { Write-Error "以下文件带 BOM: $($bad -join ', ')"; exit 1 }
```

#### topic-radar(冲突雷达,CI nightly)

```powershell
git fetch upstream
$failed = @()
git branch --format '%(refname:short)' --list 'mix/topic/*' | ForEach-Object {
    $wt = Join-Path $env:TEMP ('radar-' + ($_ -replace '[/\\]', '_'))
    git worktree add -q $wt $_
    Push-Location $wt
    git rebase upstream/develop 2>$null
    if ($LASTEXITCODE -ne 0) { $failed += $_; git rebase --abort }
    Pop-Location
    git worktree remove --force $wt
}
if ($failed) { Write-Host "需要处理的 topic: $($failed -join ', ')" }
```

### 7.3 YRpp 自有提交清单(待重建为 mix 分支,新→旧)

| SHA | 标题 |
|---|---|
| `729b0621` | 更新 |
| `9c559d54` | 更新 |
| `d2b2c2ea` | 修复 |
| `14203199` | 移除重复 |
| `3d46b176` | 按钮相关 |
| `0a54347a` | 三角函数 |
| `f93de612` | 更新需要用的 thiscall |
| `9fe97b88` | Revert "AStarClass" |

净改动:16 文件,+86/-166 行(`RadarClass.h`、`SidebarClass.h`、`TechnoClass.h`、`YRpp.props` 等)。重建时顺手改写为有信息量的提交信息。

### 7.4 执行检查单

- [x] §3.0:`personal` 远程(重命名 MJobos 远程)+ YRpp fork,完成首次推送(2026-10-02)
- [x] 第 0 步:YRpp 排雷完成(2026-10-02)——`TaranDahl/YRpp` fork,`mix` = `c6c522f7` + 5 提交(tip `74add5ca`);注意 mix **基于基点同代的 c6c522f7 而非官方最新**,官方九月演进留待整体跟进 develop 时再吸收;`.gitmodules` 已指向 fork,全量编译通过
- [x] 全局 git 配置 + rerere
- [x] Phase A:建 `mix/fused`(因 `.vscode` 沙箱拦截改用 commit-tree 构造,树与 14b904e5e 完全一致,零丢失)
- [x] Phase B:`/utf-8` + 编译 ✓ → BOM 归一 375 文件 → 编译 ✓ → 统计 511→342
- [x] Phase C:拆分清单初稿已产出(见 `拆分清单.md`,34 项功能注册表)→ 待用户审核与开放问题拍板
- [ ] Phase D:按清单逐功能摘取(每功能:双编译 + sanity + 确认)
- [ ] Phase E:残余寻亲
- [ ] Phase F:组装 + 全量编译 + 冒烟 + 对账 → 打 tag → 封存 Mix-ECpack
- [ ] 日常运作:接入冲突雷达 CI、BOM CI
