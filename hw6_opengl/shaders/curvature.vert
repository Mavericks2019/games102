#version 120
attribute vec3 aPos;
attribute vec3 aNormal;
attribute float aGaussianCurvature;  // 高斯曲率属性
attribute float aMeanCurvature;      // 平均曲率属性
attribute float aMaxCurvature;       // 最大曲率属性

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;
uniform int curvatureType;           // 曲率类型

varying vec3 FragPos;
varying vec3 Normal;
varying float vCurvature;            // 传递给片元着色器的插值曲率

void main() {
   FragPos = vec3(model * vec4(aPos, 1.0));
   Normal = normalMatrix * aNormal;
   
   // 根据曲率类型选择对应的曲率值
   if (curvatureType == 0) {         // 高斯曲率
       vCurvature = aGaussianCurvature;
   } else if (curvatureType == 1) {  // 平均曲率
       vCurvature = aMeanCurvature;
   } else {                          // 最大曲率
       vCurvature = aMaxCurvature;
   }
   
   gl_Position = projection * view * vec4(FragPos, 1.0);
}