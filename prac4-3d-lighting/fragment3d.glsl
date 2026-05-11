#version 330 core

/*
* handles rendering in GPU
* Floor mode - phong point-light illumination
* colour = floor base * light colour * attenuation * glass tint
* no transparency
*
* Ball mode - colour from base ball or texture (b)
* alpha from uniform alpha or use alpha texture (m)
* simple colour + alpha for transparency
*
* Glass tinting - light passes through ball before hitting floor 
* approximate by light col * ball col
*
*/

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;

out vec4 fragColour;

//two modes described above
uniform bool isFloor;

//textures
uniform bool useColourTex;
uniform bool useAlphaTex;
uniform sampler2D colourTexture;
uniform sampler2D alphaTexture;

//ball
uniform vec4  ballColour;   // RGBA: rgb=ball colour, a=alpha value
uniform float alphaValue;   // global alpha from +/- keys

//floor
uniform vec3  floorColour;

//lighting
uniform vec3  lightPos;     // world-space light position
uniform vec3  lightColour;
uniform vec3  cameraPos;    // world-space camera position (for specular)

uniform bool wireframe;

void main() {

    if (wireframe) {
        //Wireframe mode: just output the base colour at full opacity
        if (isFloor) {
            fragColour = vec4(floorColour, 1.0);
        } else {
            vec3 col = useColourTex ? texture(colourTexture, vUV).rgb : ballColour.rgb;
            fragColour = vec4(col, 1.0);
        }
        return;
    }

    //ball rendering 
    if (!isFloor) {
        //base colour: either uniform ball colour or colour texture
        vec3 col;
        if (useColourTex) {
            //multiply texture sample by ball colour so cycling colours works
            vec3 texCol = texture(colourTexture, vUV).rgb;
            col = texCol * ballColour.rgb * 1.6; // brighten slightly
            col = clamp(col, 0.0, 1.0);
        } else {
            col = ballColour.rgb;
        }

        //alpha: uniform or from alpha texture
        float a;
        if (useAlphaTex) {
            //alpha texture: white=opaque, black=transparent
            //dimples become transparent at alphaValue, surface stays opaque
            float texAlpha = texture(alphaTexture, vUV).r;
            // texAlpha=1 -> opaque (non-dimple), texAlpha=0 -> transparent (dimple)
            a = mix(alphaValue, 1.0, texAlpha);
        } else {
            a = alphaValue;
        }

        fragColour = vec4(col, a);
        return;
    }

    //floor rendering using phong lighting
    float Ka = 0.15;  // ambient coefficient
    float Kd = 0.85;  // diffuse coefficient
    float Ks = 0.40;  // specular coefficient
    float shininess = 32.0;

    vec3 N = normalize(vNormal);

    //light direction (surface -> light)
    vec3 toLight = lightPos - vWorldPos;
    float dist = length(toLight);
    vec3 L = normalize(toLight);

    //quadratic falloff for attenuation
    //Constants tuned so light covers a reasonable floor area
    float constant  = 1.0;
    float linear    = 0.09;
    float quadratic = 0.032;
    float atten = 1.0 / (constant + linear * dist + quadratic * dist * dist);

    //ambient component
    vec3 ambient = Ka * lightColour * floorColour;

    //diffuse component (Lambertian)
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = Kd * diff * lightColour * floorColour * atten;

    //specular component (Phong)
    vec3 V = normalize(cameraPos - vWorldPos);  // surface -> camera
    vec3 R = reflect(-L, N);                    // reflection of -L around N
    float spec = pow(max(dot(R, V), 0.0), shininess);
    vec3 specular = Ks * spec * lightColour * atten;

    //sum Phong components
    vec3 lit = ambient + diffuse + specular;

    //glass tinting: light passes through coloured glass ball
    //mix between white (no tint) and ball colour (full tint), weight 0.55
    vec3 tint = mix(vec3(1.0), ballColour.rgb, 0.55);
    lit = lit * tint;

    fragColour = vec4(clamp(lit, 0.0, 1.0), 1.0);
}