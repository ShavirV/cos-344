#version 330 core

/*
 * fragment4.glsl  –  P4 Fragment Shader
 * ========================================
 * Handles ALL rendering modes through uniforms:
 *
 * FLOOR MODE (isFloor=true):
 *   Full Phong point-light illumination.
 *   Colour = floor base colour × light colour × attenuation × glass tint
 *   No transparency.
 *
 * BALL MODE (isFloor=false):
 *   Colour from base ball colour OR colour texture (B toggle).
 *   Alpha from uniform alpha OR alpha texture (M toggle).
 *   NO lighting calculation on ball (spec: "should not affect ball perception").
 *   Just straight colour + alpha for translucency.
 *
 * PHONG LIGHTING (floor only):
 *   Follows Chapter 4 pseudocode from prescribed textbook.
 *   I = Ia*Ka + Id*Kd*max(0,L·N) + Is*Ks*max(0,R·V)^n
 *   Where:
 *     Ia = ambient light intensity
 *     Ka = ambient material coefficient
 *     Id = diffuse light intensity (attenuated by distance)
 *     Kd = diffuse material coefficient = floor colour
 *     Is = specular light intensity
 *     Ks = specular material coefficient
 *     n  = shininess
 *     L  = normalised light direction (from surface to light)
 *     N  = surface normal
 *     R  = reflect(-L, N) = reflection vector
 *     V  = normalised view direction (from surface to camera)
 *
 * GLASS TINTING:
 *   The light passes through the glass ball before hitting the floor.
 *   We approximate this by multiplying the light colour by the ball colour.
 *   Tint factor = ballColour (already normalised RGB).
 *   finalFloorColour = litColour × mix(vec3(1.0), ballColour.rgb, 0.6)
 */

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;

out vec4 fragColour;

// ---- Mode ----
uniform bool isFloor;

// ---- Textures ----
uniform bool useColourTex;
uniform bool useAlphaTex;
uniform sampler2D colourTexture;
uniform sampler2D alphaTexture;

// ---- Ball uniforms ----
uniform vec4  ballColour;   // RGBA: rgb=ball colour, a=alpha value
uniform float alphaValue;   // global alpha from +/- keys

// ---- Floor uniforms ----
uniform vec3  floorColour;

// ---- Light uniforms ----
uniform vec3  lightPos;     // world-space light position
uniform vec3  lightColour;
uniform vec3  cameraPos;    // world-space camera position (for specular)

// ---- Wireframe ----
uniform bool wireframe;

void main() {

    if (wireframe) {
        // Wireframe mode: just output the base colour at full opacity
        if (isFloor) {
            fragColour = vec4(floorColour, 1.0);
        } else {
            vec3 col = useColourTex ? texture(colourTexture, vUV).rgb : ballColour.rgb;
            fragColour = vec4(col, 1.0);
        }
        return;
    }

    // =====================================================================
    // BALL rendering
    // =====================================================================
    if (!isFloor) {
        // Base colour: either uniform ball colour or colour texture
        vec3 col;
        if (useColourTex) {
            // Multiply texture sample by ball colour so cycling colours works
            vec3 texCol = texture(colourTexture, vUV).rgb;
            col = texCol * ballColour.rgb * 1.6; // brighten slightly
            col = clamp(col, 0.0, 1.0);
        } else {
            col = ballColour.rgb;
        }

        // Alpha: uniform or from alpha texture
        float a;
        if (useAlphaTex) {
            // Alpha texture: white=opaque, black=transparent
            // Dimples become transparent at alphaValue, surface stays opaque
            float texAlpha = texture(alphaTexture, vUV).r;
            // texAlpha=1 → opaque (non-dimple), texAlpha=0 → transparent (dimple)
            a = mix(alphaValue, 1.0, texAlpha);
        } else {
            a = alphaValue;
        }

        fragColour = vec4(col, a);
        return;
    }

    // =====================================================================
    // FLOOR rendering — Phong point-light illumination
    // =====================================================================
    // Material properties
    float Ka = 0.15;  // ambient coefficient
    float Kd = 0.85;  // diffuse coefficient
    float Ks = 0.40;  // specular coefficient
    float shininess = 32.0;

    vec3 N = normalize(vNormal);

    // Light direction (surface → light)
    vec3 toLight = lightPos - vWorldPos;
    float dist = length(toLight);
    vec3 L = normalize(toLight);

    // Attenuation: quadratic falloff
    // Constants tuned so light covers a reasonable floor area
    float constant  = 1.0;
    float linear    = 0.09;
    float quadratic = 0.032;
    float atten = 1.0 / (constant + linear * dist + quadratic * dist * dist);

    // Ambient component
    vec3 ambient = Ka * lightColour * floorColour;

    // Diffuse component (Lambertian)
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = Kd * diff * lightColour * floorColour * atten;

    // Specular component (Phong)
    vec3 V = normalize(cameraPos - vWorldPos);  // surface → camera
    vec3 R = reflect(-L, N);                    // reflection of -L around N
    float spec = pow(max(dot(R, V), 0.0), shininess);
    vec3 specular = Ks * spec * lightColour * atten;

    // Sum Phong components
    vec3 lit = ambient + diffuse + specular;

    // Glass tinting: light passes through coloured glass ball
    // Mix between white (no tint) and ball colour (full tint), weight 0.55
    vec3 tint = mix(vec3(1.0), ballColour.rgb, 0.55);
    lit = lit * tint;

    fragColour = vec4(clamp(lit, 0.0, 1.0), 1.0);
}