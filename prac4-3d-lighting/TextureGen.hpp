#ifndef TEXTUREGEN_HPP
#define TEXTUREGEN_HPP

/*
 *all textures are a grid of dimples on a 512x512 canvas
 *hexagonal grid to look more like a real golf ball with offsets
 *
 *
 *DIMPLE LAYOUT:
 *  Even rows: circles at x = 0, spacing, 2*spacing, ...
 *  Odd rows:  circles at x = spacing/2, 3*spacing/2, ...
 *  Row spacing = spacing * sin(60°) ≈ spacing * 0.866
 *  Circle radius ≈ spacing * 0.38
 *
 */

#include <cmath>
#include <cstdio>
#include <vector>
#include <string>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class TextureGen {
public:
    static const int TEX_W = 512;
    static const int TEX_H = 512;
    static const int DIMPLE_SPACING = 48;  // pixels between dimple centres
    static const int DIMPLE_RADIUS  = 18;  // dimple circle radius in pixels

    static float inDimple(int px, int py) {
        float best = 0.0f;
        int spacing = DIMPLE_SPACING;
        float rowH = spacing * 0.866f;  // sin(60°) for hex packing
        int numRows = (int)(TEX_H / rowH) + 2;

        for (int row = -1; row < numRows; row++) {
            float cy = row * rowH;
            // Hex offset: odd rows shifted by half spacing
            float xOffset = (row & 1) ? spacing * 0.5f : 0.0f;
            int numCols = (int)(TEX_W / spacing) + 2;
            for (int col = -1; col < numCols; col++) {
                float cx = col * spacing + xOffset;
                float dx = px - cx;
                float dy = py - cy;
                // Wrap horizontally for seamless tiling
                if (dx >  TEX_W * 0.5f) 
                    dx -= TEX_W;
                if (dx < -TEX_W * 0.5f) 
                    dx += TEX_W;
                float dist = sqrtf(dx*dx + dy*dy);
                float t = 1.0f - dist / DIMPLE_RADIUS;
                if (t > best) 
                    best = t;
            }
        }
        return (best < 0.0f) ? 0.0f : (best > 1.0f) ? 1.0f : best;
    }

    // Generate colour texture: light grey bg, darker grey dimples
    static void writeColourTexture(const char* filename) {
        FILE* f = fopen(filename, "wb");
        if (!f) { fprintf(stderr, "Cannot open %s\n", filename); return; }
        fprintf(f, "P6\n%d %d\n255\n", TEX_W, TEX_H);
        for (int y = 0; y < TEX_H; y++) {
            for (int x = 0; x < TEX_W; x++) {
                float d = inDimple(x, y);
                // Base colour: light pearl grey (230,230,230)
                // Dimple centre: darker (140,140,150) with slight blue tint
                unsigned char r = (unsigned char)(230 - d * 90);
                unsigned char g = (unsigned char)(230 - d * 90);
                unsigned char b = (unsigned char)(230 - d * 80);
                fputc(r, f); fputc(g, f); fputc(b, f);
            }
        }
        fclose(f);
        printf("Written: %s\n", filename);
    }


    // Generate displacement texture: white=no displacement, black=push inward
    static void writeDisplacementTexture(const char* filename) {
        FILE* f = fopen(filename, "wb");
        if (!f) { fprintf(stderr, "Cannot open %s\n", filename); return; }
        fprintf(f, "P6\n%d %d\n255\n", TEX_W, TEX_H);
        for (int y = 0; y < TEX_H; y++) {
            for (int x = 0; x < TEX_W; x++) {
                float d = inDimple(x, y);
                // 1.0 at dimple centre = max inward displacement (dark)
                unsigned char v = (unsigned char)(255 - (int)(d * 255));
                // Smooth cosine shaping for realistic dimple depth
                float dcos = 0.5f * (1.0f - cosf(d * (float)M_PI));
                v = (unsigned char)(255 - (int)(dcos * 200));
                fputc(v, f); fputc(v, f); fputc(v, f);
            }
        }
        fclose(f);
        printf("Written: %s\n", filename);
    }

    // Generate alpha texture: white=opaque, black=transparent (dimple holes)
    static void writeAlphaTexture(const char* filename) {
        FILE* f = fopen(filename, "wb");
        if (!f) { fprintf(stderr, "Cannot open %s\n", filename); return; }
        fprintf(f, "P6\n%d %d\n255\n", TEX_W, TEX_H);
        for (int y = 0; y < TEX_H; y++) {
            for (int x = 0; x < TEX_W; x++) {
                float d = inDimple(x, y);
                // Hard threshold: inside dimple = black (transparent), outside = white
                // Smooth edge using d > 0.6 as threshold
                float alpha = (d > 0.5f) ? (1.0f - (d - 0.5f) * 2.0f) : 1.0f;
                unsigned char v = (unsigned char)(alpha * 255);
                fputc(v, f); fputc(v, f); fputc(v, f);
            }
        }
        fclose(f);
        printf("Written: %s\n", filename);
    }


    // Load a PPM P6 file as an OpenGL texture, return texture ID
    static unsigned int loadPPM(const char* filename) {
        FILE* f = fopen(filename, "rb");
        if (!f) { fprintf(stderr, "Cannot open texture: %s\n", filename); return 0; }

        char magic[3];
        int w, h, maxval;
        if (fscanf(f, "%2s %d %d %d ", magic, &w, &h, &maxval) != 4) {
            fclose(f); fprintf(stderr, "Bad PPM header: %s\n", filename); return 0;
        }

        int pixels = w * h * 3;
        std::vector<unsigned char> data(pixels);
        if ((int)fread(data.data(), 1, pixels, f) != pixels) {
            fclose(f); fprintf(stderr, "Short read: %s\n", filename); return 0;
        }
        fclose(f);

        unsigned int texID;
        glGenTextures(1, &texID);
        glBindTexture(GL_TEXTURE_2D, texID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, data.data());
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        return texID;
    }

    // Analytical sampling (for CPU-side displacement at vertex build time)
    // u,v in [0,1] UV space -> displacement value in [0,1]
    // 0 = dimple centre (maximum inward), 1 = surface (no displacement)
    static float sampleDisplacement(float u, float v) {
        int px = (int)(u * TEX_W) % TEX_W;
        int py = (int)(v * TEX_H) % TEX_H;
        if (px < 0) px += TEX_W;
        if (py < 0) py += TEX_H;
        float d = inDimple(px, py);
        float dcos = 0.5f * (1.0f - cosf(d * (float)M_PI));
        return 1.0f - dcos * 0.8f;  // [0.2 .. 1.0], 0.2 at deepest dimple
    }


    // Generate all three textures and return; call once at startup
    static void generateAll() {
        writeColourTexture("colour_texture.ppm");
        writeDisplacementTexture("displacement_texture.ppm");
        writeAlphaTexture("alpha_texture.ppm");
    }
};

#endif // TEXTUREGEN_HPP


