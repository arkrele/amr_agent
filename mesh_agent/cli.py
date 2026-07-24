"""CLI entry point for mesh-agent."""

from __future__ import annotations

import asyncio
import sys
from pathlib import Path

# Fix Windows encoding
if sys.platform == "win32":
    try:
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
        sys.stderr.reconfigure(encoding="utf-8", errors="replace")
    except Exception:
        pass

import click

from mesh_agent.orchestrator import Orchestrator
from mesh_agent.schemas import ProblemSpec


@click.group()
def main() -> None:
    """Mesh Agent — LLM-driven adaptive mesh refinement for numerical simulation."""
    pass


@main.command()
@click.option(
    "-p", "--problem",
    required=True,
    type=click.Path(exists=True),
    help="Path to problem.yaml specification file",
)
@click.option(
    "-o", "--output-dir",
    default="./output",
    type=click.Path(),
    help="Output directory for results and artifacts",
)
@click.option(
    "-k", "--knowledge-dir",
    default=None,
    type=click.Path(exists=True),
    help="Path to mesh strategy knowledge base directory",
)
@click.option(
    "--api-key",
    default=None,
    help="OpenAI API key (or set OPENAI_API_KEY env var)",
)
def run(problem: str, output_dir: str, knowledge_dir: str | None, api_key: str | None) -> None:
    """Run mesh adaptation on a problem specification.

    Example:
        mesh-agent run -p examples/cylinder/problem.yaml -o ./output
    """
    import os

    if api_key:
        os.environ["OPENAI_API_KEY"] = api_key

    if not os.environ.get("OPENAI_API_KEY"):
        click.echo("Error: OPENAI_API_KEY not set. Use --api-key or set the environment variable.", err=True)
        sys.exit(1)

    # Load problem spec
    try:
        problem_spec = ProblemSpec.from_yaml(problem)
    except Exception as e:
        click.echo(f"Error loading problem spec: {e}", err=True)
        sys.exit(1)

    # Resolve knowledge dir
    kd = knowledge_dir
    if not kd:
        # Look for default knowledge dir next to the package
        default_kd = Path(__file__).parent / "knowledge"
        if default_kd.exists():
            kd = str(default_kd)

    click.echo(f"Problem: {problem_spec.problem.description}")
    click.echo(f"Output:  {output_dir}")
    click.echo(f"Budget:  max {problem_spec.budget.max_solver_runs} solver runs, "
               f"{problem_spec.budget.max_wall_time_minutes} min wall time")

    # Create orchestrator and run
    orchestrator = Orchestrator(
        problem=problem_spec,
        output_dir=output_dir,
        knowledge_dir=kd,
    )

    result = asyncio.run(orchestrator.run())

    # Output result
    result_path = Path(output_dir) / "result.yaml"
    result.to_yaml(result_path)

    click.echo(f"\n{'='*60}")
    click.echo(f"Adaptation complete: {result.summary.rounds} rounds")
    click.echo(f"Initial cells: {result.summary.initial_mesh_cells}")
    click.echo(f"Final cells:   {result.summary.final_mesh_cells}")
    click.echo(f"Claims validated: {len(result.evidence_chain)}")
    click.echo(f"Result written to: {result_path}")


@main.command()
@click.option(
    "-p", "--problem",
    required=True,
    type=click.Path(exists=True),
    help="Path to problem.yaml to validate",
)
def validate(problem: str) -> None:
    """Validate a problem specification without running adaptation."""
    try:
        spec = ProblemSpec.from_yaml(problem)
        click.echo(f"Problem spec is valid.")
        click.echo(f"  Description: {spec.problem.description}")
        click.echo(f"  PDE type: {spec.problem.pde_type}")
        click.echo(f"  Solver: {spec.solver.type} @ {spec.solver.path}")
        click.echo(f"  Initial mesh: {spec.mesh.initial}")
        click.echo(f"  Budget: {spec.budget.max_solver_runs} runs, {spec.budget.max_wall_time_minutes}min")
        click.echo(f"  Target metrics: {spec.output.target_metrics}")
    except Exception as e:
        click.echo(f"Invalid problem spec: {e}", err=True)
        sys.exit(1)


if __name__ == "__main__":
    main()
