@echo off
REM ==============================================================
REM 网格自适应 Agent — 一键启动 (Windows)
REM
REM 首次运行: 确保项目根目录 .env 已填入 API key
REM 每次使用: 双击此文件，按提示输入问题描述
REM
REM 高级用法:
REM   launch.py --prompt "你的问题描述"
REM   launch.py --config my_problem.yaml
REM   launch.py --help
REM ==============================================================

cd /d "%~dp0"

REM 激活虚拟环境 (如果有)
REM call ..\venv\Scripts\activate.bat

python launch.py

pause
