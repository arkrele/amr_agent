# Separation Bubble Mesh Refinement

## When to use
When the flow encounters an adverse pressure gradient causing boundary layer separation. Common in airfoils at high angle of attack, diffusers, backward-facing steps, and flow over surface protrusions.

## Key Parameters

### Separation Point
- **Detection**: Wall shear stress τ_w ≈ 0 (sign change)
- **Refinement**: Zone from slightly upstream of separation to the reattachment point
- **Streamwise resolution**: ~0.5-1% of separation bubble length

### Normal Resolution
- **Inside bubble**: At least 20 cells across the separated shear layer
- **Reattachment zone**: High resolution needed where the shear layer impinges on the wall
- **Reverse flow region**: Moderate resolution (flow is slow)

### Streamwise Resolution
- **Separation point**: Very fine (~0.1% chord for airfoils)
- **Middle of bubble**: Can be coarser
- **Reattachment**: Fine again (~0.5% chord)

## Adaptivity Indicators

### Primary Indicators
- Wall shear stress approaching zero
- Reverse flow detected (negative u-velocity near wall)
- High vorticity in the separated shear layer

### Secondary Indicators
- Pressure plateau (flat Cp distribution on surface)
- Increased turbulence kinetic energy in separation zone
- Recirculation zone detected from streamlines

## Common Pitfalls
- Coarse mesh at separation point → separation predicted too late
- Coarse mesh at reattachment → reattachment length wrong by >20%
- Not resolving the shear layer → bubble height and recirculation strength wrong
- Refining only the wall → miss the shear layer dynamics

## Success Indicators
- Separation and reattachment locations converge (< 1% change with further refinement)
- Velocity profiles inside bubble match reference data
- Reattachment length within 5% of experimental value
