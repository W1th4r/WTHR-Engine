#pragma once
#include <string>
#include <glad/glad.h>
#include <stb_image.hpp>
#include <iostream>

struct Texture {
    unsigned int id = 0;
    std::string type;
    std::string path;

    Texture() {}
    // Constructor automatically loads the texture from file
    Texture(const std::string& path, const std::string& type)
        : path(path), type(type) {
        if (!LoadFromFile(path,type)) {
            std::cerr << "Texture failed to load: " << path << std::endl;
        }
    }

    void Bind(unsigned int unit = 0) const {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, id);
    }
    unsigned int LoadDefaultTexture() {
        unsigned int textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        // 4 bytes per pixel now (R, G, B, A)
        unsigned char data[] = {
            255, 0,   255, 255,   0,   0,   0,   255, // Row 1: Magenta, Black
            0,   0,   0,   255,   255, 0,   255, 255  // Row 2: Black, Magenta
        };

        // Change both formats to GL_RGBA
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        return textureID;
    }

    bool LoadFromFile(const std::string& path, std::string typep) {
        type = typep;
        int width, height, nrChannels;
        stbi_set_flip_vertically_on_load(true);
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
        if (!data) return false;

        GLenum format = GL_RGB;
        if (nrChannels == 1) format = GL_RED;
        else if (nrChannels == 3) format = GL_RGB;
        else if (nrChannels == 4) format = GL_RGBA;

        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        return true;
    }
};
