#pragma once
#include <pch.hpp>

class Framebuffer {
public:
    Framebuffer() {}

    Framebuffer(unsigned int width, unsigned int height, bool useDepth = true)
        : width(width), height(height)
    {
        create(width, height, useDepth);
    }

    bool create(unsigned int width, unsigned int height, bool useDepth = true)
    {
        if (FBO != 0) return true;
        this->width = width;
        this->height = height;

        glGenFramebuffers(1, &FBO);
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);

        // 🎯 Create an integer color attachment for object ID picking
        glGenTextures(1, &colorTexture);
        glBindTexture(GL_TEXTURE_2D, colorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32UI, width, height, 0, GL_RGB_INTEGER, GL_UNSIGNED_INT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

        // Optional depth buffer
        if (useDepth) {
            glGenRenderbuffers(1, &RBO);
            glBindRenderbuffer(GL_RENDERBUFFER, RBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);
        }

        GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
        glDrawBuffers(1, drawBuffers);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cout << "ERROR::FRAMEBUFFER:: Framebuffer not complete!\n";
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return true;
    }

    void Bind() const { glBindFramebuffer(GL_FRAMEBUFFER, FBO); }
    void Unbind() const { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

    unsigned int GetColorTexture() const { return colorTexture; }

    struct PixelInfo {
        unsigned int ObjectID = 0;
        unsigned int DrawID = 0;
        unsigned int PrimID = 0;

        void Print() const {
            spdlog::debug("ObjectID: {}  DrawID: {}  PrimID: {}", ObjectID, DrawID, PrimID);
        }
    };

    PixelInfo ReadPixel(uint32_t x, uint32_t y)
    {
        // 🛑 Bounds Check: If pixel is outside framebuffer dimensions, return fallback
        if (x >= width || y >= height)
        {
            PixelInfo fallbackPixel;
            fallbackPixel.ObjectID = 0xFFFFFFFF; // Match default clear ID / no-entity ID
            return fallbackPixel;
        }

        // Bind buffer and perform actual GL readback
        Bind();

        PixelInfo pixel;
        // Assuming GL_RED_INTEGER / GL_UNSIGNED_INT read format for integer picking
        glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &pixel.ObjectID);

        Unbind();

        return pixel;
    }

    ~Framebuffer() {
        if (FBO) glDeleteFramebuffers(1, &FBO);
        if (colorTexture) glDeleteTextures(1, &colorTexture);
        if (RBO) glDeleteRenderbuffers(1, &RBO);
    }

private:
    unsigned int FBO = 0;
    unsigned int colorTexture = 0;
    unsigned int RBO = 0;
    unsigned int width = 0, height = 0;
};
