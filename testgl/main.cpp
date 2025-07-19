#include <QApplication>
#include <QOpenGLWindow>
#include <QOpenGLFunctions_4_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QTime>
#include <random>

// 计算着色器代码 (移动到全局作用域)
const char* computeShaderSource = R"(
#version 430 core
layout(local_size_x = 16, local_size_y = 16) in;
layout(rgba32f, binding = 0) uniform image2D outputImage;
layout(rgba32f, binding = 1) uniform image2D accumImage;

uniform int frameCount;
uniform vec3 cameraPos;
uniform vec3 cameraTarget;
uniform vec3 cameraUp;
uniform float fov;
uniform vec2 resolution;

struct Ray {
    vec3 origin;
    vec3 direction;
};

struct HitRecord {
    vec3 position;
    vec3 normal;
    vec3 color;
    float t;
    bool isLight;
};

struct Sphere {
    vec3 center;
    float radius;
    vec3 color;
    bool isLight;
};

struct Plane {
    vec3 point;
    vec3 normal;
    vec3 color;
    bool isLight;
};

// 场景定义 (康奈尔盒子)
Sphere spheres[2] = {
    Sphere(vec3(0.27, 0.3, 0.35), 0.15, vec3(0.8, 0.8, 0.8), false), // 左侧球
    Sphere(vec3(0.73, 0.5, 0.65), 0.3,  vec3(0.8, 0.8, 0.8), false)  // 右侧球
};

Plane planes[6] = {
    Plane(vec3(0, 0, 0), vec3(0, 1, 0),  vec3(0.9, 0.9, 0.9), false), // 底面
    Plane(vec3(0, 1, 0), vec3(0, -1, 0), vec3(0.9, 0.9, 0.9), true),  // 顶面 (光源)
    Plane(vec3(0, 0, 0), vec3(1, 0, 0),  vec3(0.8, 0.1, 0.1), false), // 左墙 (红)
    Plane(vec3(1, 0, 0), vec3(-1, 0, 0), vec3(0.1, 0.8, 0.1), false), // 右墙 (绿)
    Plane(vec3(0, 0, 1), vec3(0, 0, -1), vec3(0.8, 0.8, 0.8), false), // 后墙
    Plane(vec3(0, 0, 0), vec3(0, 0, 1),  vec3(0.8, 0.8, 0.8), false)  // 前墙
};

// 球体求交
bool hitSphere(Sphere sphere, Ray ray, inout HitRecord rec) {
    vec3 oc = ray.origin - sphere.center;
    float a = dot(ray.direction, ray.direction);
    float b = 2.0 * dot(oc, ray.direction);
    float c = dot(oc, oc) - sphere.radius * sphere.radius;
    float disc = b * b - 4 * a * c;
    
    if (disc < 0.0) return false;
    
    float t = (-b - sqrt(disc)) / (2.0 * a);
    if (t < 0.001 || t > rec.t) return false;
    
    rec.t = t;
    rec.position = ray.origin + t * ray.direction;
    rec.normal = normalize(rec.position - sphere.center);
    rec.color = sphere.color;
    rec.isLight = sphere.isLight;
    return true;
}

// 平面求交
bool hitPlane(Plane plane, Ray ray, inout HitRecord rec) {
    float denom = dot(plane.normal, ray.direction);
    if (abs(denom) < 0.001) return false;
    
    float t = dot(plane.point - ray.origin, plane.normal) / denom;
    if (t < 0.001 || t > rec.t) return false;
    
    rec.t = t;
    rec.position = ray.origin + t * ray.direction;
    rec.normal = plane.normal;
    rec.color = plane.color;
    rec.isLight = plane.isLight;
    return true;
}

// 场景求交
bool hitWorld(Ray ray, inout HitRecord rec) {
    bool hitAnything = false;
    
    for (int i = 0; i < 2; i++) {
        if (hitSphere(spheres[i], ray, rec)) {
            hitAnything = true;
        }
    }
    
    for (int i = 0; i < 6; i++) {
        if (hitPlane(planes[i], ray, rec)) {
            hitAnything = true;
        }
    }
    
    return hitAnything;
}

// 随机数生成器
uint seed = uint(gl_GlobalInvocationID.x) * uint(1973) + uint(gl_GlobalInvocationID.y) * uint(9277) + uint(frameCount) * uint(26699);
float rand() {
    seed = (seed ^ uint(61)) ^ (seed >> uint(16));
    seed *= uint(9);
    seed = seed ^ (seed >> uint(4));
    seed *= uint(668265261);
    seed = seed ^ (seed >> uint(15));
    return float(seed) * (1.0 / 4294967296.0);
}

// 在单位球内生成随机点
vec3 randomInUnitSphere() {
    vec3 p;
    do {
        p = 2.0 * vec3(rand(), rand(), rand()) - vec3(1.0);
    } while (dot(p, p) >= 1.0);
    return p;
}

// 路径追踪函数
vec3 trace(Ray ray) {
    vec3 color = vec3(1.0);
    for (int depth = 0; depth < 5; depth++) {
        HitRecord rec;
        rec.t = 1e10;
        
        if (!hitWorld(ray, rec)) {
            return vec3(0.0);
        }
        
        if (rec.isLight) {
            return color * rec.color;
        }
        
        // 漫反射表面
        vec3 target = rec.position + rec.normal + randomInUnitSphere();
        ray = Ray(rec.position, normalize(target - rec.position));
        color *= rec.color;
    }
    return vec3(0.0); // 超过最大深度
}

void main() {
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    if (pixelCoords.x >= int(resolution.x) || pixelCoords.y >= int(resolution.y)) return;

    // 相机参数
    vec3 w = normalize(cameraTarget - cameraPos);
    vec3 u = normalize(cross(cameraUp, w));
    vec3 v = cross(w, u);
    
    float aspect = resolution.x / resolution.y;
    float halfHeight = tan(fov * 0.5);
    float halfWidth = aspect * halfHeight;
    
    // 生成光线
    vec2 uv = vec2(pixelCoords) / resolution;
    vec2 offset = vec2(rand(), rand()) / resolution;
    vec3 dir = normalize(u * (uv.x * 2.0 - 1.0 + offset.x) * halfWidth +
                v * (uv.y * 2.0 - 1.0 + offset.y) * halfHeight +
                w);
    
    Ray ray = Ray(cameraPos, dir);
    
    // 路径追踪
    vec3 sampleColor = trace(ray);
    
    // 累积颜色
    vec3 accumColor = imageLoad(accumImage, pixelCoords).rgb;
    vec3 newAccum = (accumColor * float(frameCount) + sampleColor) / float(frameCount + 1);
    
    imageStore(outputImage, pixelCoords, vec4(sampleColor, 1.0));
    imageStore(accumImage, pixelCoords, vec4(newAccum, 1.0));
}
)";

// 顶点着色器 (全屏四边形) (移动到全局作用域)
const char* vertexShaderSource = R"(
#version 430 core
layout(location = 0) in vec2 position;
out vec2 uv;
void main() {
    uv = position * 0.5 + 0.5;
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

// 片段着色器 (移动到全局作用域)
const char* fragmentShaderSource = R"(
#version 430 core
in vec2 uv;
out vec4 fragColor;
uniform sampler2D tex;
void main() {
    vec3 color = texture(tex, uv).rgb;
    // Gamma校正
    color = pow(color, vec3(1.0/2.2));
    fragColor = vec4(color, 1.0);
}
)";

class PathTracingWindow : public QOpenGLWindow, protected QOpenGLFunctions_4_3_Core {
public:
    PathTracingWindow() : frameCount(0) {
        setTitle("Path Tracer - Cornell Box");
        resize(800, 600);
        
        // 设置OpenGL 4.3核心上下文
        QSurfaceFormat format;
        format.setVersion(4, 3);
        format.setProfile(QSurfaceFormat::CoreProfile);
        setFormat(format);
    }
    
protected:
    void initializeGL() override {
        initializeOpenGLFunctions();
        glClearColor(0, 0, 0, 1);
        
        // 创建计算着色器程序
        computeProgram = new QOpenGLShaderProgram;
        computeProgram->addShaderFromSourceCode(QOpenGLShader::Compute, computeShaderSource);
        computeProgram->link();
        
        // 创建渲染程序
        renderProgram = new QOpenGLShaderProgram;
        renderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
        renderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
        renderProgram->link();
        
        // 创建全屏四边形
        GLfloat quadVertices[] = {
            -1.0f, -1.0f,
             1.0f, -1.0f,
            -1.0f,  1.0f,
             1.0f,  1.0f
        };
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void*)0);
        glEnableVertexAttribArray(0);
        
        // 创建纹理
        createTextures();
    }
    
    void createTextures() {
        QSize size = this->size() * devicePixelRatio();
        
        // 输出纹理
        glGenTextures(1, &outputTex);
        glBindTexture(GL_TEXTURE_2D, outputTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, size.width(), size.height(), 
                     0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        // 累积纹理
        glGenTextures(1, &accumTex);
        glBindTexture(GL_TEXTURE_2D, accumTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, size.width(), size.height(), 
                     0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        // 清除累积纹理
        std::vector<float> zeroData(size.width() * size.height() * 4, 0.0f);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size.width(), size.height(), 
                        GL_RGBA, GL_FLOAT, zeroData.data());
    }
    
    void resizeGL(int w, int h) override {
        createTextures();
        frameCount = 0;
    }
    
    void paintGL() override {
        frameCount++;
        
        // 设置计算着色器参数
        computeProgram->bind();
        computeProgram->setUniformValue("frameCount", frameCount);
        computeProgram->setUniformValue("cameraPos", QVector3D(0.5f, 0.5f, 1.5f));
        computeProgram->setUniformValue("cameraTarget", QVector3D(0.5f, 0.5f, 0.0f));
        computeProgram->setUniformValue("cameraUp", QVector3D(0.0f, 1.0f, 0.0f));
        computeProgram->setUniformValue("fov", 0.785f); // 45度
        computeProgram->setUniformValue("resolution", QVector2D(width(), height()));
        
        // 绑定图像
        glBindImageTexture(0, outputTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
        glBindImageTexture(1, accumTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        
        // 分派计算着色器
        QSize size = this->size() * devicePixelRatio();
        glDispatchCompute((size.width() + 15) / 16, (size.height() + 15) / 16, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        
        // 渲染全屏四边形
        glClear(GL_COLOR_BUFFER_BIT);
        renderProgram->bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, outputTex);
        renderProgram->setUniformValue("tex", 0);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        
        update();
    }
    
private:
    QOpenGLShaderProgram *computeProgram = nullptr;
    QOpenGLShaderProgram *renderProgram = nullptr;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint outputTex = 0;
    GLuint accumTex = 0;
    int frameCount;
};

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    PathTracingWindow window;
    window.show();
    return app.exec();
}