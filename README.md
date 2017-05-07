# obj2bom
A command line tool to convert OBJ and associated MTL files to BOM (Binary Object/Material) file format.

## Command Line Usage
`obj2bom <output.bom> <input1.obj> [input2.obj] [...inputN.obj]`

## Features
- Convert OBJ/MTL files in to a compact binary representation for efficient storage, transport and parsing.
- Combine multi-part models consisting of many OBJ/MTL files into a single BOM file.
- Automatic conversion of quad face geometry into triangulated face geometry.
- Automatic indexing of geometry buffers.
- Supports two UV channels and lightmap channel.
- Comment Annotation Syntax for OBJ and MTL provides support for embedding BOM properties into OBJ/MTL files without breaking existing parsers.

## Known Limitations
- N-gon face geometry is not supported.
- Point/Line/Curve/Surface geometry is not supported.
- 3D texture coordinates are not supported.

## Comment Annotation Syntax (CAS)
Comment Annotation Syntax provides support for embedding non-standard BOM geometry and material properties into OBJ and MTL files using comment lines without breaking compliance with existing OBJ/MTL file specifications and parsers.

Note that the characters `<>[]|` in the provided documentation are not considered literal syntax.
- `<property>` denotes a required property.
- `[property]` denotes an optional property.
- `<property1|property2|property3>` or `[property1|property2|property3]` denotes a required or optional property which can be one of several available properties.
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
`# :BOM: lightmap [-o <offset_u> <offset_v>] [-s <scale_u> <scale_v>] [-intensity <intensity_modifier>] <path>`

##### Available options:
- `-o <offset_u> <offset_v>`: Specifies UV offset of the light map texture.
- `-s <scale_u> <scale_v>`: Specifies UV scaling of the light map texture.
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

## See also
[bom-three.js](https://github.com/NGenesis/bom-three.js) - A javascript loader to parse and load BOM (Binary Object/Material) files in three.js.
