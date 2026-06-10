#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform vec3  objectColor;
uniform vec3  lightPos;
uniform vec3  lightColor;
uniform vec3  viewPos;
uniform float emissive;
uniform float alpha;

// Texture support
uniform bool      useTexture;
uniform sampler2D texSampler;

void main() {
    // Base color: dari texture atau objectColor
    vec3 baseColor;
    if (useTexture) {
        baseColor = texture(texSampler, TexCoord).rgb;
    } else {
        baseColor = objectColor;
    }

    // Ambient
    float ambientStrength = 0.40;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse
    vec3 norm     = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff    = max(dot(norm, lightDir), 0.0);
    vec3 diffuse  = diff * lightColor;

    // Specular
    float specularStrength = 0.3;
    vec3 viewDir    = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec      = pow(max(dot(viewDir, reflectDir), 0.0), 16);
    vec3 specular   = specularStrength * spec * lightColor;

    // Emissive
    vec3 emissiveColor = baseColor * emissive;

    vec3 result = (ambient + diffuse + specular) * baseColor + emissiveColor;
    FragColor   = vec4(result, alpha);
}
