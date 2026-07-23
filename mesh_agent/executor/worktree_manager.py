"""Git worktree management for experiment isolation."""

from __future__ import annotations

import subprocess
import shutil
from pathlib import Path
from typing import Optional


class WorktreeManager:
    """Manages git worktrees for isolating mesh adaptation experiments.

    Each round of adaptation runs in an isolated worktree so that:
    - Different rounds don't pollute each other
    - Failed experiments can be rolled back
    - Parallel experiments don't conflict (Phase 3)
    """

    def __init__(self, base_dir: str | Path):
        self.base_dir = Path(base_dir).resolve()
        self.git_dir = self.base_dir / ".mesh_agent_git"
        self.worktrees_dir = self.base_dir / ".worktrees"
        self._initialized = False

    def _ensure_initialized(self) -> None:
        if self._initialized:
            return
        self.base_dir.mkdir(parents=True, exist_ok=True)
        if not (self.base_dir / ".git").exists():
            self._run_git(["init"], cwd=self.base_dir)
            self._run_git(["config", "user.name", "mesh-agent"], cwd=self.base_dir)
            self._run_git(["config", "user.email", "mesh-agent@local"], cwd=self.base_dir)
            # Create initial commit so worktrees can be created
            placeholder = self.base_dir / ".gitkeep"
            placeholder.write_text("")
            self._run_git(["add", ".gitkeep"], cwd=self.base_dir)
            self._run_git(["commit", "-m", "initial"], cwd=self.base_dir)
        self.worktrees_dir.mkdir(parents=True, exist_ok=True)
        self._initialized = True

    def _run_git(self, args: list[str], cwd: Path) -> tuple[int, str, str]:
        result = subprocess.run(
            ["git"] + args,
            cwd=str(cwd),
            capture_output=True,
            text=True,
            timeout=30,
        )
        return result.returncode, result.stdout, result.stderr

    def create(self, name: str) -> Path:
        """Create a new worktree for a round."""
        self._ensure_initialized()
        wt_path = self.worktrees_dir / name
        branch = f"round/{name}"

        # Remove existing worktree if it exists
        if wt_path.exists():
            self._run_git(["worktree", "remove", "--force", str(wt_path)], cwd=self.base_dir)
            # Cleanup leftover branch
            self._run_git(["branch", "-D", branch], cwd=self.base_dir)

        code, stdout, stderr = self._run_git(
            ["worktree", "add", "-b", branch, str(wt_path), "HEAD"],
            cwd=self.base_dir,
        )
        if code != 0:
            raise RuntimeError(f"Failed to create worktree '{name}': {stderr}")

        return wt_path

    def commit(self, worktree_path: Path, message: str) -> None:
        """Commit changes in a worktree."""
        code, _, stderr = self._run_git(["add", "-A"], cwd=worktree_path)
        if code != 0:
            raise RuntimeError(f"Git add failed: {stderr}")
        code, _, stderr = self._run_git(["commit", "-m", message, "--allow-empty"], cwd=worktree_path)
        if code != 0:
            raise RuntimeError(f"Git commit failed: {stderr}")

    def cleanup(self, name: str) -> None:
        """Remove a worktree and its branch."""
        wt_path = self.worktrees_dir / name
        branch = f"round/{name}"
        if wt_path.exists():
            self._run_git(["worktree", "remove", "--force", str(wt_path)], cwd=self.base_dir)
        self._run_git(["branch", "-D", branch], cwd=self.base_dir)

    def copy_inputs(self, worktree_path: Path, files: dict[str, str | Path]) -> None:
        """Copy input files into the worktree."""
        for dest_name, src_path in files.items():
            src = Path(src_path)
            if src.exists():
                shutil.copy2(str(src), str(worktree_path / dest_name))

    def get_latest_metrics(self, name: str) -> Optional[Path]:
        """Get the metrics file from a completed worktree."""
        wt_path = self.worktrees_dir / name / "output" / "metrics.json"
        return wt_path if wt_path.exists() else None
