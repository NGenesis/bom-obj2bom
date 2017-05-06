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

## Comment Annotation Syntax (CAS)
Comment Annotation Syntax provides support for embedding non-standard BOM geometry and material properties into OBJ and MTL files using comment lines without breaking compliance with existing OBJ/MTL file specifications and parsers.

Note that the characters `<>[]` in the provided documentation are not considered literal syntax.
- `<property>` denotes a required property.
- `[property]` denotes an optional property.
- No indication denotes a required literal property.

### Syntax
`# :<vendor>: <property> [[-flag1] [value1] [value2] [...valueN]] [[-flag2] [value1] [value2] [...valueN]] [[...-flagN] [value1] [value2] [...valueN]]`

### BOM Material Properties (MTL-compatible)

#### Face Culling (cull_face)
##### Syntax:
`# :BOM: cull_face <none|front|back|all>`

##### Available options:
- `none`: Disables culling of faces, used to display double sided materials.
- `front`: Culls only front faces.
- `back`: Culls only back faces.
- `all`: Culls both front and back faces.

#### Light Map (lightmap)
##### Syntax:
`# :BOM: lightmap [-intensity <intensity>] <path>`

##### Available options:
- `-intensity <intensity_modifier>`: Intensity modifier to be applied to the light map, where a larger modifier produces a greater intensity.  Defaults to 1.
- `path`: File path of the light map texture file.

### BOM Geometry Properties (OBJ-compatible)

#### UV Channel 2 (vt2)
##### Syntax:
`# :BOM: vt2 <u> <v> [w]`

##### Available options:
- `u`: U component of a 2D texture coordinate.
- `v`: V component of a 2D texture coordinate.
- `w`: W component of a 3D texture coordinate.  Reserved for future use and is currently skipped by the parser.
