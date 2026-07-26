# 网格自适应 Agent 技术架构文档

> 基于 15 个 AI 自主科研系统的纵向分析设计
> 日期：2026-07-27
> 版本：Phase 3 完成 (记忆 + 并行 + 自演化)

---

## 一、系统定位与目标

### 1.1 核心定位

**LLM 驱动的自适应网格优化 Agent**——LLM 替代传统数学误差估计器（Zienkiewicz-Zhu、梯度重构、残差估计等），用语义理解 + 数值分析 + 多角色辩论判断"哪里需要加密/放粗"，自主完成从接受用户输入到仿真实验验证的全流程。

### 1.2 关键特征

| 特征 | 说明 |
|------|------|
| **PDE 不绑定** | 不硬编码特定物理方程，Agent 可处理对流-扩散、N-S、Euler 等多类 PDE |
| **求解器灵活** | 用户可提供求解器模板，Agent 也可自主编写（FEniCSx 默认） |
| **物理感知** | 数值程序化 pre-filter + LLM 物理推理（A+B 混合） |
| **质量最严** | PRE-Gate + POST-Gate + 多角色辩论 + Claim Ledger |
| **独立部署** | 独立 Python Agent 系统，CLI 起步，预留 Web API |

### 1.3 参考系统

本架构综合吸收了以下系统的设计经验：

| 维度 | 主要参考 | 核心借鉴 |
|------|---------|---------|
| 搜索与探索 | AI Scientist v2, ATHENA | MCTS + Contextual Bandit + 策略检索 |
| 质量验证 | EviBound, Re⁴, Sibyl H5 | PRE+POST 双门控 + 独立 Reviewer + 对立辩论 |
| 记忆管理 | DeepScientist, Sibyl H4, GRAFT-ATHENA | 知识图谱 + 角色路由 + Fingerprint 检索 |
| 代码编排 | ATHENA, DeepScientist, AI Scientist | Cell-by-Cell + Git Worktree + AIDER |
| 并行效率 | Robin, HTP Screening, Materials Lab | 策略级并行 + Worktree 隔离 |
| 预算管理 | ATHENA, ERA, Sibyl H6 | 硬限制 + 每阶段配额 + 早停 |

---

## 二、Agent 架构

### 2.1 总览

```
┌──────────────────────────────────────────────────────┐
│              Orchestrator（程序化状态机）                │
│   状态转换、预算追踪、早停判断、Cell 间契约检查             │
└──────────────────────────────────────────────────────┘
                         │
          ┌──────────────┼──────────────┐
          ▼              ▼              ▼
    Strategist      Optimist       Skeptic
    (Agent 1)       (Agent 2)      (Agent 3)
    策略生成+检索    找支持证据      找反对证据
                                  网格+物理质量
          │              │              │
          └──────────────┼──────────────┘
                         ▼
                  Gatekeeper (Agent 4)
              综合辩论 → 输出最佳策略
         （全被否决 → 终止本轮，不执行实验）
                         │
                         ▼
          ┌──────────────────────────┐
          │  Programmer    Reviewer  │
          │  (Agent 5a)    (Agent 5b)│  ← 不同 OpenAI 实例
          │  写/改代码      审代码     │
          └──────────────────────────┘
                         │
                         ▼
┌──────────────────────────────────────────────────────┐
│              执行层（程序化，非 Agent）                  │
│  网格生成 → 质量检查 → 求解器运行 → 后处理 → 声明提取     │
│              └── 每步最多重试 4 次 ──┘                  │
└──────────────────────────────────────────────────────┘
                         │
                         ▼
┌──────────────────────────────────────────────────────┐
│           Memory Keeper（Agent 6，Phase 2）            │
│  每轮结束后摘要 → 写入持久记忆 → 更新检索索引             │
└──────────────────────────────────────────────────────┘
```

**Phase 1 Agent 清单（6 个 LLM Agent + 1 程序化 Orchestrator）：**

| # | Agent | 职责 | LLM 强度 | 状态 |
|---|-------|------|---------|------|
| 1 | **Strategist** | 读求解结果 + 检索策略记忆 → 提出 3-5 个加密/放粗方案 | 强模型 | ✅ |
| 2 | **Optimist** | 论证每个方案为什么有效，找支持证据 | 强模型 | ✅ |
| 3 | **Skeptic** | 论证每个方案可能失败的原因 + 网格质量 + 物理合理性 | 强模型 | ✅ |
| 4 | **Gatekeeper** | 综合辩论结果 → 输出最佳策略；全否决则终止本轮 | 中模型 | ✅ |
| 5a | **Programmer** | 根据策略写/改网格生成代码和求解器参数 | 强模型 | ✅ |
| 5b | **Reviewer** | 审查代码逻辑、数值稳定性、离散化正确性 | 强模型 + 不同实例 | ✅ |
| 6 | **Memory Keeper** | 每轮摘要 → 写入持久记忆 + 角色路由 | 中模型 | ✅ |
| 7 | **Parallel Executor** | 多策略并行 worktree 调度 + 参数扫描 | 规则 (LLM 判层次B) | ✅ |

### 2.2 各 Agent 详细设计

#### Strategist（策略生成 + 检索）

```
输入:
  - 当前网格统计（单元数、质量指标分布）
  - 上一轮求解结果（数值场：速度、压力、涡量、残差）
  - 用户物理描述
  - 从记忆检索到的相似场景策略（Phase 2）

输出（结构化 JSON）:
  - 3-5 个加密/放粗方案
  - 每个方案: 区域描述 + 目标尺寸 + 预期效果 + 风险估计

核心 Prompt 要点:
  - 先分析全场数值，程序化标记超标区域（梯度/曲率/残差）
  - 结合用户物理描述识别特征（边界层、激波、尾流、分离区）
  - 每个方案只聚焦 1-2 个区域的操作（对标 AI Scientist v2 的单维度变异）
  - 必须给出量化预期（预计网格单元数变化、预计影响哪些指标）
```

#### Optimist

```
输入:
  - Strategist 的 3-5 个方案
  - 当前求解结果 + 物理描述

输出:
  - 每个方案的结构化评分（1-5 分 × 多维度）
  - 支持理由（对标 Sibyl H5 的 Optimist 角色）

评分维度:
  - 物理合理性: 加密区域是否确实是物理特征所在？
  - 数值必要性: 当前网格在该区域是否确实不足？
  - 预期收益: 加密后指标改善幅度估计
  - 实施可行性: 网格生成是否可行？有历史成功先例？
```

#### Skeptic

```
输入:
  - Strategist 的 3-5 个方案 + Optimist 的评分

输出:
  - 每个方案的反对评分（1-5 分 × 多维度）
  - 反对理由 + 具体风险描述

评分维度:
  - 网格质量风险: 加密是否会导致局部单元质量恶化？
  - 数值风险: 是否可能引入新的数值问题（如网格长宽比过大导致刚度矩阵病态）？
  - 成本风险: 单元数增加是否超出预算？
  - 物理误判风险: 是否可能是数值伪影被误判为物理特征？
  - 替代方案: 是否存在更简单/更便宜的替代方案？

特殊权力:
  - 可与 Optimist 的结果进行交叉对比
  - 如果发现致命缺陷（如加密区域选错），可直接建议 Gatekeeper 淘汰
```

#### Gatekeeper

```
输入:
  - 3-5 个方案 + Optimist 评分 + Skeptic 评分
  - 历史最佳结果（来自当前会话）

输出:
  - 选定 1 个最佳策略（Phase 1 串行模式）
  - 或终止本轮（所有方案被否决）→ 状态机进入 DECIDE → 可触发早停

决策规则:
  1. 综合评分 = Optimist 评分 × 0.5 + (5 - Skeptic 评分) × 0.5
  2. 综合评分 < 阈值（默认 2.5）→ 淘汰
  3. 存在致命缺陷标记 → 直接淘汰
  4. 所有候选被淘汰 → 返回空，终止本轮
```

#### Programmer

```
输入:
  - Gatekeeper 选定的策略（结构化 JSON，含区域描述 + 参数）
  - 当前网格文件路径
  - 求解器模板路径（如有）

输出:
  - 修改后的网格生成代码（Python/gmsh）
  - 或修改后的求解器参数（如需调参）

职责边界:
  - 用户模板求解器: 可调参数，可修 Bug，非必要不修改核心逻辑
  - Agent 自主编写求解器: 完整生成
  - 网格代码: 始终由 Agent 生成/修改
```

#### Reviewer

```
输入:
  - Programmer 生成的代码
  - 策略要求（目标区域、目标尺寸等）
  - 运行时输出（如有试运行）

输出:
  - 通过 / 不通过 + 具体修改建议

核心要求（对标 Re⁴）:
  - 必须是不同于 Programmer 的 OpenAI 实例
  - 审查维度: 
    (1) 代码正确性: 离散化是否正确？边界条件处理对吗？
    (2) 策略忠实度: 代码是否完全实现了策略的要求？
    (3) 数值稳定性: 是否存在已知的数值陷阱？
    (4) 执行安全: 是否会生成资源超限的网格？
```

### 2.3 通信协议

| 通信环节 | 参与者 | 格式 |
|----------|--------|------|
| 策略提案 | Strategist → 辩论 | 结构化 JSON（区域描述 + 参数 + 预期效果） |
| 辩论 | Optimist ↔ Skeptic | 自由文本推理 + 结构化评分（1-5 分 × 多维度） |
| Gate 决策 | Gatekeeper → Executor | 结构化 JSON（完整网格参数的选定策略） |
| 执行反馈 | Executor → Strategist | 结构化 metrics + 自由文本异常描述 |
| 记忆存储 | Memory Keeper | 结构化字段 + 嵌入向量（混合检索，Phase 2） |

---

## 三、物理感知与区域描述

### 3.1 物理感知架构（A+B 混合）

```
用户输入（物理描述 + 网格初始参数 + 求解结果）
    │
    ▼
程序化 Pre-filter（纯 Python，~0.01s）
    - 全场梯度计算（▽p, ▽u, ▽T）
    - 全场曲率计算
    - 局部残差分析
    - 输出: 超标区域列表（坐标 + 超标程度）
    │
    ▼
LLM 策略层（Strategist）
    - 读 Pre-filter 结果
    - 结合用户物理描述 → 识别物理特征
    - 区分: 物理特征 vs 数值伪影 vs 网格不足
    - 输出: 结构化加密/放粗策略
```

### 3.2 区域描述 Schema

区域描述采用层次化体系，Strategist 写语义描述 + 关键参数，Python 解析器补全坐标。

#### 基础几何

```json
{"type": "box",       "corner1": [x1,y1,z1], "corner2": [x2,y2,z2]}
{"type": "cylinder",  "center": [x,y,z], "radius": r, "axis": "z"}
{"type": "sphere",    "center": [x,y,z], "radius": r}
{"type": "cone",      "apex": [x,y,z], "base_center": [x,y,z], "base_radius": r, "height": h}
```

#### 边界/表面引用

```json
{"type": "tag",           "tag": "壁面名称"}
{"type": "tag_range",     "tag": "airfoil", "from": "leading_edge", "to": "0.3c_upper"}
{"type": "topology",      "topology_type": "concave_corners"}
```

#### 解场驱动

```json
{"type": "gradient_threshold",  "field": "pressure", "threshold": 1000.0}
{"type": "value_range",        "field": "vorticity", "min": 50.0, "max": null}
{"type": "curvature",          "field": "velocity", "threshold": 0.1}
{"type": "residual",           "threshold": 1e-3}
```

#### 物理特征（LLM 语义 → 程序化解析）

```json
{"type": "boundary_layer",     "on_surface": {...}, "first_layer_height": 5e-6, 
                                "growth_rate": 1.15, "num_layers": 15}
{"type": "wake",               "from_body": {...}, "direction": [1,0,0], 
                                "length": 20.0, "spread_angle": 5.0}
{"type": "shock",              "from_field": "pressure", "normal_thickness": 0.001, 
                                "tangential_width": 0.5}
{"type": "separation_bubble",  "on_surface": {...}, "estimated_length": 0.1}
```

#### 逻辑组合

```json
{"type": "boolean",   "operation": "union|intersection|difference", "a": {...}, "b": {...}}
{"type": "blend",     "regions": [{...}, {...}], "blend_radius": 0.01}
{"type": "distance",  "from_region": {...}, "distance": 0.05}
```

#### 沿曲线加密

```json
{"type": "along_curve",  "curve": {"type": "tag", "tag": "wake_centerline", 
                                    "range": {"from": "trailing_edge", "to": "20c"}},
                         "transverse_width": {"start": 0.0, "end": 1.5, "growth": "linear"},
                         "streamwise_size": {"start": 0.005, "end": 0.05, "growth": "linear"}}
```

### 3.3 操作描述

```json
{
  "action": "refine | coarsen",
  "target_size": 0.01,              // 目标网格尺寸（refine 时必填）
  "growth_rate": 1.1,               // 向区域外的增长率
  "priority": 1,                    // 1=必须实现, 2=建议实现（预算不足时可裁减）
  "target_metric": "y_plus",        // 可选: 以无量纲量为目标
  "target_value": 1.0               // 可选: 目标值
}
```

---

## 四、状态机设计

### 4.1 状态定义

```
                    ┌─────────┐
                    │  IDLE   │ ← 初始，等待用户输入
                    └────┬────┘
                         │ 用户输入 + 初始网格 + 求解器就绪
                         ▼
                    ┌─────────┐
                    │ ANALYZE │ ← Strategist 读求解结果 + 检索记忆
                    │         │    每轮从这里开始
                    └────┬────┘
                         │ 输出: 3-5 个加密/放粗方案
                         ▼
              ┌──────────────────┐
              │     DEBATE       │ ← Optimist + Skeptic（3-5 角色）
              │                  │    纯 LLM，无求解器运行
              └────────┬─────────┘
                       │ 输出: 每个方案的评分 + 风险
                       ▼
              ┌──────────────────┐
              │      GATE        │ ← Gatekeeper 综合 → 选定最佳策略
              │                  │    全被否决 → 跳到 DECIDE
              └────────┬─────────┘
                       │
                       ▼
              ┌──────────────────┐
              │    EXECUTE       │ ← Programmer + Reviewer + Cell 执行
              │   (单 Worktree)   │    每步最多重试 4 次
              └────────┬─────────┘
                       │ 所有步骤完成
                       ▼
              ┌──────────────────┐
              │     VERIFY       │ ← 程序化 POST-Gate + Claim 提取
              │                  │    数值自动验证
              └────────┬─────────┘
                       │
                       ▼
              ┌──────────────────┐
              │     DECIDE       │ ← 规则判数值 + LLM 判语义
              │                  │    有改进 → 回 ANALYZE
              │                  │    无改进/早停触发 → SUMMARIZE
              └────────┬─────────┘
                       │
                       ▼
              ┌──────────────────┐
              │   SUMMARIZE      │ ← Memory Keeper 写记忆（Phase 2）
              │                  │    输出最终报告 + 证据链
              └────────┬─────────┘
                       │
                       ▼
              ┌──────────────────┐
              │     CLOSED       │ ← 终止
              └──────────────────┘
```

### 4.2 状态转换规则

| 当前状态 | 合法下一状态 | 前置条件 |
|----------|-------------|---------|
| IDLE | ANALYZE | 用户输入完整（problem.yaml） |
| ANALYZE | DEBATE | Strategist 输出了 ≥1 个方案 |
| ANALYZE | DECIDE | 无可选方案（求解失败且无法修复） |
| DEBATE | GATE | 辩论完成，每个方案有评分 |
| GATE | EXECUTE | Gatekeeper 选定了 1 个策略 |
| GATE | DECIDE | Gatekeeper 全否决（所有方案评分 < 阈值） |
| EXECUTE | VERIFY | 网格生成 + 求解 + 后处理完成 |
| EXECUTE | DECIDE | 执行失败且重试 4 次耗尽 |
| VERIFY | DECIDE | POST-Gate 检查完成 |
| DECIDE | ANALYZE | LLM 或规则判定"有改进空间" |
| DECIDE | SUMMARIZE | 早停触发或指标收敛 |
| SUMMARIZE | CLOSED | 报告 + 证据链输出完成 |

**非法转换被代码拒绝**（对标 ResearchLoop 的 6 状态机）。

### 4.3 GATE 全否决处理

如果 Gatekeeper 所有候选预期都低于阈值或存在致命缺陷，返回空 → 状态转到 DECIDE。在 DECIDE 中：
- 如果是首轮（没有历史最佳）→ 降低阈值重试一次（阈值从 2.5 降至 2.0）
- 如果已有历史最佳且连续 2 轮被全否决 → 触发早停，进入 SUMMARIZE
- 禁止 DEBATE→GATE→DECIDE→ANALYZE→DEBATE 的无限制循环

---

## 五、质量验证体系

### 5.1 五层防线

```
L1: PRE-Gate（实验设计逻辑检查，~0.5s，LLM）
    检查: 策略是否有逻辑漏洞？加密区域是否对应物理特征？
          声明是否可测？预期指标是否有对应的输出 key？

L2: 多角色辩论（DEBATE 阶段）
    Optimist + Skeptic + 领域专家交叉验证

L3: Gatekeeper 门控（GATE 阶段）
    综合评分 + 致命缺陷淘汰

L4: 代码审查（EXECUTE 阶段，Programmer ↔ Reviewer）
    不同 LLM 实例交叉审查代码

L5: POST-Gate（程序化数值校验，~0.01s，无 LLM）
    网格质量: 偏斜率 < 0.9, 纵横比 < 100, Jacobian > 0, 无翻转单元
    求解质量: 残差下降 ≥ 3 个数量级, 质量/动量守恒检查
    声明验证: Claim Ledger 自动提取数值 → 比对声明 → 不匹配则 REJECT
```

### 5.2 Claim Ledger（对标 ResearchLoop）

每轮自适应产出一个声明条目：

```json
{
  "claim_id": "C3",
  "round": 3,
  "text": "尾流区加密使 Cd 从 1.42 降至 1.38（Δ = -2.8%）",
  "strategy": "S3_wake_refinement",
  "evidence": {
    "before": {"Cd": 1.42, "source": "worktree_r2/metrics.json#L5"},
    "after":  {"Cd": 1.38, "source": "worktree_r3/metrics.json#L5"},
    "delta": -0.04,
    "delta_pct": -2.82
  },
  "validation": {
    "method": "auto_extraction",
    "extracted_before": 1.42,
    "extracted_after": 1.38,
    "match": true,
    "significant": true,        // Δ > 最小阈值（默认 1%）
    "timestamp": "2026-07-21T15:30:00Z"
  },
  "status": "VALIDATED"
}
```

### 5.3 证据成熟度（对标 Sibyl H2）

| 级别 | 含义 | 允许的声明 |
|------|------|-----------|
| L1 | 代码跑通，求解完成 | "实验已执行" |
| L2 | 初步结果，数值在物理范围内 | "结果在合理范围内" |
| L3 | 分析就绪，指标已提取 | "Cd 从 A 变为 B" |
| L4 | 统计检验通过 + 多轮一致 | "加密策略有效" |
| L5 | 已审计声明（多个独立 Reviewer 确认） | 最终结论 |

**声明不能超越证据级别。** 例如 L1 只能声明"已执行"，不能声明"策略有效"。

---

## 六、代码编排

### 6.1 Cell-by-Cell 架构（对标 ATHENA）

```
┌─ Cell 1: 网格生成 ────────────────────────────────────┐
│  输入契约: 策略 JSON（区域 + 目标尺寸 + 增长率）         │
│  输出契约: mesh.msh/.xml + mesh_quality.json           │
│  代码来源: Agent 生成（pygmsh/gmsh Python API）         │
│  失败处理: Programmer → Reviewer → 重试（≤4 次）        │
└───────────────────────────────────────────────────────┘
                           ↓
┌─ Cell 2: 求解器 ──────────────────────────────────────┐
│  输入契约: mesh + 物理参数（Re, Ma, BCs）                 │
│  输出契约: solution.vtu/.h5 + convergence.log + residuals│
│  代码来源: 用户模板（可调参+可修Bug）或 Agent 编写        │
│  失败处理: 调参 → 重试（≤4 次）；仍失败 → 回退到用户模板   │
│  权限边界: 非必要不修改核心求解逻辑                        │
└───────────────────────────────────────────────────────┘
                           ↓
┌─ Cell 3: 后处理 + 声明 ───────────────────────────────┐
│  输入契约: solution + 上一轮 metrics                     │
│  输出契约: metrics.json + Claim Ledger 条目             │
│  代码来源: Agent 编写                                    │
│  失败处理: 修复提取脚本 → 重试（≤4 次）                   │
└───────────────────────────────────────────────────────┘
```

### 6.2 AIDER 增量编辑（对标 AI Scientist）

每轮代码修改使用 git diff 增量编辑，而非全量重写。git log 提供完整的变更审计链。

### 6.3 Git Worktree 执行隔离（对标 DeepScientist）

每轮自适应创建独立 git worktree：
- 物理隔离：不同轮次互不污染
- 可回滚：失败可回退到上一轮网格
- 并行友好：Phase 3 实现多 worktree 并行
- 分支管理：每轮 = 一个 git branch

---

## 七、EXECUTE 内部重试循环

```
Worktree 内部（每步最多重试 4 次）:

  生成网格代码 → 质量检查 ──失败──→ Programmer 修复 → Reviewer 审 → 重试(≤4次)
       │ 通过
       ▼
  求解器运行 ──发散/报错──→ 调整参数/修 Bug → 重试(≤4次)
       │ 通过
       ▼
  后处理提取指标 → 完成

  任一环节重试 4 次耗尽 → 该轮标记失败 → 状态转到 DECIDE
```

---

## 八、DECIDE 判断逻辑

### 8.1 规则判数值（程序化）

```
1. 提取本轮所有指标的 Δ 值
2. 如果 Δ > minimum_significant_threshold（默认 1%）→ 规则判定"有改进"
3. 如果所有指标 Δ < threshold → 规则判定"无显著改进"
4. 如果指标变差 → 规则判定"退化"
5. 连续 2 轮无显著改进 → 触发早停
```

### 8.2 LLM 判语义

```
当规则判定"无显著改进"但结果仍可疑时，调用 LLM 判断:
  - "虽然 Cd 数值变化不大，但流场结构（涡量分布、分离点位置）是否更合理？"
  - "求解收敛性是否改善（残差下降更快）？"
  - "网格质量是否提高（即使指标未变）？"

LLM 判断"有质量改进" → 可追加最后一轮（一次性豁免）
LLM 判断"确实无改进" → 触发早停
```

---

## 九、预算管理

### 9.1 三层模型

```
L1: 硬限制（全局安全网）
    - 总求解次数上限: N_max（默认 5，用户可配置）
    - 总墙钟时间上限: T_max（用户给，或 Agent 估算）
    - 总 LLM 调用上限: 可设但通常不是瓶颈
    - 用户未配置 → Agent 宽松估算（安全系数 1.5）

L2: 每阶段配额
    - ANALYZE:  最多 10 次 LLM 调用（策略生成 + 检索）
    - DEBATE:   最多 30 次 LLM 调用（3-5 角色辩论）
    - EXECUTE:  每 Cell 最多 15 次 LLM 调用（Programmer + Reviewer + 重试）
    - 网格单元数上限: 用户指定或默认 500k

L5: 早停（信号驱动）
    - 连续 2 轮指标改进 < 阈值 → 停止
    - 求解发散（残差不降反升）→ 停止，回退到上一轮网格
    - Gatekeeper 连续 2 轮全否决 → 停止
    - 网格单元数超上限 → 停止并警告
    - 总求解次数 或 总时间 触及上限 → 停止
```

### 9.2 成本估算（Agent 自主）

```
如果用户未提供预算:
  1. 分析问题规模（几何复杂度、PDE 类型、初始网格单元数）
  2. 估算单次求解时间
  3. 预算 = 单次求解时间 × 安全系数 1.5 × 默认 5 轮
  4. 设置上限: 总墙钟时间 ≤ 4 小时（硬编码最大上限）

求解器运行超时（单次超过估算 × 3）→ kill + 标记异常
```

---

## 十、策略检索（Phase 1 基础，Phase 2 完整）

### 10.1 Phase 1: 会话内 RAG

- 知识库: 预置 5-10 条核心策略文档（激波、边界层、尾流、驻点、分离区）
- 检索: 向量嵌入 → 相似度排序 → Top-K 注入 Strategist prompt
- 模式: **建议模式**（检索结果作为 LLM 参考，LLM 可 override）

### 10.2 Phase 2: 持久混合检索

```
策略记忆库
├── 结构化字段（精确匹配）
│   ├── 物理场景指纹: {pde_type, Re, Ma, geometry_type, ...}
│   ├── 策略参数: {region_type, target_size, growth_rate, ...}
│   ├── 效果标签: {effective, ineffective, has_side_effect}
│   └── 查询: 结构化 SQL/JSON 过滤
│
├── 向量嵌入（语义检索）
│   ├── 嵌入内容: 策略描述 + 物理场景自然语言 + 经验总结
│   └── 查询: 余弦相似度 Top-K
│
└── 角色路由（对标 Sibyl H4）
    ├── 边界层相关教训 → 只注入边界层分析角色
    ├── 激波相关教训 → 只注入激波分析角色
    └── 网格质量教训 → 只注入网格质量检查角色
```

### 10.3 预留 GRAFT 接口

```python
# Phase 2 预留: Fingerprint Embedding 接口
class StrategyFingerprint:
    def encode(self, scene: dict, strategy: dict, outcome: dict) -> np.ndarray:
        """将成功策略编码为嵌入向量"""
        ...

    def search(self, query_scene: dict, k: int = 5) -> list[dict]:
        """检索最相似的 K 个成功策略"""
        ...

# Phase 3 预留: 知识图谱接口
class StrategyGraph:
    def add_node(self, type: str, data: dict): ...
    def add_edge(self, from_id: str, to_id: str, relation: str): ...
    def query(self, cypher: str) -> list: ...  # 可计算查询
```

---

## 十一、并行执行（Phase 3）

### 11.1 并行层次

```
层次 A: 策略级并行（主）
  同一轮 Gatekeeper 产出 Top N 策略 → N 个 worktree 并行执行

层次 B: 参数级并行（LLM 判定启动）
  某策略方向优势极大 或 预算充足 → 同一策略探索不同参数组合
  由 Parallel Executor Agent 判定启动条件
```

### 11.2 并行 vs 串行判定规则

```
规则:
  - Gatekeeper 输出策略数 = 1 → 串行
  - Gatekeeper 输出策略数 ≥ 2 且 剩余预算 ≥ 2 × 单次求解成本 → 并行
  - 否则 → 串行（预算不足）
```

### 11.3 并行执行架构

```
Parallel Executor (Agent)
  │
  ├─ Worktree A ─── Cell 1 → Cell 2 → Cell 3 ──┐
  ├─ Worktree B ─── Cell 1 → Cell 2 → Cell 3 ──┤
  │                                              │
  │  任一 worktree 失败 → 不阻塞其他 worktree      │
  │  全部完成 → 收集结果 → Skeptic 比较             │
  └──────────────────────────────────────────────┘
```

---

## 十二、输入/输出规范

### 12.1 输入格式（problem.yaml）

```yaml
problem:
  description: "二维圆柱绕流，Re=100，预计尾流区存在涡脱落，边界层需要 y+ < 1"
  pde_type: "navier-stokes"
  geometry: "./geometry/cylinder.yaml"
  boundary_conditions:
    farfield:
      type: "uniform_inlet"
      u: 1.0
      v: 0.0
    cylinder:
      type: "no_slip"
    outlet:
      type: "pressure_outlet"
      p: 0.0

solver:
  type: "user_template"          # user_template | agent_generated
  path: "./solver/ns_fenics.py"  # 用户模板路径
  interface: "fenicsx"           # fenicsx | openfoam | generic_cli
  params:                        # Agent 可调参数
    reynolds: 100
    dt: 0.001
    max_iterations: 5000

mesh:
  initial: "./mesh/init.msh"
  format: "gmsh"
  max_cells: 100000

budget:
  max_solver_runs: 5             # 可选
  max_wall_time_minutes: 120     # 可选
  # 未配置则由 Agent 自主估算

output:
  target_metrics:
    - "drag_coefficient"
    - "lift_coefficient"
    - "strouhal_number"
  convergence_tolerance: 1e-4
  output_dir: "./output/"
```

### 12.2 输出格式（result.yaml）

```yaml
summary:
  rounds: 3
  initial_mesh_cells: 5000
  final_mesh_cells: 23500
  improvements:
    drag_convergence: 0.42
    lift_convergence: 0.15

rounds:
  - round: 1
    strategy_id: "S1_wake_bl_refinement"
    strategy_summary: "尾流区加密 + 边界层 y+ < 1"
    mesh_change:
      cells_added: 8000
      regions: ["wake", "boundary_layer"]
    before:
      Cd: 1.42
      Cl: 0.08
      residual: 1e-5
    after:
      Cd: 1.38
      Cl: 0.05
      residual: 1e-6
    claim: "尾流区加密使 Cd 降低 2.8%，趋于收敛"
    evidence_ref: "C1"
    verdict: "ACCEPTED"

evidence_chain:
  - claim_id: "C1"
    text: "尾流区加密使 Cd 从 1.42 降至 1.38"
    evidence_file: "worktree_r1/metrics.json"
    extraction:
      Cd_before: 1.42
      Cd_after: 1.38
      delta_pct: -2.82
    validated: true
    significance: "significant"
    maturity_level: 4

  - claim_id: "C2"
    text: "第二轮进一步加密前缘使 Cl 收敛至 < 1e-3 波动"
    evidence_file: "worktree_r2/metrics.json"
    extraction:
      Cl_std_before: 0.012
      Cl_std_after: 0.0008
      delta_pct: -93.3
    validated: true
    significance: "significant"
    maturity_level: 4

artifacts:
  final_mesh: "./output/final_mesh.msh"
  mesh_quality: "./output/mesh_quality.json"
  solutions: "./output/solutions/"
  metrics_history: "./output/metrics_history.json"
  worktree_logs: "./output/worktrees/"
```

---

## 十三、技术实现

### 13.1 技术栈

| 层面 | 选择 | 说明 |
|------|------|------|
| 语言 | Python 3.11+ | — |
| LLM SDK | OpenAI Python SDK | 仅支持 OpenAI API |
| Agent 编排 | 自定义 Event Loop + asyncio | 对标 ATHENA/DeepScientist 的自研编排 |
| 网格生成 | gmsh + pygmsh | 网格生成事实标准 |
| 默认求解器 | FEniCSx | 声明式 PDE，LLM 编写 UFL 可靠性高 |
| 求解器接口 | 抽象 solver_interface | 用户模板 + Agent 自编写统一入口 |
| 版本管理 | Git + Git Worktree | 对标 DeepScientist |
| 数值计算 | numpy + scipy | 梯度/曲率/残差计算 |
| 嵌入检索 | numpy + 自定义向量存储（Phase 1） | Phase 2 升级为向量数据库 |

### 13.2 项目结构

```
mesh_agent/
├── core/                          # 核心引擎（与入口无关）
│   ├── orchestrator.py            # 状态机 + 预算管理
│   ├── state_machine.py           # 状态定义 + 转换验证
│   ├── agents/                    # Agent 实现
│   │   ├── base.py                # Agent 基类（LLM 调用封装）
│   │   ├── strategist.py          # 策略生成 + 检索
│   │   ├── optimist.py            # 支持证据
│   │   ├── skeptic.py             # 反对证据 + 网格/物理质量
│   │   ├── gatekeeper.py          # 综合决策
│   │   ├── programmer.py          # 代码生成/修改
│   │   ├── reviewer.py            # 代码审查（不同实例）
│   │   ├── memory_keeper.py       # 记忆管理
│   │   └── role_router.py         # 角色路由
│   ├── executor/                  # 执行层
│   │   ├── worktree_manager.py    # Git Worktree 管理
│   │   ├── cell_executor.py       # Cell-by-Cell 执行 + 重试
│   │   ├── parallel_executor.py   # 并行执行
│   ├── memory/                    # 记忆系统
│   │   ├── store.py               # 记忆存储接口
│   │   ├── retriever.py           # 混合检索（结构化 + 向量）
│   │   ├── role_router.py         # 角色路由（Phase 2）
│   │   └── strategies/            # 预置策略知识库
│   │       ├── boundary_layer.md
│   │       ├── wake.md
│   │       ├── shock.md
│   │       └── separation.md
│   └── verification/              # 质量验证
│       ├── pre_gate.py            # 实验设计逻辑检查
│       ├── post_gate.py           # 程序化数值校验
│       ├── claim_ledger.py        # 声明 + 证据绑定
│       └── mesh_quality.py        # 网格质量指标计算
├── solver_interface/              # 求解器抽象层
│   ├── base.py                    # 求解器抽象基类
│   ├── fenicsx.py                 # FEniCSx 适配器
│   ├── user_template.py           # 用户模板适配器
│   └── schema.py                  # 求解器参数 Schema
├── region_parser/                 # 区域描述解析器
│   ├── parser.py                  # 主解析器（语义 → 坐标）
│   ├── geometric.py               # 基础几何解析
│   ├── field_driven.py            # 解场驱动解析
│   ├── physics_features.py        # 物理特征解析
│   └── boolean_ops.py             # 布尔操作
├── cli/                           # CLI 入口
│   ├── main.py                    # click/argparse 入口
│   └── commands/
│       ├── run.py                 # mesh-agent run -p problem.yaml
│       └── status.py              # mesh-agent status <task_id>
├── api/                           # Web API（Phase 3 预留）
│   ├── server.py                  # FastAPI
│   └── models.py                  # Pydantic models
├── tests/                         # 测试
│   ├── test_orchestrator.py
│   ├── test_agents/
│   ├── test_executor/
│   └── test_verification/
├── examples/                       # 示例算例
│   └── cylinder_re100/
│       ├── problem.yaml
│       ├── solver/
│       └── mesh/
├── pyproject.toml
└── README.md
```

---

## 十四、实现规划

### Phase 1: 最小闭环（当前）

**目标**: 一个算例跑通 IDLE → ANALYZE → DEBATE → GATE → EXECUTE → VERIFY → DECIDE → CLOSED

**包含**:
- [x] Orchestrator 状态机（串行 + 并行模式）
- [x] Strategist（策略生成 + 会话内检索 + 跨会话记忆）
- [x] Optimist + Skeptic 辩论（3-5 角色）
- [x] Gatekeeper（综合评分 → 选定最佳策略 + 并行候选判定）
- [x] Programmer + Reviewer（不同 LLM 实例）
- [x] Cell-by-Cell 执行（网格生成 + 求解 + 后处理 + 可视化）
- [x] 轻量重试循环（每步 ≤ 6 次）
- [x] PRE-Gate（实验设计逻辑检查）
- [x] POST-Gate（网格质量 + 求解收敛 + 声明验证）
- [x] Claim Ledger（声明 + 证据绑定 + 自动验证）
- [x] 单/多 Worktree 执行
- [x] 预算 L1（硬限制）+ L2（每阶段配额）+ L5（早停）
- [x] CLI 入口 + .env 配置

**Phase 2 完成**:
- [x] Memory Keeper Agent
- [x] L2 持久结构化记忆（混合检索）
- [x] L3 角色路由记忆注入
- [x] 策略检索独立 Agent 管理
- [x] GRAFT Fingerprint 嵌入接口

**Phase 3 完成**:
- [x] Parallel Executor Agent
- [x] 多 Worktree 并行执行
- [x] 层次 B（参数级并行）LLM 判定
- [x] 并行结果汇合 + 比较
- [x] L4 自演化接口

**不包含**:
- [ ] Web API
