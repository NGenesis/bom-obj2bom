# obj2bom
A command line tool to convert OBJ and associated MTL files to BOM (Binary Object/Material) file format.

## Command Line Usage
`obj2bom [output.bom] [input1.obj] [input2.obj] [...inputN.obj]`

## Features
- Convert OBJ/MTL files in to a compact binary representation for efficient storage, transport and parsing.
- Combine multi-part models consisting of many OBJ/MTL files into a single BOM file.
- Automatic conversion of quad face geometry into triangulated face geometry.
- Automatic indexing of geometry buffers.

## Known Issues
- Texture scaling and offset parameters in map_* are not currently parsed.
- Bump and displacement scaling parameters in bump/disp are not currently parsed.
- N-gon face geometry is not supported.
- Point/Line/Curve/Surface geometry is not supported.
- 3D texture coordinates are not supported.
