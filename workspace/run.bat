@echo off
REM ==============================================================
REM 网格自适应 Agent — Windows 一键启动
REM
REM 使用方法:
REM   1. 编辑 problem.yaml 描述你的问题
REM   2. 编辑 solver.py 实现你的求解器
REM   3. 将初始网格文件放到 mesh\ 目录
REM   4. 双击此文件或在终端运行它
REM ==============================================================

cd /d "%~dp0"

REM 激活虚拟环境 (如果有的话，取消下面这行的注释)
REM call ..\venv\Scripts\activate.bat

REM 运行 Agent
mesh-agent run -p problem.yaml -o output -m memory

echo.
echo ==============================================================
echo 运行完成。结果在 output\ 目录中。
echo   output\result.yaml    — 完整结果报告
echo   output\*.png          — 可视化对比图
echo   memory\               — 跨会话记忆 (下次运行时可用)
echo ==============================================================
pause
