#version 330 core

// Data passed from the vertex shader:
in float ellipseRot;       // (unused here, but may be used for debugging or other effects)
in vec2 ellipseSemi;       // (unused here; the ellipse shape data)
in float uSquareSize;      // size (in pixels) of the bounding square
in vec3 vsphereCenter;     // sphere center in view space (from: viewMatrix * vec4(coneCenter,1))
in float vViewZ;           // depth of the sphere center in view space (typically, -vsphereCenter.z)
in float sphereR;          // sphere radius (R), computed as L*tan(coneAngle)

// Uniforms provided by the application:
uniform float focalLength;    // focal length in pixels
uniform mat4 invViewMatrix;   // inverse of the view matrix

// Output: the computed world-space position for each fragment (for demonstration)
out vec4 FragColor;

void main()
{
    // --- 1. Compute the view–space coordinate for this fragment ---
    // gl_PointCoord is in [0,1] with (0,0) at the bottom-left.
    // (If your coordinate system differs, adjust accordingly.)
    vec2 offsetPixel = (gl_PointCoord - vec2(0.5)); 
    // Multiply by uSquareSize to get an offset in pixels relative to the ellipse center.
    // Then, convert that pixel offset into view–space distance.
    // The conversion factor is (vViewZ / focalLength) since an object of 1 pixel at depth vViewZ
    // corresponds to (vViewZ / focalLength) view space units.
    vec2 viewOffset = offsetPixel * uSquareSize * (vViewZ / focalLength);
    
    // In our drawing, the point sprite is centered at vsphereCenter (in view space).
    // So, the candidate fragment’s view–space position on the image plane is:
    vec3 fragPosView = vec3(vsphereCenter.xy + viewOffset, vViewZ);
    
    // --- 2. Construct the ray from the camera (at view-space origin) through the fragment ---
    vec3 rayDir = normalize(fragPosView);
    
    // --- 3. Compute ray-sphere intersection in view space ---
    // Sphere in view space: center = vsphereCenter, radius = sphereR.
    // Ray: origin O = (0,0,0), direction = rayDir.
    // The standard quadratic is:
    //    t^2 + 2 t * dot(O - C, D) + (||O - C||^2 - r^2) = 0.
    // Here, O = (0,0,0) and C = vsphereCenter, so:
    vec3 oc = -vsphereCenter; // = (O - C)
    float b = dot(oc, rayDir);
    float c = dot(oc, oc) - sphereR * sphereR;
    float discriminant = b * b - c;
    
    if(discriminant < 0.0)
    {
        // In theory, all fragments inside the ellipse should hit the sphere.
        discard;
    }
    
    // Choose the nearest intersection (smallest positive t):
    float t = -b - sqrt(discriminant);
    if(t < 0.0)
        t = -b + sqrt(discriminant);
    
    // Intersection point in view space:
    vec3 intersectView = t * rayDir;
    
    // --- 4. Convert the intersection from view space to world space ---
    vec4 worldPos4 = invViewMatrix * vec4(intersectView, 1.0);
    vec3 worldPos = worldPos4.xyz / worldPos4.w;
    
    // For demonstration purposes, output the world position as color.
    // (For example, scale and bias it to [0,1] if needed.)
    FragColor = vec4(worldPos * 0.1 + 0.5, 1.0);
}


// OLD FILE

#version 460 core

// Inputs from the vertex shader:
in vec2 ellipseSemi;   // (a, b): ellipse semi-axes (in pixels) major minor
in float ellipseRot;   // ellipse rotation angle θ
in float uSquareSize;  // size of the bounding square (in pixels)

out vec4 FragColor;

void main(){
    // Translate gl_PointCoord from [0, 1] to [-0.5, 0.5]
    // The pivot point of gl_PointCoord is top left corner
    // Center it around (0,0): now range is [-0.5, 0.5]
    vec2 uv = gl_PointCoord - vec2(0.5);
    uv.y *= -1.0; // swap the y is negative at upper part. 
    // Convert to pixel coordinates relative to the square:
    vec2 pos = uv * uSquareSize;
    
    // --- 1. Rotate the coordinate by -θ to align with the ellipse’s local axes ---
    float cosTheta = cos(-ellipseRot);
    float sinTheta = sin(-ellipseRot);
    vec2 rotPos;
    // this formulas comes from 2d vector rotation matrix
    rotPos.x = pos.x * cosTheta - pos.y * sinTheta;
    rotPos.y = pos.x * sinTheta + pos.y * cosTheta;
    
    // --- 2. Evaluate the ellipse equation ---
    // ellipseSemi.x is semi-major axis
    // ellipseSemi.y is semi-minor axis
    // In the ellipse’s local coordinates the equation is:
    //    (x/a)² + (y/b)² <= 1
    float ellipseVal = (rotPos.x / ellipseSemi.x) * (rotPos.x / ellipseSemi.x)
                     + (rotPos.y / ellipseSemi.y) * (rotPos.y / ellipseSemi.y);
    
    if (ellipseVal > 1.0){
        discard;
    }

    float value = 1.0 - ellipseVal;
    FragColor = vec4(value, value, value, 1.0);
}





//////////////// new VERT
#version 460 core
layout (location = 0) in vec4 aSpherePos;

// Uniforms supplied by the application:
uniform mat4 viewMatrix;      // world-to-view matrix
uniform mat4 projMatrix;      // view-to-clip (projection) matrix
uniform float focalLength;    // focal length for projection scaling (in pixels)
uniform vec3 cameraPos;       // world-space camera position
uniform float u_MaxDiameter;  // maxium size of point
uniform vec4 u_frustumPlanes[6];

// Outputs to fragment shader:
out vec2 ellipseSemi;   // ellipse semi-axes (a, b) in screen space major:a , minor:b
out float ellipseRot;   // rotation angle (θ) of the ellipse (in screen space)
out float uSquareSize;  // size (in pixels) of the bounding square
out vec3 vsphereCenter; // sphere center in view space (from: viewMatrix * vec4(coneCenter,1))
out vec3 worldCenter;   // world center of the sphere
out float vViewZ;       // depth of the sphere center in view space (typically, -vsphereCenter.z)
out float sphereR;      // sphere radius (R), computed as L*tan(coneAngle)

bool IsSphereVisible(vec3 pos, float radius);

void main(){
    if (!IsSphereVisible(aSpherePos.xyz, aSpherePos.w)){
        gl_Position = vec4(-1000,-1000,-1000, 1.0);
        gl_PointSize = 0.0;
        return;
    }
    worldCenter = aSpherePos.xyz;
    vec3 coneCenter = aSpherePos.xyz;
    //vec3 axis = coneCenter - cameraPos;
    //float dist = length(axis);
    //float tanalpha = aSpherePos.w / sqrt(dist*dist - aSpherePos.w*aSpherePos.w);
    // --- 1. Compute cone parameters ---
    // The cone’s axis goes from the camera to coneCenter.
    //float L = length(axis);
    //vec3 coneDir = normalize(axis);
    
    // The radius R (in world units) of the circular cross-section
    // in the plane perpendicular to the cone’s axis:
    float radius = aSpherePos.w;
    sphereR = radius;
    // --- 2. Transform coneCenter to view space ---
    vec4 viewCenter4 = viewMatrix * vec4(coneCenter, 1.0);
    vec3 viewCenter = viewCenter4.xyz;
    vsphereCenter = viewCenter;
    // For a typical OpenGL camera (looking along -z), the depth is:
    float viewZ = -viewCenter.z;
    vViewZ = -viewZ;
    // --- 3. Compute ellipse rotation ---
    // In view space the cone axis is given by the normalized viewCenter.
    // Its projection onto the x-y plane gives the orientation.
    vec3 viewConeDir = normalize(viewCenter);
    ellipseRot = atan(viewConeDir.y, viewConeDir.x);
    
    // --- 4. Compute projected semi-axes ---
    // The angle Φ between the cone’s axis and the view direction (0,0,-1):
    float cosPhi = -viewConeDir.z; //dot(viewConeDir, vec3(0.0, 0.0, -1.0));
    if (cosPhi < 0.001){
        gl_Position = vec4(-1000,-1000,-1000, 1.0);
        gl_PointSize = 0.0;
        return;
    }
    //cosPhi = max(cosPhi, 0.001); // avoid division by zero
    
    // In a pinhole model, the projected radii are scaled by (focalLength/viewZ):
    float focalOverView = focalLength / viewZ;
    float a = (radius / cosPhi) * (focalOverView);   // semi-major axis
    float b = radius * (focalOverView);              // semi-minor axis
    ellipseSemi = vec2(a, b);
    
    // --- 5. Compute bounding square for the rotated ellipse ---
    float cosVal = cos(ellipseRot);
    float sinVal = sin(ellipseRot);
    float bbWidth  = 2.0 * sqrt(a*a * cosVal*cosVal + b*b * sinVal*sinVal);
    float bbHeight = 2.0 * sqrt(a*a * sinVal*sinVal + b*b * cosVal*cosVal);
    float squareSize = max(bbWidth, bbHeight);
    uSquareSize = squareSize;
    
    if (squareSize > u_MaxDiameter){
        gl_Position = vec4(-1000,-1000,-1000, 1.0);
        gl_PointSize = 0.0;
        return;
    }

    // Set point size (this defines the size of the axis-aligned square in window/pixel space)
    gl_PointSize = squareSize;
    
    // Compute final clip-space position (the ellipse center)
    gl_Position = projMatrix * viewCenter4;
}

bool IsSphereVisible(vec3 pos, float radius){
    for (int i = 0; i < 6; i++){
        // Calculate signed distance from sphere center to the plane
        float dist = dot(pos, u_frustumPlanes[i].xyz) + u_frustumPlanes[i].w;

        // If the sphere is entirely behind the plane, it's not visible
        if (dist < -radius*2.4) {
            return false;
        }
    }

    return true;
}
//////////////// new FRAG
#version 460 core

out vec4 FragColor;

// Inputs from the vertex shader:
in float uSquareSize;  // size of the bounding square (in pixels)
in vec3 vsphereCenter; // sphere center in view space (from: viewMatrix * vec4(coneCenter,1))
in vec3 worldCenter;   // world center of the sphere
in float vViewZ;           // depth of the sphere center in view space (typically, -vsphereCenter.z)
in float sphereR;          // sphere radius (R), computed as L*tan(coneAngle)

// Uniforms provided by the application:
uniform float focalLength;    // focal length in pixels
uniform mat4 invViewMatrix;   // inverse of the view matrix
uniform float u_FarDistance; // cameras far dist
uniform vec3 cameraPos;

void main(){
    // --- 1. Compute the view–space coordinate for this fragment ---
    // gl_PointCoord is in [0,1] with (0,0) at the bottom-left.
    // (If your coordinate system differs, adjust accordingly.)
    vec2 offsetPixel = gl_PointCoord - vec2(0.5);
    offsetPixel.y *= -1.0; // swap the y is negative at upper part. 

    // Multiply by uSquareSize to get an offset in pixels relative to the ellipse center.
    // Then, convert that pixel offset into view–space distance.
    // The conversion factor is (vViewZ / focalLength) since an object of 1 pixel at depth vViewZ
    // corresponds to (vViewZ / focalLength) view space units.
    vec2 viewOffset = offsetPixel * uSquareSize * (vViewZ / focalLength);
    
    // In our drawing, the point sprite is centered at vsphereCenter (in view space).
    // So, the candidate fragment’s view–space position on the image plane is:
    vec3 fragPosView = vec3(vsphereCenter.xy + viewOffset, vViewZ);
    
    // --- 2. Construct the ray from the camera (at view-space origin) through the fragment ---
    vec3 rayDir = normalize(fragPosView);
    
    // --- 3. Compute ray-sphere intersection in view space ---
    // Sphere in view space: center = vsphereCenter, radius = sphereR.
    // Ray: origin O = (0,0,0), direction = rayDir.
    // The standard quadratic is:
    //    t^2 + 2 t * dot(O - C, D) + (||O - C||^2 - r^2) = 0.
    // Here, O = (0,0,0) and C = vsphereCenter, so:
    vec3 oc = -vsphereCenter; // = (O - C)
    float b = 2.0 * dot(oc, rayDir);
    float c = dot(oc, oc) - sphereR * sphereR;
    float discriminant = b * b - 4.0 * c;
    
    if(discriminant < 0.0)
    {
        //FragColor = vec4(1.0, 0.0, 0.0, 1.0);
        // In theory, all fragments inside the ellipse should hit the sphere.
        discard;
    }
    
    // Choose the nearest intersection (smallest positive t):
    float t = -b - sqrt(discriminant);
    if(t < 0.0)
        t = -b + sqrt(discriminant);
    
    // Intersection point in view space:
    vec3 intersectView = t * rayDir;
    
    // --- 4. Convert the intersection from view space to world space ---
    vec4 worldPos4 = invViewMatrix * vec4(intersectView, 1.0);
    vec3 worldPos = worldPos4.xyz / worldPos4.w;
    
    gl_FragDepth = distance(worldPos, cameraPos) / u_FarDistance;
    vec3 fragNormal = (worldPos - worldCenter) / sphereR;

    // For demonstration purposes, output the world position as color.
    // (For example, scale and bias it to [0,1] if needed.)
    FragColor = vec4(fragNormal);
}


// vertex shader
void main(){
    vec3 sphereCenter = aSpherePos.xyz;
    float radius = aSpherePos.w;

    if (!IsSphereVisible(sphereCenter, radius)){
        gl_Position = vec4(-1000,-1000,-1000, 1.0);
        gl_PointSize = 0.0;
        return;
    }

    // Transform sphere center into view space.
    vec4 viewPos = u_View * vec4(sphereCenter, 1.0);
    
    // Compute clip space position.
    vec4 screenPosition = u_Proj * viewPos;

    // Compute screen-space extent using projection matrix terms
    float radOverZ = (radius / -viewPos.z);
    float delta_x = u_Proj[0][0] * radOverZ; // NDC x extent per unit radius
    float delta_y = u_Proj[1][1] * radOverZ; // NDC y extent per unit radius
    
    // Convert to pixel size and set gl_PointSize
    float width_pixels = delta_x * u_ScreenSize.x;
    float height_pixels = delta_y * u_ScreenSize.y;
    
    // Apply correction factor for off-center spheres
    float scale = 1.0 + 0.5 * dot(ndc, ndc); // Correction increases with distance from center

    // Set the point size to the diameter of the square bounding box
    float pointSize = max(width_pixels, height_pixels);

    if (pointSize > u_MaxDiameter){
        gl_Position = vec4(-1000,-1000,-1000, 1.0);
        gl_PointSize = 0.0;
        return;
    }

    // set out variables
    v_Sphere.center = sphereCenter;
    v_Sphere.camToCenter = sphereCenter - u_CamPos;
    v_Sphere.camToCenDir = normalize(v_Sphere.camToCenter);
    v_Sphere.radius = radius;
    v_Sphere.pointSize = pointSize;
    // set the values
    gl_Position = screenPosition;
    gl_PointSize = pointSize;
}

/// Last vertex shader
void main(){
    vec3 sphereCenter = aSpherePos.xyz;
    float radius = aSpherePos.w;

    if (!IsSphereVisible(sphereCenter, radius)){
        gl_Position = vec4(-1000,-1000,-1000, 1.0);
        gl_PointSize = 0.0;
        return;
    }
    
    vec4 viewCenter4 = u_View * vec4(sphereCenter, 1.0);
    vec3 viewConeDir = normalize(viewCenter4.xyz);
    float ellipseRot = atan(viewConeDir.y, viewConeDir.x);
    float cosPhi = -viewConeDir.z; //dot(viewConeDir, vec3(0.0, 0.0, -1.0));
    if (cosPhi < 0.001){
        gl_Position = vec4(-1000,-1000,-1000, 1.0);
        gl_PointSize = 0.0;
        return;
    }
    // In a pinhole model, the projected radii are scaled by (focalLength/viewZ):
    float focalOverView = u_focalLength / -viewCenter4.z;
    float a = (radius / cosPhi) * (focalOverView);   // semi-major axis
    float b = radius * (focalOverView);              // semi-minor axis
    float cosVal = cos(ellipseRot);
    float sinVal = sin(ellipseRot);
    float bbWidth  = a*a * cosVal*cosVal + b*b * sinVal*sinVal;
    float bbHeight = a*a * sinVal*sinVal + b*b * cosVal*cosVal;
    float pointSize = 2.0 * sqrt(max(bbWidth, bbHeight));

    if (pointSize > u_MaxDiameter){
        gl_Position = vec4(-1000,-1000,-1000, 1.0);
        gl_PointSize = 0.0;
        return;
    }

    // set out variables
    v_Sphere.center = sphereCenter;
    v_Sphere.camToCenter = sphereCenter - u_CamPos;
    v_Sphere.radius = radius;
    v_Sphere.pointSize = pointSize;
    // set the values
    gl_Position = u_Proj * viewCenter4;
    gl_PointSize = pointSize;
}


//////////////////// Vertex shader with trigonometry
void main(){
    vec3 sphereCenter = aSpherePos.xyz;
    float radius = aSpherePos.w;

    if (!IsSphereVisible(sphereCenter, radius)){
        gl_Position = vec4(-1000,-1000,-1000, 1.0);
        gl_PointSize = 0.0;
        return;
    }
    
    vec3 viewCenter = (u_View * vec4(sphereCenter, 1.0)).xyz;

    // tangent triangle
    float oc = length(viewCenter); // because camera in the origin
    float ot = sqrt(oc*oc - radius*radius);
    float ac = sqrt(oc*oc - viewCenter.z*viewCenter.z);
    // sin and cos for alpha and theta
    float sinAlpha = radius / oc;
    float cosAlpha = ot / oc;
    float sinTheta = ac / oc;
    float cosTheta = -viewCenter.z / oc;

    float diagonalSize = u_focalLength * 2.0 * sinAlpha * cosAlpha / ((cosTheta*cosAlpha - sinTheta*sinAlpha)*(cosTheta*cosAlpha + sinTheta*sinAlpha));
    // cos(x/ac) * diagonal
    float width_pixels = abs(diagonalSize * viewCenter.x / ac);
    float height_pixels = abs(diagonalSize * viewCenter.y / ac);

    float pointSize = max(width_pixels, height_pixels);

    if (pointSize > u_MaxDiameter){
        gl_Position = vec4(-1000,-1000,-1000, 1.0);
        gl_PointSize = 0.0;
        return;
    }

    // set out variables
    v_Sphere.center = sphereCenter;
    v_Sphere.camToCenter = sphereCenter - u_CamPos;
    v_Sphere.radius = radius;
    v_Sphere.pointSize = pointSize;
    // set the values
    gl_Position = u_ProjView * vec4(sphereCenter, 1.0);
    gl_PointSize = pointSize;
}


// --- Find the Ellipse Orientation ---
    vec2 screenDir = camViewDir.xy;

    float ellipseAngle = 0.0;
    // Avoid dividing by zero; if sphereCenter is exactly centered, default to 0 angle.
    if (length(screenDir) > 0.0) {
        ellipseAngle = atan(screenDir.y, screenDir.x);
    }
    // --- Calculate the Axis–Aligned Bounding Box ---
    // For an ellipse with semi-axes a (semiMajor) and b (semiMinor) rotated by ellipseAngle,
    // the axis–aligned bounding box dimensions are given by:
    //   width  = 2 * sqrt( a²*cos²(theta) + b²*sin²(theta) )
    //   height = 2 * sqrt( a²*sin²(theta) + b²*cos²(theta) )
    float bboxWidth = 2.0 * sqrt( (a * a) * (cos(ellipseAngle) * cos(ellipseAngle)) +
                                  (b * b) * (sin(ellipseAngle) * sin(ellipseAngle)) );
    float bboxHeight = 2.0 * sqrt( (a * a) * (sin(ellipseAngle) * sin(ellipseAngle)) +
                                   (b * b) * (cos(ellipseAngle) * cos(ellipseAngle)) );



/////////////////////////////// FRAGMENT SHADER USING GEOMETRIC SOLUTION
void main(){
    // Calculate the pixel offset from the defined center
    vec2 pixelOffset = gl_FragCoord.xy - v_Sphere.pixelCenter;

    // Convert pixel offset into a world offset using the focal length.
    // (Divide by focal length and then multiply by depth.)
    // (pixelOffset / u_FocalLength) * v_Sphere.viewZ;
    vec2 worldOffset = pixelOffset * v_Sphere.viewZOverFocalLength;

    // Compute the world position on the plane.
    // Since uPlaneCenter = uCameraPos + d * cameraForward,
    // the world point is the center plus the offset along right and up.
    vec3 worldPos = v_Sphere.center + worldOffset.x * u_CamRight + worldOffset.y * u_CamUp;

    vec3 viewDir = normalize(worldPos - u_CamPos);

    // range goes from [0,1] -> [-1,1]
    //vec2 ndc = (gl_FragCoord.xy / u_Resolution) * 2.0 - 1.0;
    //vec3 viewDir = normalize(u_CamForw + ndc.x*u_CamRight*u_TanHalfFOV.x + ndc.y*u_CamUp*u_TanHalfFOV.y);
    // vec4 clipPos = vec4(ndc, -1.0, 1.0);
    // vec4 viewPos = u_InvProj * clipPos;
    // vec3 viewDir = normalize((u_CamRotation * viewPos.xyz));
    // // Compute camera space direction
    // vec4 dirCam = vec4(ndc.x * u_TanHalfFOV.x, ndc.y * u_TanHalfFOV.y, -1.0, 0.0);

    // // Transform to world space
    // vec3 viewDir = normalize((u_InvViewMatrix * dirCam).xyz);



    // Ray-Sphere Intersection - Geometric Solution 
    float tCA = dot(v_Sphere.camToCenter, viewDir);
    // d^2 = L^2 - tCA^2
    float dSquare = dot(v_Sphere.camToCenter, v_Sphere.camToCenter) - tCA*tCA;
    float rSquare = v_Sphere.radius*v_Sphere.radius;
    if (dSquare > rSquare){
        //FragColor = vec4(1.0,0.0,0.0, 1.0);
        // d cannot be bigger than radius, if it's there is no interception
        discard;
    }

    float tHC = sqrt(rSquare - dSquare);
    float t1 = tCA - tHC; // intersection point 1
    // float t2 = tCA + tHC; // intersection point 2
    vec3 fragPos = u_CamPos + t1 * viewDir;
    vec3 fragNormal = (fragPos - v_Sphere.center) / v_Sphere.radius;
    vec3 fragColor = CalcDirectLight(u_DirLight, fragNormal, -viewDir, fragPos);
    gl_FragDepth = distance(fragPos, u_CamPos) / u_FarDistance;

    FragColor = vec4(fragColor, 1.0);
}