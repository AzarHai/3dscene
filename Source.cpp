#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <iostream>
#include <vector>
#include <locale.h>
#include <windows.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <random>
// Shader sources
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
out vec2 TexCoords;
out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos, 1.0);
    TexCoords = aTexCoords;

}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 lightPos[6];
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform sampler2D terrainTexture;
uniform vec4 objectColor;

void main()
{
    // Ambient lighting
    float ambientStrength = 0.001;
    vec3 ambient = ambientStrength * lightColor;

    vec3 norm = normalize(Normal);
    vec3 result = ambient;

    // Add lighting for each light source
    for (int i = 0; i < 6; ++i) {
        vec3 lightDir = normalize(lightPos[i] - FragPos);

        // Diffuse lighting
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor;

        // Specular lighting
        float specularStrength = 0.2;
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        vec3 specular = specularStrength * spec * lightColor;

        result += diffuse + specular;
    }


    vec4 textureColor = texture(terrainTexture, TexCoords);


    FragColor = vec4(result, 1.0) * textureColor * objectColor;
}

)";

// Загрузка текстуры
unsigned int loadTexture(const char *path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data) {
        glBindTexture(GL_TEXTURE_2D, textureID);


        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


        GLenum format;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cerr << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);
    return textureID;
}


// Function to compile shaders
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cout << "Shader compilation error: " << infoLog << std::endl;
    }
    return shader;
}

// Initialize shader program
GLuint initShaderProgram() {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}

// Generate terrain (plane)
void generateTerrain(std::vector<float>& vertices, std::vector<unsigned int>& indices, int size) {
    float scale = 0.201f;  // Масштаб для террейна
    float hillHeight = 7.0f; // Высота холма
    int hillSize = 300;  // Общий размер холма
    int plateauSize = 50; // Размер плоской вершины

    for (int z = 0; z < size; ++z) {
        for (int x = 0; x < size; ++x) {
            float height = 0.0f;

            int centerX = size / 2;
            int centerZ = size / 2;
            int dx = x - centerX;
            int dz = z - centerZ;
            float distance = sqrt(dx * dx + dz * dz);

            if (distance < plateauSize) {

                height = hillHeight;
            } else if (distance < hillSize) {

                height = hillHeight - (hillHeight * ((distance - plateauSize) / (hillSize - plateauSize)));
            }


            vertices.push_back(x * scale);
            vertices.push_back(height); // Высота террейна
            vertices.push_back(z * scale);
            vertices.push_back(0.0f); // Нормаль X
            vertices.push_back(1.0f); // Нормаль Y
            vertices.push_back(0.0f); // Нормаль Z
            vertices.push_back(static_cast<float>(x) / size); // Текстурная координата S
            vertices.push_back(static_cast<float>(z) / size); // Текстурная координата T
            // Создаем индексы
            if (x < size - 1 && z < size - 1) {
                int topLeft = z * size + x;
                int topRight = topLeft + 1;
                int bottomLeft = (z + 1) * size + x;
                int bottomRight = bottomLeft + 1;
                indices.push_back(topLeft);
                indices.push_back(bottomLeft);
                indices.push_back(topRight);
                indices.push_back(topRight);
                indices.push_back(bottomLeft);
                indices.push_back(bottomRight);
            }
        }
    }
}

// Camera control variables
//float cameraSpeed = 10.0f;
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = 400, lastY = 300;
const float sensitivity = 0.1f;
bool firstMouse = true;
glm::vec3 cameraPos(0.0f, 4.0f, 10.0f);
glm::vec3 cameraFront(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);


// Callback for window resizing
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Mouse callback function
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    // Adjust yaw and pitch based on mouse movement
    yaw += xoffset * sensitivity;
    pitch += yoffset * sensitivity;

    // Limit pitch to avoid gimbal lock
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    // Update camera front vector
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}
// Функция для поиска высоты террейна на данной позиции (x, z)
float getTerrainHeight(const std::vector<float>& vertices, int size, float x, float z) {
    // Преобразуем координаты в индексы массива вершин
    int iX = static_cast<int>(x / 0.201f); // Преобразуем в координаты террейна
    int iZ = static_cast<int>(z / 0.201f);

    if (iX < 0 || iX >= size || iZ < 0 || iZ >= size) {
        return 0.0f; // Возвращаем 0, если точка выходит за пределы террейна
    }

    // Индекс вершины для поиска
    int index = (iZ * size + iX) * 8 + 1; // Вершины с координатами Y идут через 8 элементов (x, y, z, nx, ny, nz, s, t)
    return vertices[index]; // Возвращаем высоту (y-координату) для точки
}

// Обновление позиции камеры с учетом ограничения по террейну
void processInput(GLFWwindow *window, const std::vector<float>& terrainVertices, int terrainSize) {
    float cameraSpeed = 0.1f; // скорость движения камеры
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Движение камеры
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront; // Вперед
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront; // Назад
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed; // Влево
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed; // Вправо
    /*if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraUp; // Вверх
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraUp; // Вниз*/

    // Ограничение движения камеры в пределах террейна по осям X и Z
    float terrainMinX = 0.0f, terrainMaxX = terrainSize * 0.2f;
    float terrainMinZ = 0.0f, terrainMaxZ = terrainSize * 0.2f;

    // Ограничиваем положение камеры по X и Z с учетом высоты террейна
    cameraPos.x = glm::clamp(cameraPos.x, terrainMinX, terrainMaxX);
    cameraPos.z = glm::clamp(cameraPos.z, terrainMinZ, terrainMaxZ);

    // Получаем высоту террейна на текущих координатах камеры
    float terrainHeight = getTerrainHeight(terrainVertices, terrainSize, cameraPos.x, cameraPos.z);

    // Если высота террейна ниже определенного порога, устанавливаем минимальную высоту камеры
    if (terrainHeight < 0.0f) {
        terrainHeight = 0.0f;
    }

    cameraPos.y = terrainHeight + 4.0f;
}
void generateCube(std::vector<float>& vertices, std::vector<float>& textureCoords, float size, float height, float yOffset) {
    float halfSize = size * 0.5f;

    vertices = {
        // Нижняя грань
        -halfSize, yOffset, -halfSize,  // Нижний левый угол
         halfSize, yOffset, -halfSize,  // Нижний правый угол
         halfSize, yOffset,  halfSize,  // Верхний правый угол
        -halfSize, yOffset,  halfSize,  // Верхний левый угол

        // Верхняя грань
        -halfSize, yOffset + height, -halfSize,  // Нижний левый угол
         halfSize, yOffset + height, -halfSize,  // Нижний правый угол
         halfSize, yOffset + height,  halfSize,  // Верхний правый угол
        -halfSize, yOffset + height,  halfSize   // Верхний левый угол
    };

    // Координаты текстуры для каждой вершины
    textureCoords = {
        // Нижняя грань
        0.0f, 0.0f,  // Нижний левый угол
        1.0f, 0.0f,  // Нижний правый угол
        1.0f, 1.0f,  // Верхний правый угол
        0.0f, 1.0f,  // Верхний левый угол

        // Верхняя грань
        0.0f, 0.0f,  // Нижний левый угол
        1.0f, 0.0f,  // Нижний правый угол
        1.0f, 1.0f,  // Верхний правый угол
        0.0f, 1.0f   // Верхний левый угол
    };
}


// Cube indices for drawing the faces
std::vector<unsigned int> generateCubeIndices() {
    return {
        // Bottom face
        0, 1, 2,  2, 3, 0,
        // Top face
        4, 5, 6,  6, 7, 4,
        // Sides
        0, 1, 5,  5, 4, 0,
        1, 2, 6,  6, 5, 1,
        2, 3, 7,  7, 6, 2,
        3, 0, 4,  4, 7, 3
    };
}
struct Model {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    GLuint VAO, VBO, EBO;

    Model(const std::string& path) {
        loadModel(path);
        setupModel();
    }

    void loadModel(const std::string& path) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())) {
            std::cerr << warn << err << std::endl;
            return;
        }

        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                vertices.push_back(attrib.vertices[3 * index.vertex_index + 0]);
                vertices.push_back(attrib.vertices[3 * index.vertex_index + 1]);
                vertices.push_back(attrib.vertices[3 * index.vertex_index + 2]);

                if (!attrib.normals.empty()) {
                    vertices.push_back(attrib.normals[3 * index.normal_index + 0]);
                    vertices.push_back(attrib.normals[3 * index.normal_index + 1]);
                    vertices.push_back(attrib.normals[3 * index.normal_index + 2]);
                }

                if (!attrib.texcoords.empty()) {
                    vertices.push_back(attrib.texcoords[2 * index.texcoord_index + 0]);
                    vertices.push_back(attrib.texcoords[2 * index.texcoord_index + 1]);
                }

                indices.push_back(indices.size());
            }
        }
    }

    void setupModel() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
    }

    void draw(GLuint shaderProgram) {
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};
glm::vec3 rabbitPosition(1.0f, 0.0f, 1.0f); // Начальная позиция
glm::vec3 rabbitFront(0.0f, 0.0f, -1.0f);   // Направление движения
void processRabbitInput(GLFWwindow* window, const std::vector<float>& terrainVertices, int terrainSize) {
    float rabbitSpeed = 0.1f; // Скорость перемещения кролика
    glm::vec3 right = glm::normalize(glm::cross(rabbitFront, glm::vec3(0.0f, 1.0f, 0.0f)));

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        rabbitPosition -= rabbitSpeed * rabbitFront; // Вперёд
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        rabbitPosition += rabbitSpeed * rabbitFront; // Назад
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        rabbitPosition += right * rabbitSpeed; // Влево
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        rabbitPosition -= right * rabbitSpeed; // Вправо
    }

    // Ограничить движение модели по X и Z
    float terrainMinX = 0.0f, terrainMaxX = terrainSize * 0.2f;
    float terrainMinZ = 0.0f, terrainMaxZ = terrainSize * 0.2f;

    rabbitPosition.x = glm::clamp(rabbitPosition.x, terrainMinX, terrainMaxX);
    rabbitPosition.z = glm::clamp(rabbitPosition.z, terrainMinZ, terrainMaxZ);

    // Обновить высоту модели на основании террейна
    rabbitPosition.y = getTerrainHeight(terrainVertices, terrainSize, rabbitPosition.x, rabbitPosition.z);
}
int main() {
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(800, 600, "Witches tea party", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    glewInit();

    glEnable(GL_DEPTH_TEST); // Enable depth testing
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback); // Set mouse callback

    // Hide the mouse cursor and capture it
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    GLuint shaderProgram = initShaderProgram();
    GLuint texture1 = loadTexture("terrain_texture.png"); // Загрузка текстуры
    GLuint texture2 = loadTexture("white.jpg"); // Загрузка текстуры для куба
    GLuint texture3 = loadTexture("Round_table_texture_.jpg");
    GLuint texture4 = loadTexture("Round table texture _NRM.jpg");
    GLuint texture5 = loadTexture("Round table texture .jpg");
    //glActiveTexture(GL_TEXTURE0);



    //glUniform1i(glGetUniformLocation(shaderProgram, "terrainTexture"), 0);

    Model objModel("table.obj");
    Model objModel1("13518_Beach_Umbrella_v1_L3.obj");
    Model objModel2("Garden chair.obj");
    Model objModel3("uploads_files_5014646_Rabbit_Quad.obj");
    Model objModel4("teamugblend.obj");
    Model objModel5("20900_Brown_Betty_Teapot_v1.obj");
    Model objModel6("uploads_files_5014646_Rabbit_Quad1.obj");
    Model objModel7("untitled.obj");
    int terrainSize = 300;
    float cubeSize = terrainSize * 0.2f;
    float terrainYOffset = 0.0f;
    float cubeYOffset = -0.001f;
    float cubeHeight = 20.0f;
    float squareLightSize = 10.0f;
    float cutOff = glm::cos(glm::radians(90.0f));

    std::vector<float> terrainVertices;
    std::vector<unsigned int> terrainIndices;
    generateTerrain(terrainVertices, terrainIndices, terrainSize);



    GLuint VBO_Terrain, VAO_Terrain, EBO_Terrain;

    glGenVertexArrays(1, &VAO_Terrain);
    glGenBuffers(1, &VBO_Terrain);
    glGenBuffers(1, &EBO_Terrain);

    glBindVertexArray(VAO_Terrain);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_Terrain);
    glBufferData(GL_ARRAY_BUFFER, terrainVertices.size() * sizeof(float), terrainVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_Terrain);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, terrainIndices.size() * sizeof(unsigned int), terrainIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
glEnableVertexAttribArray(2);


std::vector<float> textureCoords;
std::vector<float> cubeVertices;
generateCube(cubeVertices, textureCoords, cubeSize, cubeHeight, cubeYOffset);
std::vector<unsigned int> cubeIndices = generateCubeIndices();

    GLuint VBO_Cube, VAO_Cube, EBO_Cube;
    glGenVertexArrays(1, &VAO_Cube);
    glGenBuffers(1, &VBO_Cube);
    glGenBuffers(1, &EBO_Cube);

    glBindVertexArray(VAO_Cube);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Cube);
    glBufferData(GL_ARRAY_BUFFER, cubeVertices.size() * sizeof(float), cubeVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_Cube);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, cubeIndices.size() * sizeof(unsigned int), cubeIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

glm::vec4 objectColor(1.0f, 0.0f, 0.0f, 1.0f);


GLuint objectColorLocation = glGetUniformLocation(shaderProgram, "objectColor");
glUniform4fv(objectColorLocation, 1, glm::value_ptr(objectColor));

float orbitRadius = 17.0f;
float orbitSpeed = 0.5f;
float angle = 0.0f;
glm::vec3 rotationCenter(150.0f * 0.2f, 6.0f, 150.0f * 0.2f);
float spacing = 0.2f;
float maxJumpHeight = 2.0f;
float jumpDuration = 0.5f;
int numRabbits = 31;
std::vector<float> jumpTimers(numRabbits, 0.0f);
std::vector<bool> isJumping(numRabbits, false);

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<> dis(0.0, 1.0);

    while (!glfwWindowShouldClose(window)) {
        processInput(window,terrainVertices, terrainSize);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderProgram);
        processRabbitInput(window, terrainVertices, terrainSize);

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);


        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));


        glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"), cameraPos.x, cameraPos.y, cameraPos.z);
        glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 0.8f, 0.8f, 0.8f);
        //glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.0f, 1.0f, 0.0f);
        glUniform1f(glGetUniformLocation(shaderProgram, "cutOff"), cutOff);

        // Draw terrain
        glUniform4fv(objectColorLocation, 1, glm::value_ptr(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f))); // Белый цвет для террейна
// Рисуйте террейн
            glBindTexture(GL_TEXTURE_2D, texture1);
        glBindVertexArray(VAO_Terrain);
        glDrawElements(GL_TRIANGLES, terrainIndices.size(), GL_UNSIGNED_INT, 0);


// Рисуйте куб

        // Установите позиции для 5 граней куба
glm::vec3 lightPos[6] = {
    glm::vec3(-terrainSize * 0.5f, 50.0f, -terrainSize * 0.5f), // Top-left corner
    glm::vec3( terrainSize * 0.5f, 50.0f, -terrainSize * 0.5f), // Top-right corner
   glm::vec3(-terrainSize * 0.5f, 50.0f,  terrainSize * 0.5f), // Bottom-left corner
    glm::vec3( terrainSize * 0.5f, 50.0f,  terrainSize * 0.5f), // Bottom-right corner
    glm::vec3(0.0f, 50.0f, 0.0f), // Center of the terrain
};
angle += orbitSpeed * glfwGetTime();
glfwSetTime(0.0);
for (int i = 0; i < numRabbits; ++i) {
    float currentAngle = angle + i * spacing;

    // Вычисляем позицию для вращения
    float x = rotationCenter.x + orbitRadius * cos(currentAngle);
    float z = rotationCenter.z + orbitRadius * sin(currentAngle);
    float y = rotationCenter.y;

    // Рандомно инициируем прыжок с небольшой вероятностью
    if (!isJumping[i] && dis(gen) < 0.01) {
        isJumping[i] = true;
        jumpTimers[i] = jumpDuration; // Запуск таймера прыжка
    }

    // Обрабатываем движение прыжка
    if (isJumping[i]) {
        float jumpProgress = 1.0f - (jumpTimers[i] / jumpDuration);
        y += maxJumpHeight * sin(jumpProgress * glm::pi<float>()); // Параболическая кривая прыжка

        // Обновляем таймер прыжка и проверяем, завершился ли прыжок
        jumpTimers[i] -= 0.016f; // Предполагаем 16 мс на кадр
        if (jumpTimers[i] <= 0.0f) {
            isJumping[i] = false; // Сбрасываем состояние прыжка
            jumpTimers[i] = 0.0f;
        }
    }

    // Создаем модельную матрицу трансформации
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(x, y, z));
    model = glm::scale(model, glm::vec3(2.5f, 2.5f, 2.5f)); // Масштабируем зайца

    // Рассчитываем вектор направления к центру вращения
    glm::vec3 direction = glm::normalize(rotationCenter - glm::vec3(x, y, z));
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(up, direction));
    glm::vec3 adjustedUp = glm::cross(direction, right);

    // Поворотная матрица, чтобы зaец смотрел в центр
    glm::mat4 rotationMatrix = glm::mat4(1.0f);
    rotationMatrix[0] = glm::vec4(right, 0.0f);
    rotationMatrix[1] = glm::vec4(adjustedUp, 0.0f);
    rotationMatrix[2] = glm::vec4(direction, 0.0f);

    model *= rotationMatrix;

    // Передаем матрицу трансформации в шейдер и рендерим зайца
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glBindTexture(GL_TEXTURE_2D, texture2);
    objModel3.draw(shaderProgram);
}
glm::mat4 rabbitModel = glm::translate(glm::mat4(1.0f), rabbitPosition);
glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(rabbitModel));
objModel6.draw(shaderProgram);
//стол
glm::mat4 model1 = glm::mat4(1.0f);
model1 = glm::translate(model1, glm::vec3(150.0f*0.2f, 7.0f, 150.0f*0.2f));

// Передаем матрицу модели в шейдер для первого объекта
GLuint modelLoc1 = glGetUniformLocation(shaderProgram, "model");
glUniformMatrix4fv(modelLoc1, 1, GL_FALSE, glm::value_ptr(model1));

// Устанавливаем цвет для первого объекта
glUniform4fv(objectColorLocation, 1, glm::value_ptr(glm::vec4(0.5f, 0.5f, 0.5f, 1.0f)));
glBindTexture(GL_TEXTURE_2D, texture3);
objModel.draw(shaderProgram);

//чайник
glm::mat4 model13 = glm::mat4(1.0f);
model13 = glm::translate(model13, glm::vec3(150.0f*0.2f-0.75f, 8.5f, 150.0f*0.2f-0.3f));
model13 = glm::scale(model13, glm::vec3(1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f));
model13 = glm::rotate(model13, glm::radians(270.0f), glm::vec3(1.0f, 0.0f, 0.0f));
model13 = glm::rotate(model13, glm::radians(-120.0f), glm::vec3(0.0f, 0.0f, 1.0f));
// Передаем матрицу модели в шейдер для первого объекта
GLuint modelLoc13 = glGetUniformLocation(shaderProgram, "model");
glUniformMatrix4fv(modelLoc13, 1, GL_FALSE, glm::value_ptr(model13));

// Устанавливаем цвет для первого объекта
glUniform4fv(objectColorLocation, 1, glm::value_ptr(glm::vec4(0.5f, 0.5f, 0.5f, 1.0f)));
glBindTexture(GL_TEXTURE_2D, texture3);
objModel5.draw(shaderProgram);
//кружка 1
glm::mat4 model11 = glm::mat4(1.0f);  // Инициализация единичной матрицы
model11 = glm::translate(model11, glm::vec3(150.0f*0.2f, 8.4f, 150.0f*0.2f+1.0f));
model11 = glm::scale(model11, glm::vec3(1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f));

// Передаем матрицу модели в шейдер для первого объекта
GLuint modelLoc11 = glGetUniformLocation(shaderProgram, "model");
glUniformMatrix4fv(modelLoc11, 1, GL_FALSE, glm::value_ptr(model11));

// Устанавливаем цвет для первого объекта
glUniform4fv(objectColorLocation, 1, glm::value_ptr(glm::vec4(0.5f, 0.5f, 0.5f, 1.0f)));
glBindTexture(GL_TEXTURE_2D, texture3);
objModel4.draw(shaderProgram);
//кружка 2
glm::mat4 model12 = glm::mat4(1.0f);
model12 = glm::translate(model12, glm::vec3(150.0f*0.2f, 8.4f, 150.0f*0.2f-1.0f));
model12 = glm::scale(model12, glm::vec3(1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f));
model12 = glm::rotate(model12, glm::radians(120.0f), glm::vec3(0.0f, 1.0f, 0.0f));

GLuint modelLoc12 = glGetUniformLocation(shaderProgram, "model");
glUniformMatrix4fv(modelLoc12, 1, GL_FALSE, glm::value_ptr(model12));

// Устанавливаем цвет для первого объекта
glUniform4fv(objectColorLocation, 1, glm::value_ptr(glm::vec4(0.5f, 0.5f, 0.5f, 1.0f)));
glBindTexture(GL_TEXTURE_2D, texture3);
objModel4.draw(shaderProgram);
//зонт
glm::mat4 model2 = glm::mat4(1.0f);
model2 = glm::translate(model2, glm::vec3(150.0f*0.2f, 5.0f, 150.0f*0.2f));
model2 = glm::scale(model2, glm::vec3(1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f));
model2 = glm::rotate(model2, glm::radians(270.0f), glm::vec3(1.0f, 0.0f, 0.0f));

GLuint modelLoc2 = glGetUniformLocation(shaderProgram, "model");
glUniformMatrix4fv(modelLoc2, 1, GL_FALSE, glm::value_ptr(model2));

//glUniform4fv(objectColorLocation, 1, glm::value_ptr(glm::vec4(0.6f, 0.3f, 0.0f, 1.0f)));
glBindTexture(GL_TEXTURE_2D, texture4);
objModel1.draw(shaderProgram);
//стул 1
glm::mat4 model3 = glm::mat4(1.0f);
model3 = glm::translate(model3, glm::vec3(150.0f*0.2f+1.0f, 7.0f, 150.0f*0.2f-1.5f));
model3 = glm::scale(model3, glm::vec3(1.0f*2.0f , 1.0f*2.0f , 1.0f*2.0f));
model3 = glm::rotate(model3, glm::radians(-40.0f), glm::vec3(0.0f, 1.0f, 0.0f));

GLuint modelLoc3 = glGetUniformLocation(shaderProgram, "model");
glUniformMatrix4fv(modelLoc3, 1, GL_FALSE, glm::value_ptr(model3));


//glUniform4fv(objectColorLocation, 1, glm::value_ptr(glm::vec4(0.6f, 0.3f, 0.0f, 1.0f)));
glBindTexture(GL_TEXTURE_2D, texture5);
objModel2.draw(shaderProgram);
//стул 2
glm::mat4 model4 = glm::mat4(1.0f);
model4 = glm::translate(model4, glm::vec3(150.0f*0.2f-1.0f, 7.0f, 150.0f*0.2f+1.5f));
model4 = glm::scale(model4, glm::vec3(1.0f*2.0f , 1.0f*2.0f , 1.0f*2.0f));
model4 = glm::rotate(model4, glm::radians(145.0f), glm::vec3(0.0f, 1.0f, 0.0f));
GLuint modelLoc4 = glGetUniformLocation(shaderProgram, "model");
glUniformMatrix4fv(modelLoc4, 1, GL_FALSE, glm::value_ptr(model4));
//glUniform4fv(objectColorLocation, 1, glm::value_ptr(glm::vec4(0.6f, 0.3f, 0.0f, 1.0f)));
glBindTexture(GL_TEXTURE_2D, texture5);
objModel2.draw(shaderProgram);

for (int i = 0; i < 6; ++i) {
    std::string uniformName = "lightPos[" + std::to_string(i) + "]";
    glUniform3fv(glGetUniformLocation(shaderProgram, uniformName.c_str()), 1, glm::value_ptr(lightPos[i]));
}
        glUniform4fv(objectColorLocation, 1, glm::value_ptr(glm::vec4(0.0f, 2.0f, 6.0f, 1.0f)));
        //glBindTexture(GL_TEXTURE_2D, texture5);
        glBindVertexArray(VAO_Cube);
        glm::mat4 cubeModel = glm::mat4(1.0f);
        cubeModel = glm::translate(cubeModel, glm::vec3(terrainSize * 0.2f*0.5f, cubeYOffset, terrainSize * 0.2f*0.5f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(cubeModel));
        glDrawElements(GL_TRIANGLES, cubeIndices.size(), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Clean up
    glDeleteVertexArrays(1, &VAO_Terrain);
    glDeleteBuffers(1, &VBO_Terrain);
    glDeleteBuffers(1, &EBO_Terrain);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}
