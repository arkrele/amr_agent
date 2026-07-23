base_1atm_no_hardconstraints_unrefined

This package is the unrefined-grid 1 atm program case after the hard-coded
mesh/index constraints were replaced by coordinate-driven mesh configuration.
Large simulation outputs are intentionally excluded. The outputdata directory
tree is kept empty so the executable can write results after unpacking.

Contents:
- streamer.cu and related C/CUDA source files
- hfile/ headers
- inputdata/ input tables and mesh files
- makefile
- a.out, the executable built in the source case
- empty outputdata/ subdirectories

Build:
  make

Run:
  ./a.out

Notes:
- This package was prepared from AI_grid_agent_work/base_1atm.
- It uses the original, unrefined grid size, not the NZx2 refined grid.
- It does not include historical outputdata files or long-run logs.
- The package keeps the coordinate-driven mesh configuration file
  inputdata/mesh_physics.cfg that was added during the hard-code cleanup.
