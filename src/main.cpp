#include <iostream>
#include <sstream>
#include <memory>
#include <string>
#include <fstream>
#include <vector>
#include <cstdint>
#include <map>
#include <set>
#include <type_traits>
#include <regex>

template <typename T>
auto BitmaskFlag(T flag)
{
	return static_cast<typename std::underlying_type<T>::type>(flag);

}

static const std::uint8_t BOM_DATA_VERSION = 1;

typedef std::uint16_t obj_index_t;

// BOM
enum class FileDataAttribute : std::uint16_t
{
	NONE = 1 << 0,
	MATERIAL_LIBRARY = 1 << 1

};

enum class GroupDataAttribute : std::uint16_t
{
	NONE = 1 << 0,
	NAME = 1 << 1,
	INDEX = 1 << 2,
	SMOOTHING = 1 << 3,
	MATERIAL = 1 << 4

};

enum class ObjectDataAttribute : std::uint16_t
{
	NONE = 1 << 0,
	NAME = 1 << 1,
	GEOMETRY = 1 << 2

};

enum class GeometryDataAttribute : std::uint16_t
{
	NONE = 1 << 0,
	NORMAL = 1 << 1,
	UV = 1 << 2

};

enum class MaterialDataAttribute : std::uint32_t
{
	NONE = 1 << 0,
	ILLUMINATION_MODEL = 1 << 1,
	SPECULAR_EXPONENT = 1 << 2,
	OPTICAL_DENSITY = 1 << 3,
	DISSOLVE = 1 << 4,
	TRANSMISSION_FILTER = 1 << 5,
	AMBIENT_REFLECTANCE = 1 << 6,
	DIFFUSE_REFLECTANCE = 1 << 7,
	SPECULAR_REFLECTANCE = 1 << 8,
	EMISSIVE_REFLECTANCE = 1 << 9,
	AMBIENT_MAP = 1 << 10,
	DIFFUSE_MAP = 1 << 11,
	SPECULAR_MAP = 1 << 12,
	EMISSIVE_MAP = 1 << 13,
	DISSOLVE_MAP = 1 << 14,
	BUMP_MAP = 1 << 15,
	DISPLACEMENT_MAP = 1 << 16

};

enum class MapDataAttribute : std::uint16_t
{
	NONE = 1 << 0,
	PATH = 1 << 1,
	SCALE = 1 << 2,
	OFFSET = 1 << 3,
	BUMP_SCALE = 1 << 4,
	DISPLACEMENT_SCALE = 1 << 5

};

struct alignas(1) obj_vector3_t
{
	float x, y, z;

};

struct alignas(1) obj_vector2_t
{
	float x, y;

};

struct alignas(1) obj_face3_t
{
	obj_index_t a, b, c;

};

struct alignas(1) mtl_color_t
{
	float r, g, b;

};

struct mtl_map_t
{
	decltype(BitmaskFlag(MapDataAttribute::NONE)) attributes;
	std::string path;
	obj_vector2_t scale, offset;
	union
	{
		float bumpScale;
		float displacementScale;

	};

};

// MTL Properties
struct mtl_material_t
{
	std::uint16_t id;
	std::string name;
	decltype(BitmaskFlag(MaterialDataAttribute::NONE)) attributes;
	std::uint8_t illuminationModel;
	float specularExponent, opticalDensity, dissolve;
	mtl_color_t transmissionFilter, ambientReflectance, diffuseReflectance, specularReflectance, emissiveReflectance;
	mtl_map_t ambientMap, diffuseMap, specularMap, emissiveMap, dissolveMap, bumpMap, displacementMap;

};

struct mtl_state_t
{
	std::string name;
	std::map<std::uint16_t, std::shared_ptr<mtl_material_t>> materials;

};

// OBJ Properties
struct obj_group_t
{
	std::uint16_t materialId;
	std::string name, materialName;
	std::uint8_t smoothing = 1;
	std::vector<obj_face3_t> faces;

};

struct obj_object_t
{
	std::uint32_t containerId;
	std::string name;
	std::vector<obj_vector3_t> positions;
	std::vector<obj_vector3_t> normals;
	std::vector<obj_vector2_t> uvs;
	std::vector<std::shared_ptr<obj_group_t>> groups;

};

struct obj_state_t
{
	std::uint16_t materialId;
	std::string materialFileName, materialName;
	std::vector<std::shared_ptr<obj_object_t>> objects;
	std::shared_ptr<mtl_state_t> mtlState;
	std::uint8_t smoothing = 1;

};

std::vector<std::shared_ptr<obj_state_t>> objStates;
std::vector<std::shared_ptr<mtl_state_t>> mtlStates;
std::uint16_t maxMaterialId = 0;

bool createIndexedGeometry = true;
bool logWarnings = true;
bool logErrors = true;

bool WriteBOM(const std::string &bomFilePath)
{
	std::cout << "Writing BOM '" << bomFilePath << "'..." << std::endl;

	// BOM Writer
	std::ofstream bomFile;
	bomFile.open(bomFilePath, std::ios::out | std::ios::binary);
	if(!bomFile.is_open()) return false;

	// File Signature
	std::string fileSignature = "BOM";
	bomFile.write(fileSignature.c_str(), fileSignature.size());

	// Version
	std::uint8_t version = BOM_DATA_VERSION;
	bomFile.write(reinterpret_cast<char*>(&version), sizeof(version));

	// File Data Attributes
	auto fileAttributes = BitmaskFlag(FileDataAttribute::NONE);
	if(!mtlStates.empty() && !mtlStates[0]->materials.empty()) fileAttributes |= BitmaskFlag(FileDataAttribute::MATERIAL_LIBRARY);
	bomFile.write(reinterpret_cast<char*>(&fileAttributes), sizeof(fileAttributes));

	if(fileAttributes & BitmaskFlag(FileDataAttribute::MATERIAL_LIBRARY))
	{
		// Material Count
		std::uint16_t materialCount = 0;
		for(const auto &mtlState : mtlStates) materialCount += mtlState->materials.size();
		bomFile.write(reinterpret_cast<char*>(&materialCount), sizeof(materialCount));

		for(const auto &mtlState : mtlStates)
		{
			for(const auto &material : mtlState->materials)
			{
				// Material Data Attributes
				auto materialAttributes = material.second->attributes;
				bomFile.write(reinterpret_cast<char*>(&materialAttributes), sizeof(materialAttributes));

				// Material Name
				std::uint16_t materialNameLength = material.second->name.size();
				bomFile.write(reinterpret_cast<char*>(&materialNameLength), sizeof(materialNameLength));
				bomFile.write(material.second->name.c_str(), materialNameLength);

				// Illumination Model (illum)
				if(materialAttributes & BitmaskFlag(MaterialDataAttribute::ILLUMINATION_MODEL)) bomFile.write(reinterpret_cast<char*>(&material.second->illuminationModel), sizeof(material.second->illuminationModel));

				// Specular Exponent (Ns)
				if(materialAttributes & BitmaskFlag(MaterialDataAttribute::SPECULAR_EXPONENT)) bomFile.write(reinterpret_cast<char*>(&material.second->specularExponent), sizeof(material.second->specularExponent));

				// Optical Density (Ni)
				if(materialAttributes & BitmaskFlag(MaterialDataAttribute::OPTICAL_DENSITY)) bomFile.write(reinterpret_cast<char*>(&material.second->opticalDensity), sizeof(material.second->opticalDensity));

				// Dissolve (d / [1 - Tr])
				if(materialAttributes & BitmaskFlag(MaterialDataAttribute::DISSOLVE)) bomFile.write(reinterpret_cast<char*>(&material.second->dissolve), sizeof(material.second->dissolve));

				// Transmission Filter (Tf)
				if(materialAttributes & BitmaskFlag(MaterialDataAttribute::TRANSMISSION_FILTER)) bomFile.write(reinterpret_cast<char*>(&material.second->transmissionFilter), sizeof(material.second->transmissionFilter));

				// Ambient Reflectance (Ka)
				if(materialAttributes & BitmaskFlag(MaterialDataAttribute::AMBIENT_REFLECTANCE)) bomFile.write(reinterpret_cast<char*>(&material.second->ambientReflectance), sizeof(material.second->ambientReflectance));

				// Diffuse Reflectance (Kd)
				if(materialAttributes & BitmaskFlag(MaterialDataAttribute::DIFFUSE_REFLECTANCE)) bomFile.write(reinterpret_cast<char*>(&material.second->diffuseReflectance), sizeof(material.second->diffuseReflectance));

				// Specular Reflectance (Ks)
				if(materialAttributes & BitmaskFlag(MaterialDataAttribute::SPECULAR_REFLECTANCE)) bomFile.write(reinterpret_cast<char*>(&material.second->specularReflectance), sizeof(material.second->specularReflectance));

				// Emissive Reflectance (Ke)
				if(materialAttributes & BitmaskFlag(MaterialDataAttribute::EMISSIVE_REFLECTANCE)) bomFile.write(reinterpret_cast<char*>(&material.second->emissiveReflectance), sizeof(material.second->emissiveReflectance));

				// Ambient Map (map_Ka)
				if(materialAttributes & BitmaskFlag(MaterialDataAttribute::AMBIENT_MAP))
				{
					const auto map = &material.second->ambientMap;

					// Map Data Attributes
					auto mapAttributes = map->attributes;
					bomFile.write(reinterpret_cast<char*>(&mapAttributes), sizeof(mapAttributes));
					
					// Map Path
					if(mapAttributes & BitmaskFlag(MapDataAttribute::PATH))
					{
						std::uint16_t pathCount = map->path.size();
						bomFile.write(reinterpret_cast<char*>(&pathCount), sizeof(pathCount));
						bomFile.write(map->path.c_str(), pathCount);

					}
					
					// Map Scale
					if(mapAttributes & BitmaskFlag(MapDataAttribute::SCALE)) bomFile.write(reinterpret_cast<char*>(&map->scale), sizeof(map->scale));
					
					// Map Offset
					if(mapAttributes & BitmaskFlag(MapDataAttribute::SCALE)) bomFile.write(reinterpret_cast<char*>(&map->offset), sizeof(map->offset));

				}
				
				// Diffuse Map (map_Kd)
				if(materialAttributes & BitmaskFlag(MaterialDataAttribute::DIFFUSE_MAP))
				{
					const auto map = &material.second->diffuseMap;

					// Map Data Attributes
					auto mapAttributes = map->attributes;
					bomFile.write(reinterpret_cast<char*>(&mapAttributes), sizeof(mapAttributes));
					
					// Map Path
					if(mapAttributes & BitmaskFlag(MapDataAttribute::PATH))
					{
						std::uint16_t pathCount = map->path.size();
						bomFile.write(reinterpret_cast<char*>(&pathCount), sizeof(pathCount));
						bomFile.write(map->path.c_str(), pathCount);

					}
					
					// Map Scale
					if(mapAttributes & BitmaskFlag(MapDataAttribute::SCALE)) bomFile.write(reinterpret_cast<char*>(&map->scale), sizeof(map->scale));
					
					// Map Offset
					if(mapAttributes & BitmaskFlag(MapDataAttribute::SCALE)) bomFile.write(reinterpret_cast<char*>(&map->offset), sizeof(map->offset));

				}
				
				// Specular Map (map_Kd)
				if(materialAttributes & BitmaskFlag(MaterialDataAttribute::SPECULAR_MAP))
				{
					const auto map = &material.second->specularMap;

					// Map Data Attributes
					auto mapAttributes = map->attributes;
					bomFile.write(reinterpret_cast<char*>(&mapAttributes), sizeof(mapAttributes));
					
					// Map Path
					if(mapAttributes & BitmaskFlag(MapDataAttribute::PATH))
					{
						std::uint16_t pathCount = map->path.size();
						bomFile.write(reinterpret_cast<char*>(&pathCount), sizeof(pathCount));
						bomFile.write(map->path.c_str(), pathCount);

					}
					
					// Map Scale
					if(mapAttributes & BitmaskFlag(MapDataAttribute::SCALE)) bomFile.write(reinterpret_cast<char*>(&map->scale), sizeof(map->scale));
					
					// Map Offset
					if(mapAttributes & BitmaskFlag(MapDataAttribute::SCALE)) bomFile.write(reinterpret_cast<char*>(&map->offset), sizeof(map->offset));

				}
				
				// Emissive Map (map_Ke)
				if(materialAttributes & BitmaskFlag(MaterialDataAttribute::EMISSIVE_MAP))
				{
					const auto map = &material.second->emissiveMap;

					// Map Data Attributes
					auto mapAttributes = map->attributes;
					bomFile.write(reinterpret_cast<char*>(&mapAttributes), sizeof(mapAttributes));
					
					// Map Path
					if(mapAttributes & BitmaskFlag(MapDataAttribute::PATH))
					{
						std::uint16_t pathCount = map->path.size();
						bomFile.write(reinterpret_cast<char*>(&pathCount), sizeof(pathCount));
						bomFile.write(map->path.c_str(), pathCount);

					}
					
					// Map Scale
					if(mapAttributes & BitmaskFlag(MapDataAttribute::SCALE)) bomFile.write(reinterpret_cast<char*>(&map->scale), sizeof(map->scale));
					
					// Map Offset
					if(mapAttributes & BitmaskFlag(MapDataAttribute::SCALE)) bomFile.write(reinterpret_cast<char*>(&map->offset), sizeof(map->offset));

				}
				
				// Dissolve Map (map_d)
				if(materialAttributes & BitmaskFlag(MaterialDataAttribute::DISSOLVE_MAP))
				{
					const auto map = &material.second->dissolveMap;

					// Map Data Attributes
					auto mapAttributes = map->attributes;
					bomFile.write(reinterpret_cast<char*>(&mapAttributes), sizeof(mapAttributes));
					
					// Map Path
					if(mapAttributes & BitmaskFlag(MapDataAttribute::PATH))
					{
						std::uint16_t pathCount = map->path.size();
						bomFile.write(reinterpret_cast<char*>(&pathCount), sizeof(pathCount));
						bomFile.write(map->path.c_str(), pathCount);

					}
					
					// Map Scale
					if(mapAttributes & BitmaskFlag(MapDataAttribute::SCALE)) bomFile.write(reinterpret_cast<char*>(&map->scale), sizeof(map->scale));
					
					// Map Offset
					if(mapAttributes & BitmaskFlag(MapDataAttribute::SCALE)) bomFile.write(reinterpret_cast<char*>(&map->offset), sizeof(map->offset));

				}
				
				// Bump Map (map_bump / bump)
				if(materialAttributes & BitmaskFlag(MaterialDataAttribute::BUMP_MAP))
				{
					const auto map = &material.second->bumpMap;

					// Map Data Attributes
					auto mapAttributes = map->attributes;
					bomFile.write(reinterpret_cast<char*>(&mapAttributes), sizeof(mapAttributes));
					
					// Map Path
					if(mapAttributes & BitmaskFlag(MapDataAttribute::PATH))
					{
						std::uint16_t pathCount = map->path.size();
						bomFile.write(reinterpret_cast<char*>(&pathCount), sizeof(pathCount));
						bomFile.write(map->path.c_str(), pathCount);

					}
					
					// Map Scale
					if(mapAttributes & BitmaskFlag(MapDataAttribute::SCALE)) bomFile.write(reinterpret_cast<char*>(&map->scale), sizeof(map->scale));
					
					// Map Offset
					if(mapAttributes & BitmaskFlag(MapDataAttribute::SCALE)) bomFile.write(reinterpret_cast<char*>(&map->offset), sizeof(map->offset));
					
					// Map Bump Scale
					if(mapAttributes & BitmaskFlag(MapDataAttribute::BUMP_SCALE)) bomFile.write(reinterpret_cast<char*>(&map->bumpScale), sizeof(map->bumpScale));

				}
				
				// Displacement Map (map_disp / disp)
				if(materialAttributes & BitmaskFlag(MaterialDataAttribute::DISPLACEMENT_MAP))
				{
					const auto map = &material.second->displacementMap;

					// Map Data Attributes
					auto mapAttributes = map->attributes;
					bomFile.write(reinterpret_cast<char*>(&mapAttributes), sizeof(mapAttributes));
					
					// Map Path
					if(mapAttributes & BitmaskFlag(MapDataAttribute::PATH))
					{
						std::uint16_t pathCount = map->path.size();
						bomFile.write(reinterpret_cast<char*>(&pathCount), sizeof(pathCount));
						bomFile.write(map->path.c_str(), pathCount);

					}
					
					// Map Scale
					if(mapAttributes & BitmaskFlag(MapDataAttribute::SCALE)) bomFile.write(reinterpret_cast<char*>(&map->scale), sizeof(map->scale));
					
					// Map Offset
					if(mapAttributes & BitmaskFlag(MapDataAttribute::SCALE)) bomFile.write(reinterpret_cast<char*>(&map->offset), sizeof(map->offset));
					
					// Map Displacement Scale
					if(mapAttributes & BitmaskFlag(MapDataAttribute::BUMP_SCALE)) bomFile.write(reinterpret_cast<char*>(&map->displacementScale), sizeof(map->displacementScale));

				}

			}

		}

	}

	// Object Count
	std::uint16_t objectCount = 0;
	for(const auto &objState : objStates) objectCount += objState->objects.size();
	bomFile.write(reinterpret_cast<char*>(&objectCount), sizeof(objectCount));

	for(const auto &objState : objStates)
	{
		for(const auto &object : objState->objects)
		{
			// Object Data Attributes
			auto objectAttributes = BitmaskFlag(ObjectDataAttribute::NONE);
			if(!object->name.empty()) objectAttributes |= BitmaskFlag(ObjectDataAttribute::NAME);
			if(!object->positions.empty()) objectAttributes |= BitmaskFlag(ObjectDataAttribute::GEOMETRY);
			bomFile.write(reinterpret_cast<char*>(&objectAttributes), sizeof(objectAttributes));
			
			// Container ID
			bomFile.write(reinterpret_cast<char*>(&object->containerId), sizeof(object->containerId));

			// Object Name
			if(objectAttributes & BitmaskFlag(ObjectDataAttribute::NAME))
			{
				std::uint16_t objectNameLength = object->name.size();
				bomFile.write(reinterpret_cast<char*>(&objectNameLength), sizeof(objectNameLength));
				bomFile.write(object->name.c_str(), objectNameLength);

			}

			if(objectAttributes & BitmaskFlag(ObjectDataAttribute::GEOMETRY))
			{
				// Geometry Data Attributes
				auto geometryAttributes = BitmaskFlag(GeometryDataAttribute::NONE);
				if(!object->normals.empty()) geometryAttributes |= BitmaskFlag(GeometryDataAttribute::NORMAL);
				if(!object->uvs.empty()) geometryAttributes |= BitmaskFlag(GeometryDataAttribute::UV);
				bomFile.write(reinterpret_cast<char*>(&geometryAttributes), sizeof(geometryAttributes));

				// Vertex Count
				std::uint32_t vertexCount = object->positions.size();
				bomFile.write(reinterpret_cast<char*>(&vertexCount), sizeof(vertexCount));

				// Vertex Positions
				bomFile.write(reinterpret_cast<char*>(object->positions.data()), sizeof(float) * vertexCount * 3);

				// Vertex Normals
				if(geometryAttributes & BitmaskFlag(GeometryDataAttribute::NORMAL)) bomFile.write(reinterpret_cast<char*>(object->normals.data()), sizeof(float) * vertexCount * 3);

				// Vertex UVs
				if(geometryAttributes & BitmaskFlag(GeometryDataAttribute::UV)) bomFile.write(reinterpret_cast<char*>(object->uvs.data()), sizeof(float) * vertexCount * 2);

			}

			// Group Count
			std::uint16_t groupCount = object->groups.size();
			bomFile.write(reinterpret_cast<char*>(&groupCount), sizeof(groupCount));

			for(const auto &group : object->groups)
			{
				// Group Data Attributes
				auto groupAttributes = BitmaskFlag(GroupDataAttribute::NONE);
				if(!group->name.empty()) groupAttributes |= BitmaskFlag(GroupDataAttribute::NAME);
				if(!group->faces.empty()) groupAttributes |= BitmaskFlag(GroupDataAttribute::INDEX);
				if(group->smoothing >= 0) groupAttributes |= BitmaskFlag(GroupDataAttribute::SMOOTHING);
				if(!group->materialName.empty() && objState->mtlState && objState->mtlState->materials.find(group->materialId) != objState->mtlState->materials.end()) groupAttributes |= BitmaskFlag(GroupDataAttribute::MATERIAL);
				bomFile.write(reinterpret_cast<char*>(&groupAttributes), sizeof(groupAttributes));

				// Group Name
				if(groupAttributes & BitmaskFlag(GroupDataAttribute::NAME))
				{
					std::uint16_t groupNameLength = group->name.size();
					bomFile.write(reinterpret_cast<char*>(&groupNameLength), sizeof(groupNameLength));
					bomFile.write(group->name.c_str(), groupNameLength);

				}

				if(objectAttributes & BitmaskFlag(ObjectDataAttribute::GEOMETRY))
				{
					// Indices
					if(groupAttributes & BitmaskFlag(GroupDataAttribute::INDEX))
					{
						// Index Count
						std::uint32_t indexCount = group->faces.size() * 3;
						bomFile.write(reinterpret_cast<char*>(&indexCount), sizeof(indexCount));

						// Indices
						bomFile.write(reinterpret_cast<char*>(group->faces.data()), sizeof(obj_index_t) * indexCount);

					}

					// Smoothing
					if(groupAttributes & BitmaskFlag(GroupDataAttribute::SMOOTHING)) bomFile.write(reinterpret_cast<char*>(&group->smoothing), sizeof(group->smoothing));

					if(groupAttributes & BitmaskFlag(GroupDataAttribute::MATERIAL))
					{
						// Material ID
						std::uint16_t materialId = objState->mtlState->materials[group->materialId]->id;
						bomFile.write(reinterpret_cast<char*>(&materialId), sizeof(materialId));

					}

				}

			}

		}

	}

	bomFile.close();
	return true;

}

bool ReadMTL(const std::string &relativePath, std::shared_ptr<obj_state_t> objState)
{
	// MTL Parser
	std::ifstream mtlFile(relativePath + objState->materialFileName);
	if(!mtlFile.is_open()) return false;

	auto mtlState = std::make_shared<mtl_state_t>();
	mtlStates.push_back(mtlState);
	objState->mtlState = mtlState;

	mtlState->name = objState->materialFileName;

	std::shared_ptr<mtl_material_t> material = std::make_shared<mtl_material_t>();
	material->id = maxMaterialId++;

	std::cout << "Parsing MTL '" << objState->materialFileName << "'..." << std::endl;
	std::string line;
	int lineNo = -1;

	while(std::getline(mtlFile, line))
	{
		++lineNo;
		std::istringstream iss(line);
		//std::cout << line << "\n";
		if(line.empty()) continue;

		std::string entryType;
		if(!(iss >> entryType))
		{
			if(logErrors) std::cout << "ERROR: [" << objState->materialFileName << ":" << lineNo << "] Failed to parse entry type." << std::endl;
			break;

		}

		if(entryType == "newmtl")
		{
			if(!mtlState->materials.empty())
			{
				material = std::make_shared<mtl_material_t>();
				material->id = maxMaterialId++;

			}

			if(!(iss >> material->name))
			{
				if(logErrors) std::cout << "ERROR: [" << objState->materialFileName << ":" << lineNo << "] Syntax error while parsing entry type '" << entryType << "'" << std::endl;
				break;

			}

			mtlState->materials.insert(std::make_pair(material->id, material));
			//std::cout << "Material: " << material->name << std::endl;

		}
		else if(entryType == "illum")
		{
			if(!(iss >> material->illuminationModel))
			{
				if(logErrors) std::cout << "ERROR: [" << objState->materialFileName << ":" << lineNo << "] Syntax error while parsing entry type '" << entryType << "'" << std::endl;
				break;

			}

			material->attributes |= BitmaskFlag(MaterialDataAttribute::ILLUMINATION_MODEL);

		}
		else if(entryType == "Ns")
		{
			if(!(iss >> material->specularExponent))
			{
				if(logErrors) std::cout << "ERROR: [" << objState->materialFileName << ":" << lineNo << "] Syntax error while parsing entry type '" << entryType << "'" << std::endl;
				break;

			}

			material->attributes |= BitmaskFlag(MaterialDataAttribute::SPECULAR_EXPONENT);

		}
		else if(entryType == "Ni")
		{
			if(!(iss >> material->opticalDensity))
			{
				if(logErrors) std::cout << "ERROR: [" << objState->materialFileName << ":" << lineNo << "] Syntax error while parsing entry type '" << entryType << "'" << std::endl;
				break;

			}

			material->attributes |= BitmaskFlag(MaterialDataAttribute::OPTICAL_DENSITY);

		}
		else if(entryType == "d")
		{
			if(!(iss >> material->dissolve))
			{
				if(logErrors) std::cout << "ERROR: [" << objState->materialFileName << ":" << lineNo << "] Syntax error while parsing entry type '" << entryType << "'" << std::endl;
				break;

			}

			material->attributes |= BitmaskFlag(MaterialDataAttribute::DISSOLVE);

		}
		else if(entryType == "Tr")
		{
			if(!(material->attributes & BitmaskFlag(MaterialDataAttribute::DISSOLVE)))
			{
				if(!(iss >> material->dissolve))
				{
					if(logErrors) std::cout << "ERROR: [" << objState->materialFileName << ":" << lineNo << "] Syntax error while parsing entry type '" << entryType << "'" << std::endl;
					break;

				}

				material->dissolve = 1.0f - material->dissolve;
				material->attributes |= BitmaskFlag(MaterialDataAttribute::DISSOLVE);

			}
			else
			{
				if(logWarnings) std::cout << "WARNING: [" << objState->materialFileName << ":" << lineNo << "] Transparency (Tr) property is non-standard, defaulting to Dissolve (d)." << std::endl;
				
			}

		}
		else if(entryType == "Tf")
		{
			if(!(iss >> material->transmissionFilter.r))
			{
				if(logErrors) std::cout << "ERROR: [" << objState->materialFileName << ":" << lineNo << "] Syntax error while parsing entry type '" << entryType << "'" << std::endl;
				break;

			}

			if(!(iss >> material->transmissionFilter.g >> material->transmissionFilter.b)) material->transmissionFilter.b = material->transmissionFilter.g = material->transmissionFilter.r;
			material->attributes |= BitmaskFlag(MaterialDataAttribute::TRANSMISSION_FILTER);

		}
		else if(entryType == "Ka")
		{
			if(!(iss >> material->ambientReflectance.r))
			{
				if(logErrors) std::cout << "ERROR: [" << objState->materialFileName << ":" << lineNo << "] Syntax error while parsing entry type '" << entryType << "'" << std::endl;
				break;

			}
			
			if(!(iss >> material->ambientReflectance.g >> material->ambientReflectance.b)) material->ambientReflectance.b = material->ambientReflectance.g = material->ambientReflectance.r;
			material->attributes |= BitmaskFlag(MaterialDataAttribute::AMBIENT_REFLECTANCE);

		}
		else if(entryType == "Kd")
		{
			if(!(iss >> material->diffuseReflectance.r))
			{
				if(logErrors) std::cout << "ERROR: [" << objState->materialFileName << ":" << lineNo << "] Syntax error while parsing entry type '" << entryType << "'" << std::endl;
				break;

			}
			
			if(!(iss >> material->diffuseReflectance.g >> material->diffuseReflectance.b)) material->diffuseReflectance.b = material->diffuseReflectance.g = material->diffuseReflectance.r;
			material->attributes |= BitmaskFlag(MaterialDataAttribute::DIFFUSE_REFLECTANCE);

		}
		else if(entryType == "Ks")
		{
			if(!(iss >> material->specularReflectance.r))
			{
				if(logErrors) std::cout << "ERROR: [" << objState->materialFileName << ":" << lineNo << "] Syntax error while parsing entry type '" << entryType << "'" << std::endl;
				break;

			}
			
			if(!(iss >> material->specularReflectance.g >> material->specularReflectance.b)) material->specularReflectance.b = material->specularReflectance.g = material->specularReflectance.r;
			material->attributes |= BitmaskFlag(MaterialDataAttribute::SPECULAR_REFLECTANCE);

		}
		else if(entryType == "Ke")
		{
			if(!(iss >> material->emissiveReflectance.r))
			{
				if(logErrors) std::cout << "ERROR: [" << objState->materialFileName << ":" << lineNo << "] Syntax error while parsing entry type '" << entryType << "'" << std::endl;
				break;

			}
			
			if(!(iss >> material->emissiveReflectance.g >> material->emissiveReflectance.b)) material->emissiveReflectance.b = material->emissiveReflectance.g = material->emissiveReflectance.r;
			material->attributes |= BitmaskFlag(MaterialDataAttribute::EMISSIVE_REFLECTANCE);

		}
		else if(entryType == "map_Ka")
		{
			if(!(iss >> material->ambientMap.path))
			{
				if(logErrors) std::cout << "ERROR: [" << objState->materialFileName << ":" << lineNo << "] Syntax error while parsing entry type '" << entryType << "'" << std::endl;
				break;

			}
			
			material->ambientMap.attributes |= BitmaskFlag(MapDataAttribute::PATH);
			material->attributes |= BitmaskFlag(MaterialDataAttribute::AMBIENT_MAP);

		}
		else if(entryType == "map_Kd")
		{
			if(!(iss >> material->diffuseMap.path))
			{
				if(logErrors) std::cout << "ERROR: [" << objState->materialFileName << ":" << lineNo << "] Syntax error while parsing entry type '" << entryType << "'" << std::endl;
				break;

			}
			
			material->diffuseMap.attributes |= BitmaskFlag(MapDataAttribute::PATH);
			material->attributes |= BitmaskFlag(MaterialDataAttribute::DIFFUSE_MAP);

		}
		else if(entryType == "map_Ks")
		{
			if(!(iss >> material->specularMap.path))
			{
				if(logErrors) std::cout << "ERROR: [" << objState->materialFileName << ":" << lineNo << "] Syntax error while parsing entry type '" << entryType << "'" << std::endl;
				break;

			}
			
			material->specularMap.attributes |= BitmaskFlag(MapDataAttribute::PATH);
			material->attributes |= BitmaskFlag(MaterialDataAttribute::SPECULAR_MAP);

		}
		else if(entryType == "map_Ke")
		{
			if(!(iss >> material->emissiveMap.path))
			{
				if(logErrors) std::cout << "ERROR: [" << objState->materialFileName << ":" << lineNo << "] Syntax error while parsing entry type '" << entryType << "'" << std::endl;
				break;

			}
			
			material->emissiveMap.attributes |= BitmaskFlag(MapDataAttribute::PATH);
			material->attributes |= BitmaskFlag(MaterialDataAttribute::EMISSIVE_MAP);

		}
		else if(entryType == "map_d")
		{
			if(!(iss >> material->dissolveMap.path))
			{
				if(logErrors) std::cout << "ERROR: [" << objState->materialFileName << ":" << lineNo << "] Syntax error while parsing entry type '" << entryType << "'" << std::endl;
				break;

			}
			
			material->dissolveMap.attributes |= BitmaskFlag(MapDataAttribute::PATH);
			material->attributes |= BitmaskFlag(MaterialDataAttribute::DISSOLVE_MAP);

		}
		else if(entryType == "map_bump" || entryType == "bump")
		{
			if(!(iss >> material->bumpMap.path))
			{
				if(logErrors) std::cout << "ERROR: [" << objState->materialFileName << ":" << lineNo << "] Syntax error while parsing entry type '" << entryType << "'" << std::endl;
				break;

			}

			material->bumpMap.attributes |= BitmaskFlag(MapDataAttribute::PATH);
			material->attributes |= BitmaskFlag(MaterialDataAttribute::BUMP_MAP);

		}
		else if(entryType == "map_disp" || entryType == "disp")
		{
			if(!(iss >> material->displacementMap.path))
			{
				if(logErrors) std::cout << "ERROR: [" << objState->materialFileName << ":" << lineNo << "] Syntax error while parsing entry type '" << entryType << "'" << std::endl;
				break;

			}

			material->displacementMap.attributes |= BitmaskFlag(MapDataAttribute::PATH);
			material->attributes |= BitmaskFlag(MaterialDataAttribute::DISPLACEMENT_MAP);

		}
		else if(entryType == "#")
		{
			continue;

		}
		else
		{
			if(logWarnings) std::cout << "WARNING: [" << objState->materialFileName << ":" << lineNo << "]: Unsupported entry type '" << entryType << "'" << std::endl;

		}

	}

	mtlFile.close();

	if(mtlState->materials.empty()) mtlState->materials.insert(std::make_pair(material->id, material));

	return true;

}

// f position.a/uv.a/normal.a position.b/uv.b/normal.b position.c/uv.c/normal.c [position.d/uv.d/normal.d] [position.n/uv.n/normal.n]
std::regex FACE_POSITION_UV_NORMAL { R"(^f\s+(-?\d+)\/(-?\d+)\/(-?\d+)\s+(-?\d+)\/(-?\d+)\/(-?\d+)\s+(-?\d+)\/(-?\d+)\/(-?\d+)(?:\s+(-?\d+)\/(-?\d+)\/(-?\d+))?(?:\s+(-?\d+)\/(-?\d+)\/(-?\d+))?)" };

// f position.a position.b position.c [position.d] [position.n]
std::regex FACE_POSITION { R"(^f\s+(-?\d+)\s+(-?\d+)\s+(-?\d+)(?:\s+(-?\d+))?(?:\s+(-?\d+))?)" };

// f position.a/uv.a position.b/uv.b position.c/uv.c [position.d/uv.d] [position.n/uv.n]
std::regex FACE_POSITION_UV { R"(^f\s+(-?\d+)\/(-?\d+)\s+(-?\d+)\/(-?\d+)\s+(-?\d+)\/(-?\d+)(?:\s+(-?\d+)\/(-?\d+))?(?:\s+(-?\d+)\/(-?\d+))?)" };

// f position.a//normal.a position.b//normal.b position.c//normal.c [position.d//normal.d] [position.n//normal.n]
std::regex FACE_POSITION_NORMAL { R"(^f\s+(-?\d+)\/\/(-?\d+)\s+(-?\d+)\/\/(-?\d+)\s+(-?\d+)\/\/(-?\d+)(?:\s+(-?\d+)\/\/(-?\d+))?(?:\s+(-?\d+)\/\/(-?\d+))?)" };

int main(int argc, char *argv[])
{
	std::string bomFilePath;//"BACK_butterfly_blue.bom";
	std::vector<std::string> objFilePaths;//{ "BACK_butterfly_blue.obj" };

	if(argc >= 2) bomFilePath = argv[1];
	if(bomFilePath.empty())
	{
		if(logWarnings) std::cout << "WARNING: No BOM file path provided as output. Syntax: obj2bom <output.bom> <input1.obj> [<input2.obj> ... <inputN.obj>]" << std::endl;
		return 0;

	}

	if(argc >= 3) for(int i = 0; i < (argc - 2); ++i) objFilePaths.push_back(argv[2 + i]);
	if(objFilePaths.empty())
	{
		if(logWarnings) std::cout << "WARNING: No OBJ file path(s) provided as input. Syntax: obj2bom <output.bom> <input1.obj> [<input2.obj> ... <inputN.obj>]" << std::endl;
		return 0;

	}

	std::uint32_t maxContainerId = 0;

	// OBJ Reader
	for(const auto &objFilePath : objFilePaths)
	{
		bool isFirstObject = true, isFirstGroup = true;
		std::string objectName = objFilePath.substr(objFilePath.find_last_of("/\\") + 1);
		std::string relativePath = objFilePath.substr(0, objFilePath.find_last_of("/\\") + 1);
		std::shared_ptr<obj_object_t> object = std::make_shared<obj_object_t>();
		object->name = objectName;
		object->containerId = maxContainerId;
		std::shared_ptr<obj_group_t> group = std::make_shared<obj_group_t>();

		std::map<std::string, obj_index_t> indices;
		std::vector<obj_vector3_t> positions;
		std::vector<obj_vector3_t> normals;
		std::vector<obj_vector2_t> uvs;
		obj_index_t index = 0;

		std::ifstream objFile(objFilePath);
		std::string line;
		int lineNo = 0;

		auto objState = std::make_shared<obj_state_t>();
		objStates.push_back(objState);

		std::cout << "Parsing OBJ '" << objFilePath << "'..." << std::endl;
		while(std::getline(objFile, line))
		{
			++lineNo;
			std::istringstream iss(line);
			//std::cout << line << "\n";
			if(line.empty()) continue;

			std::string entryType;
			if(!(iss >> entryType))
			{
				if(logErrors) std::cout << "ERROR: [" << objFilePath << ":" << lineNo << "] Failed to parse entry type." << std::endl;
				break;

			}

			if(entryType == "mtllib")
			{
				if(!(iss >> objState->materialFileName))
				{
					if(logErrors) std::cout << "ERROR: [" << objFilePath << ":" << lineNo << "] Syntax error while parsing entry type '" << entryType << "'" << std::endl;
					break;

				}
				
				objState->materialFileName = objState->materialFileName.substr(objState->materialFileName.find_last_of("/\\") + 1);

				if(!ReadMTL(relativePath, objState))
				{
					if(logErrors) std::cout << "ERROR: [" << objFilePath << ":" << lineNo << "] Failed to open material file '" << objState->materialFileName << "'" << std::endl;
					break;

				}

			}
			else if(entryType == "v")
			{
				obj_vector3_t position;
				if(!(iss >> position.x >> position.y >> position.z))
				{
					if(logErrors) std::cout << "ERROR: [" << objFilePath << ":" << lineNo << "] Syntax error while parsing entry type '" << entryType << "'" << std::endl;
					break;

				}

				positions.push_back(position);

			}
			else if(entryType == "vn")
			{
				obj_vector3_t normal;
				if(!(iss >> normal.x >> normal.y >> normal.z))
				{
					if(logErrors) std::cout << "ERROR: [" << objFilePath << ":" << lineNo << "] Syntax error while parsing entry type '" << entryType << "'" << std::endl;
					break;

				}

				normals.push_back(normal);

			}
			else if(entryType == "vt")
			{
				obj_vector2_t uv;
				float w;
				if(!(iss >> uv.x >> uv.y))
				{
					if(logErrors) std::cout << "ERROR: [" << objFilePath << ":" << lineNo << "] Syntax error while parsing entry type '" << entryType << "'" << std::endl;
					break;

				}
				else if((iss >> w) && w > 0.0f)
				{
					if(logWarnings) std::cout << "WARNING: [" << objFilePath << ":" << lineNo << "] 3D texture coordinates are not supported, W component has been discarded." << std::endl;

				}

				uvs.push_back(uv);

			}
			else if(entryType == "g" || entryType == "o")
			{
				if(!isFirstGroup) group = std::make_shared<obj_group_t>();
				if(!isFirstObject)
				{
					object = std::make_shared<obj_object_t>();
					object->name = objectName;

				}
				object->containerId = maxContainerId;

				//positions.clear();
				//uvs.clear();
				//normals.clear();
				indices.clear();
				index = 0;

				if(!(iss >> group->name))
				{
					if(logErrors) std::cout << "ERROR: [" << objFilePath << ":" << lineNo << "] Syntax error while parsing entry type '" << entryType << "'" << std::endl;
					break;

				}

				group->materialName = objState->materialName;
				group->materialId = objState->materialId;
				group->smoothing = objState->smoothing;

				object->groups.push_back(group);
				isFirstGroup = false;
				
				objState->objects.push_back(object);
				isFirstObject = false;

			}
			else if(entryType == "usemtl")
			{
				if(!(iss >> objState->materialName))
				{
					if(logErrors) std::cout << "ERROR: [" << objFilePath << ":" << lineNo << "] Syntax error while parsing entry type '" << entryType << "'" << std::endl;
					break;

				}

				bool foundMaterial = false;

				for(const auto &material : objState->mtlState->materials)
				{
					if(material.second->name == objState->materialName)
					{
						objState->materialId = material.second->id;
						foundMaterial = true;
						break;

					}

				}

				if(!foundMaterial)
				{
					if(logErrors) std::cout << "ERROR: [" << objFilePath << ":" << lineNo << "] Could not find material '" << objState->materialName << "'" << std::endl;
					break;

				}

				group->materialName = objState->materialName;
				group->materialId = objState->materialId;

			}
			else if(entryType == "s")
			{
				std::string smoothing;
				if(!(iss >> smoothing))
				{
					if(logErrors) std::cout << "ERROR: [" << objFilePath << ":" << lineNo << "] Syntax error while parsing entry type '" << entryType << "'" << std::endl;
					break;

				}

				if(smoothing == "off") objState->smoothing = 0;
				else if(smoothing == "on") objState->smoothing = 1;
				else objState->smoothing = std::stoi(smoothing);

				group->smoothing = objState->smoothing;

			}
			else if(entryType == "f")
			{
				struct obj_index_t
				{
					int a = 0, b = 0, c = 0, d = 0;//, e = 0;

				} position, uv, normal;
				//char skip;
				std::smatch matches;
				
				// Non-Indexed Geometry
				auto makeNonIndexedVertices = [&](const auto &positionIndex, const auto &normalIndex, const auto &uvIndex)
				{
					if(!positions.empty()) object->positions.push_back(positions[positionIndex >= 0 ? (positionIndex - 1) : (positions.size() + positionIndex)]);
					if(!normals.empty()) object->normals.push_back(normals[normalIndex >= 0 ? (normalIndex - 1) : (normals.size() + normalIndex)]);
					if(!uvs.empty()) object->uvs.push_back(uvs[uvIndex >= 0 ? (uvIndex - 1) : (uvs.size() + uvIndex)]);

				};
				
				// Indexed Geometry
				auto makeFaceIndex = [&](auto &faceIndex, const auto &positionIndex, const auto &normalIndex, const auto &uvIndex)
				{
					auto cacheIndexKey = std::to_string(positionIndex) + "," + std::to_string(normalIndex) + "," + std::to_string(uvIndex);
					auto cachedIndex = indices.find(cacheIndexKey);
					if(cachedIndex == indices.end())
					{
						indices.insert(std::make_pair(cacheIndexKey, index));
						faceIndex = index++;

						if(!positions.empty()) object->positions.push_back(positions[positionIndex >= 0 ? (positionIndex - 1) : (positions.size() + positionIndex)]);
						if(!normals.empty()) object->normals.push_back(normals[normalIndex >= 0 ? (normalIndex - 1) : (normals.size() + normalIndex)]);
						if(!uvs.empty()) object->uvs.push_back(uvs[uvIndex >= 0 ? (uvIndex - 1) : (uvs.size() + uvIndex)]);

					}
					else
					{
						faceIndex = cachedIndex->second;

					}

				};
				
				int numFaces = 0;

				if(std::regex_search(line, matches, FACE_POSITION_UV_NORMAL) && matches.size() >= 10)
				{
					numFaces = 3;

					position = { std::stoi(matches.str(1)), std::stoi(matches.str(4)), std::stoi(matches.str(7)) };
					uv = { std::stoi(matches.str(2)), std::stoi(matches.str(5)), std::stoi(matches.str(8)) };
					normal = { std::stoi(matches.str(3)), std::stoi(matches.str(6)), std::stoi(matches.str(9)) };
					
					if(matches.size() >= 13 && !matches.str(10).empty() && !matches.str(11).empty() && !matches.str(12).empty())
					{
						position.d = std::stoi(matches.str(10));
						uv.d = std::stoi(matches.str(11));
						normal.d = std::stoi(matches.str(12));
						++numFaces;

					}

					if(matches.size() >= 16 && !matches.str(13).empty() && !matches.str(14).empty() && !matches.str(15).empty()) ++numFaces;

				}
				else if(std::regex_search(line, matches, FACE_POSITION_UV) && matches.size() >= 7)
				{
					numFaces = 3;

					position = { std::stoi(matches.str(1)), std::stoi(matches.str(3)), std::stoi(matches.str(5)) };
					uv = { std::stoi(matches.str(2)), std::stoi(matches.str(4)), std::stoi(matches.str(6)) };
					
					if(matches.size() >= 9 && !matches.str(7).empty() && !matches.str(8).empty())
					{
						position.d = std::stoi(matches.str(7));
						uv.d = std::stoi(matches.str(8));
						++numFaces;

					}

					if(matches.size() >= 11 && !matches.str(9).empty() && !matches.str(10).empty()) ++numFaces;

				}
				else if(std::regex_search(line, matches, FACE_POSITION_NORMAL) && matches.size() >= 7)
				{
					numFaces = 3;

					position = { std::stoi(matches.str(1)), std::stoi(matches.str(3)), std::stoi(matches.str(5)) };
					normal = { std::stoi(matches.str(2)), std::stoi(matches.str(4)), std::stoi(matches.str(6)) };
					
					if(matches.size() >= 9 &&!matches.str(7).empty() && !matches.str(8).empty())
					{
						position.d = std::stoi(matches.str(7));
						normal.d = std::stoi(matches.str(8));
						++numFaces;

					}

					if(matches.size() >= 11 && !matches.str(9).empty() && !matches.str(10).empty()) ++numFaces;

				}
				else if(std::regex_search(line, matches, FACE_POSITION) && matches.size() >= 4)
				{
					numFaces = 3;

					position = { std::stoi(matches.str(1)), std::stoi(matches.str(2)), std::stoi(matches.str(3)) };
					
					if(matches.size() >= 5 && !matches.str(4).empty())
					{
						position.d = std::stoi(matches.str(4));
						++numFaces;

					}

					if(matches.size() >= 6 && !matches.str(5).empty()) ++numFaces;

				}
				
				if(numFaces == 3)
				{
					// Triangles
					if(createIndexedGeometry)
					{
						// Indexed Geometry
						obj_face3_t face;
						makeFaceIndex(face.a, position.a, normal.a, uv.a);
						makeFaceIndex(face.b, position.b, normal.b, uv.b);
						makeFaceIndex(face.c, position.c, normal.c, uv.c);
						group->faces.push_back(face);

					}
					else
					{
						// Non-Indexed Geometry
						makeNonIndexedVertices(position.a, normal.a, uv.a);
						makeNonIndexedVertices(position.b, normal.b, uv.b);
						makeNonIndexedVertices(position.c, normal.c, uv.c);

					}

				}
				else if(numFaces == 4)
				{
					// Quads
					if(logWarnings) std::cout << "WARNING: [" << objFilePath << ":" << lineNo << "] Quad geometry faces are automatically triangulated." << std::endl;
					/*
					double dist1 = x.Vertices[mf.A].DistanceTo(x.Vertices[mf.C]);
					double dist2 = x.Vertices[mf.B].DistanceTo(x.Vertices[mf.D]);
					
					if(dist1 > dist2)
					{
						x.Faces.AddFace(mf.A, mf.B, mf.D);
						x.Faces.AddFace(mf.B, mf.C, mf.D);
					}
					else
					{
						x.Faces.AddFace(mf.A, mf.B, mf.C);
						x.Faces.AddFace(mf.A, mf.C, mf.D);
					}*/

					// Triangulate Quad Face
					if(createIndexedGeometry)
					{
						// Indexed Geometry
						obj_face3_t face;
						makeFaceIndex(face.a, position.a, normal.a, uv.a);
						makeFaceIndex(face.b, position.b, normal.b, uv.b);
						makeFaceIndex(face.c, position.c, normal.c, uv.c);
						group->faces.push_back(face);

						makeFaceIndex(face.a, position.a, normal.a, uv.a);
						makeFaceIndex(face.b, position.c, normal.c, uv.c);
						makeFaceIndex(face.c, position.d, normal.d, uv.d);
						group->faces.push_back(face);

					}
					else
					{
						// Non-Indexed Geometry
						makeNonIndexedVertices(position.a, normal.a, uv.a);
						makeNonIndexedVertices(position.b, normal.b, uv.b);
						makeNonIndexedVertices(position.c, normal.c, uv.c);

						makeNonIndexedVertices(position.a, normal.a, uv.a);
						makeNonIndexedVertices(position.c, normal.c, uv.c);
						makeNonIndexedVertices(position.d, normal.d, uv.d);

					}

				}
				else if(numFaces > 4)
				{
					// N-gons
					if(logErrors) std::cout << "ERROR: [" << objFilePath << ":" << lineNo << "] N-gon geometry faces are not supported." << std::endl;
					break;

				}
				else
				{
					if(logErrors) std::cout << "ERROR: [" << objFilePath << ":" << lineNo << "] Syntax error while parsing entry type '" << entryType << "'" << std::endl;
					break;

				}
/*
				// Format Assumption
				// position/uv/normal position/uv/normal position/uv/normal [position/uv/normal]
				if(!(iss >>
					 position.a >> skip >> uv.a >> skip >> normal.a >>
					 position.b >> skip >> uv.b >> skip >> normal.b >>
					 position.c >> skip >> uv.c >> skip >> normal.c))
				{
					if(logErrors) std::cout << "ERROR: [" << objFilePath << ":" << lineNo << "] Syntax error while parsing entry type '" << entryType << "'" << std::endl;
					break;

				}
				else if(iss >> position.d >> skip >> uv.d >> skip >> normal.d)
				{
					if(logWarnings) std::cout << "WARNING: [" << objFilePath << ":" << lineNo << "] Quad geometry faces are automatically triangulated." << std::endl;

					// Triangulate Quad Face
					if(createIndexedGeometry)
					{
						// Indexed Geometry
						obj_face3_t face;
						makeFaceIndex(face.a, position.a, normal.a, uv.a);
						makeFaceIndex(face.b, position.b, normal.b, uv.b);
						makeFaceIndex(face.c, position.d, normal.d, uv.d);
						group->faces.push_back(face);

						makeFaceIndex(face.a, position.b, normal.b, uv.b);
						makeFaceIndex(face.b, position.c, normal.c, uv.c);
						makeFaceIndex(face.c, position.d, normal.d, uv.d);
						group->faces.push_back(face);

					}
					else
					{
						// Non-Indexed Geometry
						makeNonIndexedVertices(position.a, normal.a, uv.a);
						makeNonIndexedVertices(position.b, normal.b, uv.b);
						makeNonIndexedVertices(position.d, normal.d, uv.d);

						makeNonIndexedVertices(position.b, normal.b, uv.b);
						makeNonIndexedVertices(position.c, normal.c, uv.c);
						makeNonIndexedVertices(position.d, normal.d, uv.d);

					}
					
					continue;

				}
				else if(iss >> skip >> position.e >> skip >> uv.e >> skip >> normal.e)
				{
					if(logErrors) std::cout << "ERROR: [" << objFilePath << ":" << lineNo << "] N-gon geometry faces are not supported." << std::endl;
					break;

				}

				if(createIndexedGeometry)
				{
					// Indexed Geometry
					obj_face3_t face;
					makeFaceIndex(face.a, position.a, normal.a, uv.a);
					makeFaceIndex(face.b, position.b, normal.b, uv.b);
					makeFaceIndex(face.c, position.c, normal.c, uv.c);
					group->faces.push_back(face);

				}
				else
				{
					// Non-Indexed Geometry
					makeNonIndexedVertices(position.a, normal.a, uv.a);
					makeNonIndexedVertices(position.b, normal.b, uv.b);
					makeNonIndexedVertices(position.c, normal.c, uv.c);

				}*/

			}
			else if(entryType == "#")
			{
				continue;

			}
			else if(entryType == "p" || entryType == "l" || entryType == "curv" || entryType == "curv2" || entryType == "surf")
			{
				if(logErrors) std::cout << "ERROR: [" << objFilePath << ":" << lineNo << "] Geometry type '" << entryType << "' is not supported." << std::endl;
				break;

			}
			else
			{
				std::cout << entryType << std::endl;

			}

		}

		if(isFirstGroup) object->groups.push_back(group);
		if(isFirstObject) objState->objects.push_back(object);

		objFile.close();
		
		++maxContainerId;

	}

	WriteBOM(bomFilePath);

	//system("PAUSE");
	return 0;

}