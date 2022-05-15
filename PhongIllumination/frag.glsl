#version 410

out vec4 frag_color;

in vec4 n;
in vec4 pos;
in vec4 light_pos;


vec3 g_light_pos = vec3(0, 0, 100);
vec3 g_light_color = vec3(1.f, 1.f, 1.f);

vec3 material_diffuse_color = vec3(1.f, 0.84f, 0.f);
vec4 material_specular_color_exp = vec4(1.f, 1.f, 1.f, 128.f);
vec4 material_specular_color_ex_exp = vec4(1.f, 1.f, 1.f, 128.f);
vec3 material_emissive_color = vec3(0.02f, 0.f, 0.05f);
vec3 material_ambient_color = vec3(0.08f, 0.0f, 0.0f);

vec3 computeDiffuseComponent(vec3 surface_normal, vec3 light_direction)
{
    return g_light_color * material_diffuse_color * clamp(dot(surface_normal, light_direction), 0.f, 1.f);  
}

vec3 computeSpecularComponent(vec3 surface_normal, vec3 light_direction, vec3 view_direction)
{
    vec3 reflection = reflect(-light_direction, surface_normal);
    reflection = normalize(reflection);
    view_direction = normalize(view_direction);
    float r_dot_v = clamp(dot(reflection, -view_direction), 0.f, 1.f);
    float exponent = material_specular_color_exp.w;
    return g_light_color * material_specular_color_exp.xyz * pow(r_dot_v, exponent);
}

// Not inverting the view_direction gives the shiny outline on the models surface.
// (This is wrong, but looks cool!)
vec3 computeSpecularComponentEx(vec3 surface_normal, vec3 light_direction, vec3 view_direction)
{
    vec3 reflection = reflect(-light_direction, surface_normal);
    reflection = normalize(reflection);
    view_direction = normalize(view_direction);
    float r_dot_v = clamp(dot(reflection, view_direction), 0.f, 1.f);
    float exponent = material_specular_color_ex_exp.w;
    return g_light_color * material_specular_color_ex_exp.xyz * pow(r_dot_v, exponent);
}

vec3 computeAmbientComponent()
{
    return g_light_color * material_ambient_color;
}

// NOTE: We pass pos.xyz as the view direction vector to the compute* functions
//       as both position and normal are passed from the vertex-shader stage 
//       with the model and view matrices applied. Thus, we are doing our lighting 
//       in view-space at which out camera is located at (0, 0, 0). 
void main () {
    
    vec3 surface_normal = normalize(n.xyz);
    vec3 light_direction = normalize(light_pos.xyz - pos.xyz);
    
    vec3 diffuse = computeDiffuseComponent(surface_normal, light_direction);
    vec3 specular = computeSpecularComponent(surface_normal, light_direction, pos.xyz);
    vec3 specular_ex = computeSpecularComponentEx(surface_normal, light_direction, pos.xyz);
    vec3 emissive = material_emissive_color;
    vec3 ambient = computeAmbientComponent();
    frag_color = vec4 (emissive + ambient + diffuse + specular, 1.f);
}
