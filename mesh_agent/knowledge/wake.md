# Wake Region Mesh Refinement

## When to use
When the flow involves bluff bodies producing vortex streets (e.g., cylinder, flat plate normal to flow, bridge deck sections). The wake region contains coherent vortex structures that must be resolved to accurately predict drag and Strouhal number.

## Key Parameters

### Refinement Zone
- **Streamwise extent**: 10-20 body diameters downstream (longer for high Re)
- **Transverse extent**: 2-5 body diameters (wider at downstream end)
- **Shape**: Tapered box or cone starting from the body trailing edge

### Cell Size
- **Near body**: Same as boundary layer outer cell size (smooth transition)
- **Wake core (0-5D)**: ~D/50 to D/100 (resolve individual vortices)
- **Far wake (5-20D)**: Gradually coarsen to ~D/20

### Growth Strategy
- Linear growth in streamwise direction (constant stretching ratio)
- Linear growth in transverse direction (wider → coarser)

## Common Pitfalls
- Wake zone too narrow: vortex street hits refinement boundary → reflections
- Refinement stops too early: far wake still affects base pressure recovery
- Abrupt size transition at wake boundary: numerical reflections contaminate solution
- Uniform refinement of entire wake: wastes cells in regions with smooth flow

## Success Indicators
- Strouhal number matches experimental/reference values within 5%
- Vortex street persists without artificial dissipation for >10 body diameters
- Base pressure coefficient converges with mesh refinement
