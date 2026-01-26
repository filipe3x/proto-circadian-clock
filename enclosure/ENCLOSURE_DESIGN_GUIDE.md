# P10 Enclosure Design Guide

## Overview

Enclosure design for the Proto Circadian Clock P10 32x16 LED panel with Matrix Portal S3.

## Hardware Dimensions

### P10 Panel (Standard 32x16)
| Dimension | Value | Notes |
|-----------|-------|-------|
| Width | 320mm | 32 pixels × 10mm pitch |
| Height | 160mm | 16 pixels × 10mm pitch |
| Depth | ~15-20mm | Varies by manufacturer |
| Active Area | 320mm × 160mm | Full panel is active |
| Frame | ~5mm | Metal frame around panel |
| Mounting Holes | M3 | Typically 4 corners |

### Matrix Portal S3 (Adafruit)
| Dimension | Value | Notes |
|-----------|-------|-------|
| Width | ~52mm | |
| Height | ~23mm | |
| Depth | ~10mm | Including components |
| Connector | HUB75 | Plugs directly into panel |

### Power Connector
| Component | Specification |
|-----------|---------------|
| Type | USB-C |
| Input | 5V, 2-5A |
| Max Power | 25W |

## Design Requirements

### Functional Requirements
- [ ] Front opening for LED display (320mm × 160mm)
- [ ] Access to USB-C port for power
- [ ] Access to UP/DOWN buttons (if exposed)
- [ ] Ventilation for heat dissipation
- [ ] Wall mount option (keyhole slots or VESA)
- [ ] Optional: desk stand

### Mechanical Requirements
- [ ] Secure panel mounting (screws or clips)
- [ ] Matrix Portal S3 protection
- [ ] Cable management for power cable
- [ ] Light blocking around display edges

## Fusion 360 Workflow

### 1. Create New Design
```
File → New Design
Modify → Change Active Units → Millimeters
```

### 2. Create Parameters (recommended)
```
Modify → Change Parameters → Add User Parameters:
- panel_width = 320mm
- panel_height = 160mm
- panel_depth = 18mm
- wall_thickness = 2.5mm
- tolerance = 0.3mm
```

### 3. Main Body Sketch
1. Create Sketch on XY plane
2. Draw rectangle: `panel_width + 2*wall_thickness` × `panel_height + 2*wall_thickness`
3. Finish Sketch

### 4. Extrude Main Body
```
Create → Extrude
- Profile: rectangle
- Distance: panel_depth + 15mm (space for electronics)
- Operation: New Body
```

### 5. Shell Interior
```
Modify → Shell
- Faces to Remove: back face
- Inside Thickness: wall_thickness
```

### 6. Front Display Cutout
1. Create Sketch on front face
2. Draw rectangle for display area (320mm × 160mm, centered)
3. Extrude → Cut through front wall

### 7. Add Features
- USB-C port cutout (side)
- Ventilation slots (back/sides)
- Mounting features
- Fillets on edges (2mm)

## 3D Printing Considerations

### Material Recommendations
| Material | Pros | Cons | Recommended |
|----------|------|------|-------------|
| PLA | Easy to print, cheap | Heat sensitive | Prototyping |
| PETG | Heat resistant, strong | Stringing | Production |
| ABS | Heat resistant, paintable | Warping, fumes | If painting |

### Print Settings
```
Layer Height: 0.2mm (quality) or 0.28mm (speed)
Wall Count: 3-4 walls
Infill: 15-20%
Supports: As needed for overhangs >45°
```

### Design for Printing
- Minimum wall thickness: 1.5mm (2mm+ recommended)
- Hole tolerance: +0.3mm for M3 screws
- Snap-fit tolerance: +0.2mm
- Avoid overhangs >45° or add chamfers
- Consider print orientation for strength

## Two-Part Design (Recommended)

For easier printing and assembly:

### Part 1: Front Frame
- Display opening
- Panel mounting clips/screws
- Decorative front face

### Part 2: Back Cover
- Electronics enclosure
- USB-C port access
- Ventilation
- Wall mount features

### Assembly
- Snap-fit clips or M3 screws
- Gasket/light seal between parts

## File Organization

```
enclosure/
├── ENCLOSURE_DESIGN_GUIDE.md (this file)
├── fusion360/
│   └── (save .f3d files here)
├── stl/
│   ├── front_frame.stl
│   └── back_cover.stl
└── step/
    └── enclosure_assembly.step
```

## Export Checklist

Before exporting for print:
- [ ] Check wall thickness (Analysis → Section Analysis)
- [ ] Verify tolerances for hardware fit
- [ ] Add fillets to sharp edges
- [ ] Orient for optimal print direction
- [ ] Export as STL (High refinement)

## Resources

- [Fusion 360 Keyboard Shortcuts](https://www.autodesk.com/shortcuts/fusion-360)
- [3D Printing Design Guide](https://www.hubs.com/knowledge-base/how-design-parts-fdm-3d-printing/)
- [Matrix Portal S3 Dimensions](https://learn.adafruit.com/adafruit-matrixportal-s3)

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 0.1 | 2026-01-26 | Initial design guide |
