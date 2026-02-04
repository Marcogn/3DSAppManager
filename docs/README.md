# 📚 Technical Documentation - 3DS Fast Uninstall

This folder contains detailed technical documentation for the project.

## 📄 Available Documents

### CHARACTER_CORRUPTION_FIX.md
Details about UTF-16 character handling and text sanitization.

**Content**:
- Character corruption issues
- UTF-16 to UTF-8 conversion
- Sanitization implementation
- Testing results

### FLICKERING_FIX.md
Initial console-based rendering fixes.

**Content**:
- Console buffer flickering issues
- Screen clear strategies
- Early solutions attempted

### FLICKERING_FIX_v3_FINAL.md
Complete migration to citro2d/citro3d hardware rendering.

**Content**:
- Why citro2d/citro3d was necessary
- Migration process details
- Implementation specifics
- Performance improvements

### RENDERING_TROUBLESHOOTING.md
Comprehensive guide for graphics troubleshooting.

**Content**:
- Common rendering issues
- Debugging techniques
- Performance optimization tips

### PROJECT_SUMMARY.md
Complete project overview and architecture.

**Content**:
- Project goals
- Feature list
- Technical architecture
- Development history

## 🎯 Getting Started

If you're a new contributor or want to understand the project:

1. First read the [README.md](../README.md) in the project root
2. For graphics system, see [FLICKERING_FIX_v3_FINAL.md](FLICKERING_FIX_v3_FINAL.md)
3. For character handling, see [CHARACTER_CORRUPTION_FIX.md](CHARACTER_CORRUPTION_FIX.md)
4. For troubleshooting, see [RENDERING_TROUBLESHOOTING.md](RENDERING_TROUBLESHOOTING.md)

## 🔧 Build and Development

For information on how to compile the project, consult the "Building from Source" section in the main README.

## ✅ Project Status

**Last Update**: 2026-02-04  
**Version**: v4.0.0  
**Build Status**: ✅ Success (0 errors, minor warnings)  
**Graphics System**: ✅ citro2d/citro3d (hardware-accelerated)  
**3DS Compatibility**: ✅ Confirmed (Old 3DS, New 3DS, 2DS)  
**Ready for**: Production use on real hardware
