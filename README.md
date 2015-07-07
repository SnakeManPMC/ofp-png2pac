# ofp-png2pac

Operation Flashpoint png2pac by feersum.endjinn

Converts TGA textures to Operation Flashpoint PAA format.

Usage: ..\release\png2pac -type source destination

```
  -type Destination file type, must be one of following:
        dxt1    DXT1 compression
        dxt1a   DXT1 compression with 1-bit alpha channel
        4444    RGBA 4:4:4:4
        1555    RGBA 5:5:5:1
        ia88    IA88, black&white texture with alpha channel
  source        Specifies 32-bit TGA image file to be converted
  destination   Specifies OFP texture file to be written
```

Example: ..\release\png2pac -5551 texture.tga texture.pac

If executed with only one parameter, TGA file specified by parameter is converted with destination type autodetection.

png2pac (C) 2004 feersum.endjinn
