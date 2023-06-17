#include "GLTFImporter.hpp"

// Thanks to Victor Gordan for his tutorial on gltf model loader: https://github.com/VictorGordan/opengl-tutorials

using namespace nlohmann;

namespace en
{
	GLTFImporter::GLTFImporter(const MeshImportProperties& properties, Handle<Material> defaultMaterial, Handle<Texture> defaultSRGBTexture, Handle<Texture> defaultNonSRGBTexture) 
        : Importer(properties, defaultMaterial, defaultSRGBTexture, defaultNonSRGBTexture)
	{

	}
    MeshData GLTFImporter::LoadMeshFromFile(const std::string& filePath, const std::string& name)
	{
        m_FilePath = filePath;
        m_FileDirectory = filePath.substr(0, std::max(filePath.find_last_of('/') + 1, filePath.find_last_of('\\') + 1));

        Handle<Mesh> mesh = MakeHandle<Mesh>(name, m_FilePath);

        JSON = GetMeshJsonData();
        if (JSON == nullptr)
        {
            EN_WARN("GLTFImporter::LoadMeshFromFile() - Failed to locate a JSON gltf file at " + m_FilePath);
            return MeshData{ mesh };
        }

        m_BinaryData = GetMeshBinaryData();
        if (m_BinaryData.size() == 0)
        {
            EN_WARN("GLTFImporter::LoadMeshFromFile() - Failed to locate a binary gltf file at " + m_FileDirectory + std::string(JSON["buffers"][0]["uri"]));
            return MeshData{ mesh };
        }

        if (m_ImportProperties.importMaterials)
            GetMaterials();

        for (uint32_t i = 0U; i < JSON["nodes"].size(); i++)
            ProcessNode(i, mesh);

        EN_SUCCESS("Succesfully loaded a mesh from \"" + m_FilePath + "\"");

        return MeshData{ mesh, m_Materials, m_Textures };
	}

    void GLTFImporter::ProcessNode(uint32_t id, Handle<Mesh> mesh)
    {
        const json& node = JSON["nodes"][id];

        glm::vec3 position(0.0f);
        glm::vec3 rotation(0.0f);
        glm::vec3 scale(1.0f);
        glm::mat4 matrix = glm::mat4(1.0f);

        if (node.contains("translation"))
            memcpy(&position, &node["translation"], sizeof(glm::vec3));

        if (node.contains("rotation"))
        {
            float rotValues[4]{
                node["rotation"][3],
                node["rotation"][0],
                node["rotation"][1],
                node["rotation"][2]
            };

            rotation = glm::eulerAngles(glm::make_quat(rotValues));
        }

        if (node.contains("scale"))
            memcpy(&scale, &node["scale"], sizeof(glm::vec3));

        if (node.contains("mesh"))
            ProcessMesh(node["mesh"], mesh);

        if (node.contains("children"))
            for (uint32_t i = 0U; i < node["children"].size(); i++)
                ProcessNode(node["children"][i], mesh);
    }
    void GLTFImporter::ProcessMesh(uint32_t id, Handle<Mesh> mesh)
    {
        const json& primitive = JSON["meshes"][id]["primitives"][0];

        uint32_t posAccInd = primitive["attributes"]["POSITION"];
        uint32_t normalAccInd = primitive["attributes"]["NORMAL"];
        uint32_t texAccInd = primitive["attributes"]["TEXCOORD_0"];
        uint32_t indAccInd = primitive["indices"];
        uint32_t matAccInd = (uint32_t)-1; 

        if(primitive.contains("material") && m_ImportProperties.importMaterials)
            matAccInd = primitive["material"];


        std::vector<float> posVec = GetFloats(JSON["accessors"][posAccInd]);
        std::vector<glm::vec3> positions(posVec.size() / 3);
        memcpy(positions.data(), posVec.data(), posVec.size() * sizeof(float));

        std::vector<float> normalVec = GetFloats(JSON["accessors"][normalAccInd]);
        std::vector<glm::vec3> normals(normalVec.size() / 3);
        memcpy(normals.data(), normalVec.data(), normalVec.size() * sizeof(float));

        std::vector<float> texVec = GetFloats(JSON["accessors"][texAccInd]);
        std::vector<glm::vec2> texUVs(texVec.size() / 2);
        memcpy(texUVs.data(), texVec.data(), texVec.size() * sizeof(float));


        std::vector<Vertex> vertices(positions.size());
        for (size_t i = 0; i < vertices.size(); i++)
        {
            vertices[i] = Vertex{
                positions[i],
                normals[i],
                texUVs[i]
            };
        }
        
        std::vector<uint32_t> indices = GetIndices(JSON["accessors"][indAccInd]);

        mesh->m_SubMeshes.emplace_back(vertices, indices, matAccInd == (uint32_t)-1 ? m_DefaultMaterial : m_Materials[matAccInd]);
    }

    std::vector<float> GLTFImporter::GetFloats(const nlohmann::json& accessor)
    {
        uint32_t bufferViewID = accessor.value("bufferView", 1);
        const json& bufferView = JSON["bufferViews"][bufferViewID];

        size_t accessorByteOffset = accessor.value("byteOffset", 0);
        size_t byteOffset = bufferView["byteOffset"];
        size_t finalOffset = byteOffset + accessorByteOffset;

        size_t count = accessor["count"];
        
        const std::string& dataType = accessor["type"];

        size_t typeSize{};
        if (dataType == "SCALAR") typeSize = 1;
        else if (dataType == "VEC2")   typeSize = 2;
        else if (dataType == "VEC3")   typeSize = 3;
        else if (dataType == "VEC4")   typeSize = 4;
        else EN_ERROR("GLTFImporter::GetFloats() type is not SCALAR or VEC2 or VEC3 or VEC4");

        std::vector<float> floats(count * typeSize);
        std::memcpy(floats.data(), m_BinaryData.data() + finalOffset, floats.size() * sizeof(float));

        return floats;
    }
    std::vector<uint32_t> GLTFImporter::GetIndices(const nlohmann::json& accessor)
    {
        uint32_t bufferViewID = accessor.value("bufferView", 0);
        const json& bufferView = JSON["bufferViews"][bufferViewID];

        size_t accessorByteOffset = accessor.value("byteOffset", 0);
        size_t byteOffset = bufferView["byteOffset"];
        size_t finalOffset = byteOffset + accessorByteOffset;

        size_t count = accessor["count"];

        std::vector<uint32_t> indices(count);

        uint32_t componentType = accessor["componentType"];
        if (componentType == 5125)
        {
            std::memcpy(indices.data(), m_BinaryData.data() + finalOffset, count * sizeof(uint32_t));
        }
        else if (componentType == 5123)
        {
            finalOffset /= sizeof(unsigned short);

            const unsigned short* binaryDataShort = (const unsigned short*)m_BinaryData.data();
            for (size_t i = finalOffset, j = 0; i < finalOffset + count; ++i, ++j)
                indices[j] = (uint32_t)binaryDataShort[i];
        }
        else if (componentType == 5122)
        {
            finalOffset /= sizeof(short);

            const short* binaryDataShort = (const short*)m_BinaryData.data();
            for (size_t i = finalOffset, j = 0; i < finalOffset + count; i++, ++j)
                indices[j] = (uint32_t)binaryDataShort[i];
        }

        return indices;
    }
    
    void GLTFImporter::GetMaterials()
    {
        std::unordered_map<uint32_t, Handle<Texture>> textures{};
        //std::unordered_map<uint32_t, VkSampler> samplers{};
        
        if (JSON.contains("materials"))
        for (const auto& material : JSON["materials"])
        {
            std::string name{"New Material " + std::to_string(GetMaterialCounter()++)};

            Handle<Texture> albedoTexture    = m_DefaultSRGBTexture;
            Handle<Texture> roughnessTexture = m_DefaultNonSRGBTexture;
            Handle<Texture> metalnessTexture = m_DefaultNonSRGBTexture;
            Handle<Texture> normalTexture    = m_DefaultNonSRGBTexture;

            glm::vec3 color(1.0f);
            float roughness = 0.75f;
            float metalness = 0.0f;
            float normalStrength = 1.0f;

            if (material.contains("name"))
                name = material["name"];

            if (material.contains("normalTexture"))
            {
                uint32_t textureIndex = material["normalTexture"]["index"];

                //uint32_t normalSamplerIndex = JSON["textures"][normalTextureIndex]["sampler"];
                uint32_t imageIndex = JSON["textures"][textureIndex]["source"];

                std::string textureName = JSON["images"][imageIndex]["name"];
                std::string textureURI = JSON["images"][imageIndex]["uri"];

                if (m_ImportProperties.importNormalTextures)
                {
                    if (!textures.contains(textureIndex))
                    {
                        normalTexture = MakeHandle<Texture>(m_FileDirectory + textureURI, textureName, VK_FORMAT_R8G8B8A8_UNORM);
                        textures[textureIndex] = normalTexture;
                    }
                    else
                        normalTexture = textures.at(textureIndex);
                }

                //if(!samplers.contains(samplerIndex))
                //

                if (material["normalTexture"].contains("scale"))
                    normalStrength = material["normalTexture"]["scale"];
            }

            if (material.contains("pbrMetallicRoughness"))
            {
                const auto& pbr = material["pbrMetallicRoughness"];

                if (pbr.contains("baseColorFactor") && m_ImportProperties.importColor)
                    color = glm::vec3(pbr["baseColorFactor"][0], pbr["baseColorFactor"][1], pbr["baseColorFactor"][2]);
   
            
                if (pbr.contains("baseColorTexture") && m_ImportProperties.importAlbedoTextures)
                {
                    uint32_t textureIndex = pbr["baseColorTexture"]["index"];

                    //uint32_t samplerIndex = JSON["textures"][normalTextureIndex]["sampler"];
                    uint32_t imageIndex = JSON["textures"][textureIndex]["source"];

                    std::string textureName = JSON["images"][imageIndex]["name"];
                    std::string textureURI = JSON["images"][imageIndex]["uri"];
                
                    if (!textures.contains(textureIndex))
                    {
                        albedoTexture = MakeHandle<Texture>(m_FileDirectory + textureURI, textureName, VK_FORMAT_R8G8B8A8_SRGB);
                        textures[textureIndex] = albedoTexture;
                    }
                    else
                        albedoTexture = textures.at(textureIndex);
                    //if(!samplers.contains(samplerIndex))
                    //
                }
                if (pbr.contains("metallicRoughnessTexture"))
                {
                    uint32_t textureIndex = pbr["metallicRoughnessTexture"]["index"];

                    //uint32_t samplerIndex = JSON["textures"][normalTextureIndex]["sampler"];
                    uint32_t imageIndex = JSON["textures"][textureIndex]["source"];

                    std::string textureName = JSON["images"][imageIndex]["name"];
                    std::string textureURI = JSON["images"][imageIndex]["uri"];

                    if (m_ImportProperties.importMetalnessTextures)
                    {
                        if (!textures.contains(textureIndex))
                        {
                            metalnessTexture = MakeHandle<Texture>(m_FileDirectory + textureURI, textureName, VK_FORMAT_R8G8B8A8_UNORM);
                            textures[textureIndex] = metalnessTexture;
                        }
                        else
                            metalnessTexture = textures.at(textureIndex);
                    }
                    if (m_ImportProperties.importRoughnessTextures)
                    {
                        if (!textures.contains(textureIndex))
                        {
                            roughnessTexture = MakeHandle<Texture>(m_FileDirectory + textureURI, textureName, VK_FORMAT_R8G8B8A8_UNORM);
                            textures[textureIndex] = roughnessTexture;
                        }
                        else
                            roughnessTexture = textures.at(textureIndex);
                    }
                }

                if (pbr.contains("metallicFactor"))
                    metalness = pbr["metallicFactor"];
                if (pbr.contains("roughnessFactor"))
                    roughness = pbr["roughnessFactor"];
            }

            m_Materials.emplace_back(MakeHandle<Material>(name, color, metalness, roughness, normalStrength, albedoTexture, roughnessTexture, normalTexture, metalnessTexture));
        }

        for (const auto& [id, texture] : textures)
            m_Textures.emplace_back(texture);
    }

    json GLTFImporter::GetMeshJsonData()
    {
        std::ifstream jsonFile(m_FilePath);
        if (!jsonFile.is_open())
            return nullptr;

        std::string fileContent = std::string(std::istreambuf_iterator<char>(jsonFile), std::istreambuf_iterator<char>());

        jsonFile.close();

        return json::parse(fileContent);
    }

    std::vector<char> GLTFImporter::GetMeshBinaryData()
    {
        std::string binaryFilePath = m_FileDirectory + std::string(JSON["buffers"][0]["uri"]);

        std::ifstream binaryFile(binaryFilePath, std::ios::ate | std::ios::binary);
        if (!binaryFile.is_open())
            return std::vector<char>{};

        std::vector<char> m_BinaryData((size_t)binaryFile.tellg());

        binaryFile.seekg(0);
        binaryFile.read(m_BinaryData.data(), m_BinaryData.size());
        binaryFile.close();

        return m_BinaryData;
    }
}