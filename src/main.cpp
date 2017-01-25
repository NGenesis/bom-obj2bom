#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include <vector>
#include <cstdint>
#include <map>
#include <set>
#include <type_traits>

#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
#include "tiny_obj_loader.h"

template <typename T>
auto BitmaskFlag(T flag)
{
    return static_cast<typename std::underlying_type<T>::type>(flag);

}

static const std::uint8_t BOM_DATA_VERSION = 1;

// BOM
enum class FileDataAttribute : std::uint8_t
{
    NONE = 1 << 0,
    MATERIAL_LIBRARY = 1 << 1

};

enum class GroupDataAttribute : std::uint8_t
{
    NONE = 1 << 0,
    INDEX = 1 << 1,
    SMOOTHING = 1 << 2,
    MATERIAL = 1 << 3

};

enum class ObjectDataAttribute : std::uint8_t
{
    NONE = 1 << 0,
    GEOMETRY = 1 << 1

};

enum class GeometryDataAttribute : std::uint32_t
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
    std::uint16_t a, b, c;

};

struct alignas(1) mtl_color_t
{
    float r, g, b;

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
    std::uint8_t smoothing = 0;
    std::vector<obj_face3_t> faces;

};

struct obj_object_t
{
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

};

std::vector<std::shared_ptr<obj_state_t>> objStates;
std::vector<std::shared_ptr<mtl_state_t>> mtlStates;
std::uint16_t maxMaterialId = 0;

bool WriteBOM(const std::string &bomFilePath)
{
    std::cout << "Writing BOM..." << std::endl;

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
            if(!object->positions.empty()) objectAttributes |= BitmaskFlag(ObjectDataAttribute::GEOMETRY);
            bomFile.write(reinterpret_cast<char*>(&objectAttributes), sizeof(objectAttributes));

            // Object Name
            std::uint16_t objectNameLength = object->name.size();
            bomFile.write(reinterpret_cast<char*>(&objectNameLength), sizeof(objectNameLength));
            bomFile.write(object->name.c_str(), objectNameLength);
            std::cout << "Object Name: " << object->name << std::endl;

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
                if(!group->faces.empty()) groupAttributes |= BitmaskFlag(GroupDataAttribute::INDEX);
                if(group->smoothing >= 0) groupAttributes |= BitmaskFlag(GroupDataAttribute::SMOOTHING);
                if(!group->materialName.empty() && objState->mtlState && objState->mtlState->materials.find(group->materialId) != objState->mtlState->materials.end()) groupAttributes |= BitmaskFlag(GroupDataAttribute::MATERIAL);
                bomFile.write(reinterpret_cast<char*>(&groupAttributes), sizeof(groupAttributes));

                // Group Name
                std::uint16_t groupNameLength = group->name.size();
                bomFile.write(reinterpret_cast<char*>(&groupNameLength), sizeof(groupNameLength));
                bomFile.write(group->name.c_str(), groupNameLength);

                std::cout << "Group Name: " << group->name << std::endl;

                if(objectAttributes & BitmaskFlag(ObjectDataAttribute::GEOMETRY))
                {
                    // Indices
                    if(groupAttributes & BitmaskFlag(GroupDataAttribute::INDEX))
                    {
                        // Index Count
                        std::uint16_t indexCount = group->faces.size() * 3;
                        bomFile.write(reinterpret_cast<char*>(&indexCount), sizeof(indexCount));
                        std::cout << "Index Count: " << indexCount << std::endl;

                        // Indices
                        bomFile.write(reinterpret_cast<char*>(group->faces.data()), sizeof(std::uint16_t) * indexCount);

                    }

                    // Smoothing
                    if(groupAttributes & BitmaskFlag(GroupDataAttribute::SMOOTHING)) bomFile.write(reinterpret_cast<char*>(&group->smoothing), sizeof(group->smoothing));

                    if(groupAttributes & BitmaskFlag(GroupDataAttribute::MATERIAL))
                    {
                        std::cout << "Material: " << group->materialName << ", ID: " << objState->mtlState->materials[group->materialId]->id << std::endl;
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

bool ReadMTL(std::shared_ptr<obj_state_t> objState)
{
    // MTL Parser
    std::ifstream mtlFile(objState->materialFileName);
    if(!mtlFile.is_open()) return false;

    auto mtlState = std::make_shared<mtl_state_t>();
    mtlStates.push_back(mtlState);
    objState->mtlState = mtlState;

    mtlState->name = objState->materialFileName;

    std::shared_ptr<mtl_material_t> material = std::make_shared<mtl_material_t>();
    material->id = maxMaterialId++;

    std::cout << "Parsing MTL..." << std::endl;
    std::string line;
    while(std::getline(mtlFile, line))
    {
        std::istringstream iss(line);
        //std::cout << line << "\n";
        if(line.empty()) continue;

        std::string entryType;
        if(!(iss >> entryType))
        {
            std::cout << "Error parsing MTL, could not parse entry type." << std::endl;
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
                std::cout << "Syntax error while parsing entry type: " << entryType << std::endl;
                break;

            }

            mtlState->materials.insert(std::make_pair(material->id, material));
            //std::cout << "Material: " << material->name << std::endl;


        }
        else if(entryType == "illum")
        {
            if(!(iss >> material->illuminationModel))
            {
                std::cout << "Syntax error while parsing entry type: " << entryType << std::endl;
                break;

            }

            material->attributes |= BitmaskFlag(MaterialDataAttribute::ILLUMINATION_MODEL);

        }
        else if(entryType == "Ns")
        {
            if(!(iss >> material->specularExponent))
            {
                std::cout << "Syntax error while parsing entry type: " << entryType << std::endl;
                break;

            }

            material->attributes |= BitmaskFlag(MaterialDataAttribute::SPECULAR_EXPONENT);

        }
        else if(entryType == "Ni")
        {
            if(!(iss >> material->opticalDensity))
            {
                std::cout << "Syntax error while parsing entry type: " << entryType << std::endl;
                break;

            }

            material->attributes |= BitmaskFlag(MaterialDataAttribute::OPTICAL_DENSITY);

        }
        else if(entryType == "d")
        {
            if(!(iss >> material->dissolve))
            {
                std::cout << "Syntax error while parsing entry type: " << entryType << std::endl;
                break;

            }

            material->attributes |= BitmaskFlag(MaterialDataAttribute::DISSOLVE);

        }
        else if(entryType == "Tr")
        {
            if(!(iss >> material->dissolve))
            {
                std::cout << "Syntax error while parsing entry type: " << entryType << std::endl;
                break;

            }

            material->dissolve = 1.0f - material->dissolve;
            material->attributes |= BitmaskFlag(MaterialDataAttribute::DISSOLVE);

        }
        else if(entryType == "Tf")
        {
            if(!(iss >> material->transmissionFilter.r >> material->transmissionFilter.g >> material->transmissionFilter.b))
            {
                std::cout << "Syntax error while parsing entry type: " << entryType << std::endl;
                break;

            }

            material->attributes |= BitmaskFlag(MaterialDataAttribute::TRANSMISSION_FILTER);

        }
        else if(entryType == "Ka")
        {
            if(!(iss >> material->ambientReflectance.r >> material->ambientReflectance.g >> material->ambientReflectance.b))
            {
                std::cout << "Syntax error while parsing entry type: " << entryType << std::endl;
                break;

            }

            material->attributes |= BitmaskFlag(MaterialDataAttribute::AMBIENT_REFLECTANCE);

        }
        else if(entryType == "Kd")
        {
            if(!(iss >> material->diffuseReflectance.r >> material->diffuseReflectance.g >> material->diffuseReflectance.b))
            {
                std::cout << "Syntax error while parsing entry type: " << entryType << std::endl;
                break;

            }

            material->attributes |= BitmaskFlag(MaterialDataAttribute::DIFFUSE_REFLECTANCE);

        }
        else if(entryType == "Ks")
        {
            if(!(iss >> material->specularReflectance.r >> material->specularReflectance.g >> material->specularReflectance.b))
            {
                std::cout << "Syntax error while parsing entry type: " << entryType << std::endl;
                break;

            }

            material->attributes |= BitmaskFlag(MaterialDataAttribute::SPECULAR_REFLECTANCE);

        }
        else if(entryType == "Ke")
        {
            if(!(iss >> material->emissiveReflectance.r >> material->emissiveReflectance.g >> material->emissiveReflectance.b))
            {
                std::cout << "Syntax error while parsing entry type: " << entryType << std::endl;
                break;

            }

            material->attributes |= BitmaskFlag(MaterialDataAttribute::EMISSIVE_REFLECTANCE);

        }
        else
        {
            //std::cout << entryType << std::endl;

        }

    }

    mtlFile.close();

    if(mtlState->materials.empty()) mtlState->materials.insert(std::make_pair(material->id, material));

    return true;

}

int main(int argc, char *argv[])
{
    std::string bomFilePath = "boat.bom";//"BACK_butterfly_blue.bom";
    std::vector<std::string> objFilePaths { "boat.obj" };//{ "BACK_butterfly_blue.obj" };

    if(argc >= 2) bomFilePath = argv[1];
    if(bomFilePath.empty()) return 0;

    if(argc >= 3) for(int i = 0; i < (argc - 3); ++i) objFilePaths.push_back(argv[2 + i]);
    if(objFilePaths.empty()) return 0;

    // OBJ Reader
    for(const auto &objFilePath : objFilePaths)
    {
        bool isFirstObject = true, isFirstGroup = true;
        std::shared_ptr<obj_object_t> object = std::make_shared<obj_object_t>();
        std::shared_ptr<obj_group_t> group = std::make_shared<obj_group_t>();
        std::map<std::uint16_t, std::pair<obj_vector2_t, obj_vector3_t>> indices;

        std::ifstream objFile(objFilePath);
        std::string line, lastEntryType;

        auto objState = std::make_shared<obj_state_t>();
        objStates.push_back(objState);

        std::cout << "Parsing OBJ..." << std::endl;
        while(std::getline(objFile, line))
        {
            std::istringstream iss(line);
            //std::cout << line << "\n";
            if(line.empty()) continue;

            std::string entryType;
            if(!(iss >> entryType))
            {
                std::cout << "Error parsing OBJ, could not parse entry type." << std::endl;
                break;

            }

            if(entryType == "mtllib")
            {
                if(!(iss >> objState->materialFileName))
                {
                    std::cout << "Syntax error while parsing entry type: " << entryType << std::endl;
                    break;

                }

                //std::cout << "Material File: " << objState->materialFileName << std::endl;
                ReadMTL(objState);

            }
            else if(entryType == "v")
            {
                if(lastEntryType != "v")
                {
                    bool hasUV = !object->uvs.empty(), hasNormal = !object->normals.empty();
                    if(hasUV || hasNormal)
                    {
                        object->uvs.clear();
                        object->normals.clear();

                        for(auto &index : indices)
                        {
                            if(hasUV) object->uvs.push_back(index.second.first);
                            if(hasNormal) object->normals.push_back(index.second.second);

                        }

                    }

                    indices.clear();

                    if(!isFirstObject) object = std::make_shared<obj_object_t>();

                    objState->objects.push_back(object);
                    isFirstObject = false;
                    std::cout << "New Object: " << std::endl;

                }

                obj_vector3_t position;
                if(!(iss >> position.x >> position.y >> position.z))
                {
                    std::cout << "Syntax error while parsing entry type: " << entryType << std::endl;
                    break;

                }

                object->positions.push_back(position);
                //std::cout << "Position: " << position.x << " " << position.y << " " << position.z << std::endl;

            }
            else if(entryType == "vn")
            {
                obj_vector3_t normal;
                if(!(iss >> normal.x >> normal.y >> normal.z))
                {
                    std::cout << "Syntax error while parsing entry type: " << entryType << std::endl;
                    break;

                }

                object->normals.push_back(normal);
                //std::cout << "Normal: " << normal.x << " " << normal.y << " " << normal.z << std::endl;

            }
            else if(entryType == "vt")
            {
                obj_vector2_t uv;
                if(!(iss >> uv.x >> uv.y))
                {
                    std::cout << "Syntax error while parsing entry type: " << entryType << std::endl;
                    break;

                }

                object->uvs.push_back(uv);
                //std::cout << "UV: " << uv.x << " " << uv.y << std::endl;

            }
            else if(entryType == "g" || entryType == "o")
            {
                if(!isFirstGroup) group = std::make_shared<obj_group_t>();

                if(!(iss >> group->name))
                {
                    std::cout << "Syntax error while parsing entry type: " << entryType << std::endl;
                    break;

                }

                group->materialName = objState->materialName;
                group->materialId = objState->materialId;

                object->groups.push_back(group);
                isFirstGroup = false;
                std::cout << "Group: " << group->name << ", Material: " << group->materialName << ", MaterialID: " << group->materialId << std::endl;

            }/*
            else if(entryType == "o")
            {
                bool hasUV = !object->uvs.empty(), hasNormal = !object->normals.empty();
                if(hasUV || hasNormal)
                {
                    object->uvs.clear();
                    object->normals.clear();

                    for(auto &index : indices)
                    {
                        if(hasUV) object->uvs.push_back(index.second.first);
                        if(hasNormal) object->normals.push_back(index.second.second);

                    }

                }

                if(!objState->objects.empty())
                {
                    object = std::make_shared<obj_object_t>();
                    indices.clear();

                }

                if(!(iss >> object->name))
                {
                    std::cout << "Syntax error while parsing entry type: " << entryType << std::endl;
                    break;

                }

                objState->objects.push_back(object);
                std::cout << "Object: " << object->name << std::endl;

            }*/
            else if(entryType == "usemtl")
            {
                if(!(iss >> objState->materialName))
                {
                    std::cout << "Syntax error while parsing entry type: " << entryType << std::endl;
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
                    std::cout << "Material not found: " << objState->materialName << std::endl;
                    break;

                }

                std::cout << "Material Name: " << objState->materialName << ", ID: " << objState->materialId << ", assigned to group " << group->name << std::endl;
                group->materialName = objState->materialName;
                group->materialId = objState->materialId;

            }
            else if(entryType == "s")
            {
                std::string smoothing;
                if(!(iss >> smoothing))
                {
                    std::cout << "Syntax error while parsing entry type: " << entryType << std::endl;
                    break;

                }

                if(smoothing == "off") group->smoothing = 0;
                else if(smoothing == "on") group->smoothing = 1;
                else group->smoothing = std::stoi(smoothing);

                std::cout << "Smoothing: " << static_cast<unsigned>(group->smoothing) << std::endl;

            }
            else if(entryType == "f")
            {
                struct obj_index_t
                {
                    std::uint16_t a, b, c, d;

                } position, uv, normal;
                char skip;

                // Format Assumption
                // position/uv/normal position/uv/normal position/uv/normal [position/uv/normal]
                if(!(iss >>
                     position.a >> skip >> uv.a >> skip >> normal.a >>
                     position.b >> skip >> uv.b >> skip >> normal.b >>
                     position.c >> skip >> uv.c >> skip >> normal.c))
                {
                    std::cout << "Syntax error while parsing entry type: " << entryType << std::endl;
                    break;

                }
                else if(iss >> skip >> position.d >> skip >> uv.d >> skip >> normal.d)
                {
                    std::cout << "Quad geometry faces are not supported." << std::endl;
                    break;

                }
/*
                std::cout << position.a << "/" << uv.a << "/" << normal.a << " " <<
                     position.b << "/" << uv.b << "/" << normal.b << " " <<
                     position.c << "/" << uv.c << "/" << normal.c << std::endl;
*/
                bool hasUV = !object->uvs.empty(), hasNormal = !object->normals.empty();
                indices[--position.a] = std::make_pair(hasUV ? object->uvs[--uv.a] : obj_vector2_t{}, hasNormal ? object->normals[--normal.a] : obj_vector3_t{});
                indices[--position.b] = std::make_pair(hasUV ? object->uvs[--uv.b] : obj_vector2_t{}, hasNormal ? object->normals[--normal.b] : obj_vector3_t{});
                indices[--position.c] = std::make_pair(hasUV ? object->uvs[--uv.c] : obj_vector2_t{}, hasNormal ? object->normals[--normal.c] : obj_vector3_t{});

                group->faces.push_back({ position.a, position.b, position.c });

            }
            else if(entryType == "#")
            {
                continue;

            }
            else if(entryType == "p" || entryType == "l" || entryType == "curv" || entryType == "curv2" || entryType == "surf")
            {
                std::cout << "Geometry type '" << entryType << "' is not supported." << std::endl;
                break;

            }
            else
            {
                std::cout << entryType << std::endl;

            }

            lastEntryType = entryType;

        }

        bool hasUV = !object->uvs.empty(), hasNormal = !object->normals.empty();
        if(hasUV || hasNormal)
        {
            object->uvs.clear();
            object->normals.clear();

            for(auto &index : indices)
            {
                if(hasUV) object->uvs.push_back(index.second.first);
                if(hasNormal) object->normals.push_back(index.second.second);

            }

            std::cout << object->positions.size() << " " << object->uvs.size() << " " << object->normals.size() << std::endl;

        }

        std::cout << objState->objects.size() << std::endl;
        for(auto &object : objState->objects) for(auto &group : object->groups) std::cout << group << std::endl;

        if(isFirstGroup) object->groups.push_back(group);
        if(isFirstObject) objState->objects.push_back(object);

        objFile.close();

    }

    WriteBOM(bomFilePath);

    //system("PAUSE");
    return 0;

}
