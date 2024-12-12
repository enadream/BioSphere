#version 330 core

out vec4 FragColor;

uniform vec3 sphereCenter; // Center of the sphere in world space
uniform float sphereRadius; // Radius of the sphere
uniform vec3 cameraPos; // Camera position
uniform vec3 lightPos; // Light position
uniform vec3 sphereColor; // Base color of the sphere

void main() {
    // Normalized screen coordinates (-1 to 1)
    vec2 uv = (gl_FragCoord.xy / vec2(800, 600)) * 2.0 - 1.0;
    uv.y *= -1.0; // Flip Y-axis for OpenGL coordinate space

    // Ray direction from the camera
    vec3 rayDir = normalize(vec3(uv, 1.0));

    // Vector from camera to sphere center
    vec3 toSphere = sphereCenter - cameraPos;

    // Calculate the projection of toSphere onto the rayDir
    float projLength = dot(toSphere, rayDir);
    vec3 projPoint = projLength * rayDir;

    // Distance from sphereCenter to the ray
    float distToRay = length(toSphere - projPoint);

    // Determine alpha based on the distance to the sphere
    float edgeThreshold = 0.01; // Controls the width of the smooth edge
    float alpha = 1.0 - smoothstep(sphereRadius, sphereRadius + edgeThreshold, distToRay);

    // If the pixel is outside the sphere and beyond the smoothing threshold, discard it
    if (alpha < 0.0) {
        discard;
    }

    // Calculate intersection point for shading
    float thc = sqrt(sphereRadius * sphereRadius - distToRay * distToRay);
    float t0 = projLength - thc; // Near intersection
    vec3 hitPoint = cameraPos + t0 * rayDir;

    // Calculate normal at the hit point
    vec3 normal = normalize(hitPoint - sphereCenter);

    // Simple shading
    vec3 lightDir = normalize(lightPos - hitPoint);
    float diff = max(dot(normal, lightDir), 0.0);

    // Final color with alpha blending
    FragColor = vec4(sphereColor * diff, alpha);
}
