#version 330 core

/*
 * vertex3d.glsl
 * =============
 * Receives pre-transformed position in LOCAL space.
 * MVP (Model * View * Projection) is applied here on the GPU.
 * All matrix construction happens on the CPU (custom mat4.hpp).
 *
 * Uniforms:
 *   model      – positions this object in the world
 *   view       – camera transform
 *   projection – perspective projection
 *
 * The spec says no GLM math functions — the matrices are built
 * with our own code and uploaded as uniform mat4 arrays.
 */

layout(location = 0) in vec3 aPos;    // local-space position
layout(location = 1) in vec3 aColour; // per-vertex colour

out vec3 vColour;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vColour = aColour;
}
