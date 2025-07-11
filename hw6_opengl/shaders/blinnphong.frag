#version 120
varying vec3 FragPos;
varying vec3 Normal;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform bool specularEnabled; // 新增：高光开关

void main() {
   // 环境光
   float ambientStrength = 0.1;
   vec3 ambient = ambientStrength * lightColor;
   
   // 漫反射
   vec3 norm = normalize(Normal);
   vec3 lightDir = normalize(lightPos - FragPos);
   float diff = max(dot(norm, lightDir), 0.0);
   vec3 diffuse = diff * lightColor;
   
   // 镜面反射 (Blinn-Phong)
   vec3 specular = vec3(0.0);
   if (specularEnabled) {
       float specularStrength = 0.5;
       vec3 viewDir = normalize(viewPos - FragPos);
       vec3 halfwayDir = normalize(lightDir + viewDir);
       float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
       specular = specularStrength * spec * lightColor;
   }
   
   vec3 result = (ambient + diffuse + specular) * objectColor;
   gl_FragColor = vec4(result, 1.0);
}