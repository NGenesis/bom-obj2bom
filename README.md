# obj2bom
A command line tool to convert OBJ and associated MTL files to BOM (Binary Object/Material) file format.

## Command Line Usage
`obj2bom [output.bom] [input1.obj] [input2.obj] [...inputN.obj]`

## Features
- Convert OBJ/MTL files in to a compact binary representation for efficient storage, transport and parsing.
- Combine multi-part models consisting of many OBJ/MTL files into a single BOM file.
- Automatic conversion of quad face geometry into triangulated face geometry.
- Automatic indexing of geometry buffers.
- Comment annotation syntax for MTL provides support for embedding non-standard material properties into BOM file.

## Known Issues
- Texture scaling and offset parameters in map_* are not currently parsed.
- Bump and displacement scaling parameters in bump/disp are not currently parsed.
- N-gon face geometry is not supported.
- Point/Line/Curve/Surface geometry is not supported.
- 3D texture coordinates are not supported.

## MTL Comment Annotation Syntax
Comment annotation syntax provides support for embedding non-standard material properties into BOM file without breaking compliance with the MTL file specification.

### Syntax
`# :[vendor]: [property] [[-flag1] [value1] [value2] [...valueN]] [[-flag2] [value1] [value2] [...valueN]] [[...-flagN] [value1] [value2] [...valueN]] [value1] [value2] [...valueN]`

### BOM Material Properties

#### Face Culling (cull_face)
##### Syntax:
`# :BOM: cull_face [none|front|back|all]`

##### Available options:
- `none`: Disables culling of faces, used to display double sided materials.
- `front`: Culls only front faces.
- `back`: Culls only back faces.
- `all`: Culls both front and back faces.
