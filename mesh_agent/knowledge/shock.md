# Shock Wave Mesh Refinement

## When to use
For compressible flows (Ma > 0.3) where shock waves form. Shocks are near-discontinuities in the flow field and require very high mesh resolution normal to the shock front to avoid smearing and post-shock oscillations.

## Key Parameters

### Resolution Normal to Shock
- **Inviscid (Euler)**: 5-10 cells across the shock (with limiters)
- **Viscous (Navier-Stokes)**: 10-20 cells across the shock
- **With shock fitting**: 2-3 cells (specialized technique)

### Tangential Resolution
- Can be significantly coarser than normal resolution
- Ratio of 5:1 to 10:1 (tangential:normal) is typical

### Shock Detection
- **Pressure gradient**: |∇p| / p > threshold
- **Density gradient**: |∇ρ| / ρ > threshold  
- **Mach number gradient**: Detects shock location from Ma > 1 crossing
- **Normalized**: Use (δ · ∇p) / p where δ is local cell size (detects under-resolution)

## Adaptivity Strategies

### Shock Tracking
1. Detect shock location from current solution
2. Refine in a narrow band normal to the shock
3. Allow the band to follow shock movement

### Shock Capturing
1. Refine wherever gradients exceed threshold
2. Use solution-based indicator (gradient * cell_size^order)
3. Coarsen downstream of shock where gradients are smooth again

## Common Pitfalls
- Refining only the shock front → pre- and post-shock oscillations remain
- Using only density gradient → misses weak shocks in rarefaction regions
- Over-refinement normal to shock → unnecessarily small time steps
- Ignoring shock motion → refined region doesn't cover shock at all times
- Not coarsening after shock passes → mesh stays expensive

## Success Indicators
- Shock captured in 3-5 cells (sharp, no Gibbs oscillations)
- Post-shock values match Rankine-Hugoniot conditions within 1%
- Mass/momentum/energy conservation across shock
