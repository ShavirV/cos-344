#version 330 core

/*
 * Handles:
 *   - MVP transform (custom matrices, no GLM math)
 *   - Texture coordinate pass-through
 *   - Normal transformation for lighting
 *   - Displacement mapping (actual vertex displacement along normal)
 *
 * Vertex layout (8 floats): x,y,z, nx,ny,nz, u,v
 *
 */

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

out vec3 vWorldPos;   // world-space position for lighting
out vec3 vNormal;     // world-space normal
out vec2 vUV;         // texture coords

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;  // = transpose(inverse(mat3(model)))

uniform bool  useDisplacement;
uniform float dispStrength;   // max displacement in local units (e.g. 0.08)
uniform sampler2D dispTexture;

void main() {
    vec3 pos = aPos;

    // Displacement: move vertex along its normal by texture value
    // dispValue=1.0 means no displacement, dispValue<1.0 means push inward
    if (useDisplacement) {
        float dispValue = texture(dispTexture, aUV).r;  // [0..1]
        // dispValue = 1 → no move; dispValue near 0 → push inward along -normal
        float offset = (dispValue - 1.0) * dispStrength;  // negative = inward
        pos += aNormal * offset;
    }

    // Transform to world space
    vec4 worldPos4 = model * vec4(pos, 1.0);
    vWorldPos = worldPos4.xyz;

    // Transform normal (use normalMatrix to handle non-uniform scale)
    vNormal = normalize(normalMatrix * aNormal);

    vUV = aUV;

    gl_Position = projection * view * worldPos4;
}