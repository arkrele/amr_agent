# Boundary Layer Mesh Refinement

## When to use
When the flow has solid walls with viscous boundary layers. The near-wall region requires sufficient resolution to capture velocity gradients and accurately predict wall shear stress and heat transfer.

## Key Parameters

### First Layer Height
- **Laminar flow**: y+ ≈ 1 is sufficient
- **Transitional flow**: y+ < 1 recommended
- **Turbulent flow (wall-resolved)**: y+ < 1
- **Turbulent flow (wall-modeled)**: y+ ≈ 30-100

### Growth Rate
- **Conservative**: 1.05-1.10 (high accuracy, more cells)
- **Standard**: 1.10-1.20 (good balance)
- **Aggressive**: 1.20-1.30 (fewer cells, may miss features)

### Number of Layers
- Aim to cover the full boundary layer thickness with prism/hex layers
- Typically 10-20 layers for wall-resolved LES/DNS
- 3-5 layers for RANS with wall functions

## Common Pitfalls
- Too-large first layer height: misses viscous sublayer, underpredicts drag
- Too-small first layer: extremely small cells → CFL constraint → tiny time steps
- Growth rate > 1.3: sudden cell size jump creates numerical reflections
- Not enough layers to reach the freestream cell size: transition to isotropic mesh is too abrupt

## Success Indicators
- y+ distribution within target range across the entire wall surface
- Smooth transition from prism layers to isotropic mesh
- No negative volumes in highly curved regions (concave corners)
