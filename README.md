# Mesh Agent

LLM 驱动的自适应网格优化 Agent — 从接受用户输入到仿真实验验证的完全自主闭环系统。

## 快速开始

### 1. 环境要求

- Python >= 3.11
- Git
- OpenAI 兼容 API (支持 DeepSeek 等)

### 2. 安装

```bash
cd mesh_agent
pip install -e .
```

### 3. 配置

项目根目录已有 `.env` 模板。编辑填入 API 信息：

```env
OPENAI_API_KEY=sk-your-key-here
OPENAI_BASE_URL=https://api.deepseek.com
MESH_AGENT_API_TIMEOUT=180
```

### 4. 编写你的输入

用户在 **`workspace/`** 目录中准备输入。根据你的情况，**三种方式任选**：

#### 方式 1: 你已有求解器（推荐）

只需编辑 `problem.yaml`，将 `solver.path` 指向你的求解脚本：

```yaml
solver:
  type: "user_template"
  path: "./my_solver.py"      # 你的求解器路径
```

你的求解器只要接受 `--mesh <dir> --output-dir <dir> --params '<json>'` 这三个参数即可。
参见 `solver_template.py`（Python）或 `solver_wrapper.sh`（Shell）。
网格放到 `mesh/` 目录。

#### 方式 2: 让 Agent 自己写求解器

```yaml
solver:
  type: "agent_generated"
  path: ""
```

Agent 自动生成 Python 求解器。仅适合简单 PDE（1D/2D 基础方程）。

#### 方式 3: 用内置测试算例

```bash
mesh-agent run -p tests\test_convection_diffusion\problem.yaml -o .\test_output
```

不需要准备任何文件，直接看效果。

#### 工作目录结构

```
workspace/
├── problem.yaml           ← 你写的问题描述和配置
├── solver.py              ← 你的求解器 (方式1, 从模板复制)
├── solver_wrapper.sh      ← Shell 版求解器模板 (方式1)
├── mesh/                  ← 初始网格文件
│   └── your_mesh.msh
├── output/                ← 结果 (自动生成)
│   ├── result.yaml
│   └── *.png
└── memory/                ← 跨会话记忆 (自动生成)
```

### 5. 运行

```bash
# 在 workspace 目录内
mesh-agent run -p problem.yaml -o ./output -m ./memory

# 或者直接双击 run.bat (Windows)
```

### 6. 求解器接口规范

无论用户自己写求解器还是让 Agent 生成，求解器必须接受以下 CLI 参数：

```bash
python solver.py --mesh <mesh_dir> --output-dir <output_dir> --params '<json>'
```

**必须输出两个文件到 `--output-dir`：**

`metrics.json`:
```json
{
  "drag_coefficient": 1.42,
  "lift_coefficient": 0.08,
  "solver_convergence": {
    "converged": true,
    "residual_final": 1e-6,
    "residual_initial": 1.0,
    "iterations": 5000
  }
}
```

`mesh_quality.json` (可选，但有会更好):
```json
{
  "total_cells": 15000,
  "max_skewness": 0.7,
  "max_aspect_ratio": 15.0,
  "min_jacobian": 0.2,
  "has_inverted_cells": false
}
```

### 7. 解读输出

运行结束后 `output/result.yaml` 包含完整结果：

```yaml
summary:
  rounds: 2                    # 自适应迭代轮数
  initial_mesh_cells: 5000
  final_mesh_cells: 23500
  improvements:
    drag_coefficient: 1.38

rounds:
  - round: 1
    strategy_id: S1_wake_refinement
    before: {Cd: 1.42, Cl: 0.08}
    after:  {Cd: 1.38, Cl: 0.05}
    claim: "尾流区加密使 Cd 从 1.42 降至 1.38"
    verdict: ACCEPTED
    images: ["output/.../mesh_comparison.png", ...]

evidence_chain:
  - claim_id: C1
    text: "尾流区加密使 Cd 从 1.42 降至 1.38"
    validation: {match: true, significant: true}
    evidence_level: 4          # L4 = 已验证
```

`output/` 目录中还包含每轮生成的可视化对比图。

---

## 工作流程

```
用户提供 problem.yaml
    │
    ▼
┌─ INIT_SOLVE ──────────────────────────────────────────────┐
│  无求解器 → Agent 自己生成并测试 (≤6 次重试)               │
│  有求解器 → 直接运行初始网格                                │
└──────────────────────────────────────────────────────────┘
    │
    ▼
┌─ ANALYZE ────────────────────────────────────────────────┐
│  Strategist 读求解结果 + 检索知识库 + 跨会话记忆            │
│  → 提出 3-5 个加密/放粗方案                                 │
└──────────────────────────────────────────────────────────┘
    │
    ▼
┌─ DEBATE ────────────────────────────────────────────────┐
│  Optimist: 论证每个方案为什么有效 (4 维度评分)              │
│  Skeptic: 论证每个方案为什么可能失败 (含致命缺陷检测)       │
└──────────────────────────────────────────────────────────┘
    │
    ▼
┌─ GATE ───────────────────────────────────────────────────┐
│  Gatekeeper 综合辩论 → 选定最佳策略                        │
│  所有策略被否 → 终止本轮                                   │
│  多个高分策略 + 预算充足 → 并行执行 (Phase 3)               │
└──────────────────────────────────────────────────────────┘
    │
    ▼
┌─ EXECUTE ───────────────────────────────────────────────┐
│  Cell 1: Programmer 写网格代码 → Reviewer 审查 → 运行     │
│  Cell 2: 求解器运行 (≤6 次重试 + 参数自动调整)            │
│  Cell 3: 后处理 → 提取指标                                │
│  Cell 4: 可视化 → mesh/solution 对比图                    │
└──────────────────────────────────────────────────────────┘
    │
    ▼
┌─ VERIFY ────────────────────────────────────────────────┐
│  PRE-Gate: LLM 检查实验设计逻辑                            │
│  POST-Gate: 程序化检查数值/网格质量/收敛                   │
│  Claim Ledger: 声明-证据绑定 + 自动数值提取验证             │
└──────────────────────────────────────────────────────────┘
    │
    ▼
┌─ DECIDE ────────────────────────────────────────────────┐
│  有改进 + 预算剩余 → 回到 ANALYZE 下一轮                    │
│  无改进 → LLM 语义判断是否再试一次                         │
│  早停触发 → SUMMARIZE                                     │
└──────────────────────────────────────────────────────────┘
    │
    ▼
┌─ SUMMARIZE ─────────────────────────────────────────────┐
│  Memory Keeper 持久化经验 → 结果输出 + 证据链              │
└──────────────────────────────────────────────────────────┘
```

---

## 两种使用模式

### 模式 A：用户提供求解器（推荐起步）

```yaml
solver:
  type: "user_template"
  path: "./my_solver.py"
```

你的求解器只需遵循 CLI 接口规范。Agent 负责：
- 分析求解结果 → 提出网格修改策略
- 生成/修改网格代码
- 调整求解器参数 (dt, max_iterations 等)
- 非必要不修改求解器核心逻辑

已有 CUDA 求解器？参考 `examples/streamer_1atm/` 中的 `run_solver.py` 包装器。

### 模式 B：Agent 自己写求解器

```yaml
solver:
  type: "agent_generated"
  path: ""
```

Agent 从零生成 Python 求解器 (numpy/scipy)，通过语法检查 + Reviewer 审查 + 实际运行测试后才使用。适合简单问题（1D/2D 基本 PDE）。

---

## 已测试算例

`tests/test_convection_diffusion/` — 1D 对流-扩散 (Pe=50)
- 20 均匀节点 → 37 节点几何加密
- 误差 0.052 → 0.012 (并行双策略验证)
- 跨轮学习：自动诊断失败策略

`examples/streamer_1atm/` — 2D 轴对称等离子体流光 (CUDA)
- NR=256 × NZ=1792, 13 个物理特征坐标
- 含 `run_solver.py` 包装器

---

## 项目结构

```
mesh_agent/
├── orchestrator.py          # 主循环: 10 状态驱动
├── state_machine.py         # 状态定义 + 转换验证
├── budget.py                # L1 硬限制 + L2 配额 + L5 早停
├── schemas.py               # 全部 Pydantic 模型
├── cli.py                   # CLI 入口
│
├── agents/
│   ├── base.py              # Agent 基类 (StrongAgent/LightAgent)
│   ├── strategist.py        # 策略生成 + 知识库检索 + 记忆注入
│   ├── optimist.py          # 支持论证 (4 维度评分)
│   ├── skeptic.py           # 反对论证 + 致命缺陷检测
│   ├── gatekeeper.py        # 辩论综合 → 选定/否决
│   ├── programmer.py        # 代码生成 (网格/求解器/后处理/可视化)
│   ├── reviewer.py          # 独立代码审查 (不同 LLM 实例)
│   └── memory_keeper.py     # 每轮摘要 → 持久记忆
│
├── executor/
│   ├── cell_executor.py     # Cell-by-Cell 执行 + 重试
│   ├── worktree_manager.py  # Git Worktree 隔离
│   └── parallel_executor.py # 多策略并行调度
│
├── memory/
│   ├── store.py             # 混合存储 (JSON + NPY + 索引)
│   ├── retriever.py         # 混合检索 (结构化 + 向量 + 标签)
│   └── role_router.py       # L3 角色路由 (Sibyl H4)
│
├── verification/
│   ├── pre_gate.py          # LLM 实验设计逻辑检查
│   ├── post_gate.py         # 程序化数值校验
│   ├── claim_ledger.py      # 声明-证据绑定
│   └── mesh_quality.py      # 网格质量指标
│
├── region_parser/           # 区域描述解析器
├── solver_interface/        # 求解器抽象接口
└── knowledge/               # 预设策略知识库 (4 篇)
```

---

## 常用命令速查

```bash
# 安装
pip install -e .

# 验证配置
mesh-agent validate -p problem.yaml

# 基本运行
mesh-agent run -p problem.yaml -o ./output

# 带记忆（跨会话学习）
mesh-agent run -p problem.yaml -o ./output -m ./memory

# 指定 API key
mesh-agent run -p problem.yaml --api-key sk-xxx

# 带自定义知识库
mesh-agent run -p problem.yaml -k ./my_knowledge -o ./output
```

---

## 环境变量参考

| 变量 | 默认值 | 说明 |
|------|--------|------|
| `OPENAI_API_KEY` | (必填) | API 密钥 |
| `OPENAI_BASE_URL` | `https://api.openai.com/v1` | API 地址 |
| `MESH_AGENT_API_TIMEOUT` | `180` | 单次 API 调用超时(秒) |
| `MESH_AGENT_STRONG_MODEL` | `deepseek-chat` | 策略/辩论/编程用模型 |
| `MESH_AGENT_LIGHT_MODEL` | `deepseek-chat` | Gatekeeper/记忆用模型 |
