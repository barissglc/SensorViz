#include <iostream>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>

// GLAD OpenGL ES 2.0 loader
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM matematik kütüphanesi
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// M_PI tanımı (Windows uyumluluğu için)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Pencere boyutları
const unsigned int WINDOW_WIDTH = 800;
const unsigned int WINDOW_HEIGHT = 600;

// OpenGL nesneleri için global değişkenler
unsigned int shaderProgram;
unsigned int triangleVBO;
unsigned int gridVBO;
unsigned int lidarVBO;

// Lidar simülasyon değişkenleri
const int LIDAR_POINTS = 360;  // 360 derece, her derece için 1 nokta
std::vector<float> lidarVertices(LIDAR_POINTS * 2); // x,y koordinatları
float simulationTime = 0.0f;

// Kamera kontrolü değişkenleri
glm::mat4 viewMatrix = glm::mat4(1.0f);
glm::mat4 projectionMatrix = glm::mat4(1.0f);
float zoomLevel = 1.0f;
glm::vec2 panOffset(0.0f, 0.0f);
bool mousePressed = false;
glm::vec2 lastMousePos(0.0f, 0.0f);

// Shader dosyası okuma fonksiyonu
std::string readShaderFile(const std::string& filePath)
{
    std::ifstream shaderFile;
    shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    
    try
    {
        shaderFile.open(filePath);
        std::stringstream shaderStream;
        shaderStream << shaderFile.rdbuf();
        shaderFile.close();
        return shaderStream.str();
    }
    catch(std::ifstream::failure& e)
    {
        std::cerr << "Shader dosyasi okuma hatasi: " << filePath << std::endl;
        std::cerr << "Hata: " << e.what() << std::endl;
        return "";
    }
}

// Shader derleme fonksiyonu
unsigned int compileShader(unsigned int type, const std::string& source)
{
    unsigned int shader = glad_glCreateShader(type);
    const char* src = source.c_str();
    
    glad_glShaderSource(shader, 1, &src, nullptr);
    glad_glCompileShader(shader);
    
    // Derleme hatası kontrolü
    int success;
    char infoLog[512];
    glad_glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    
    if (!success)
    {
        glad_glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader derleme hatasi (" << (type == GL_VERTEX_SHADER ? "Vertex" : "Fragment") << "):" << std::endl;
        std::cerr << infoLog << std::endl;
    }
    
    return shader;
}

// Shader programı oluşturma fonksiyonu
unsigned int createShaderProgram(const std::string& vertexPath, const std::string& fragmentPath)
{
    // Shader dosyalarını oku
    std::string vertexCode = readShaderFile(vertexPath);
    std::string fragmentCode = readShaderFile(fragmentPath);
    
    if (vertexCode.empty() || fragmentCode.empty())
    {
        std::cerr << "Shader dosyalari okunamadi!" << std::endl;
        return 0;
    }
    
    // Shader'ları derle
    unsigned int vertex = compileShader(GL_VERTEX_SHADER, vertexCode);
    unsigned int fragment = compileShader(GL_FRAGMENT_SHADER, fragmentCode);
    
    // Shader programını oluştur ve linke et
    unsigned int program = glad_glCreateProgram();
    glad_glAttachShader(program, vertex);
    glad_glAttachShader(program, fragment);
    glad_glLinkProgram(program);
    
    // Link hatası kontrolü
    int success;
    char infoLog[512];
    glad_glGetProgramiv(program, GL_LINK_STATUS, &success);
    
    if (!success)
    {
        glad_glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Shader program link hatasi:" << std::endl;
        std::cerr << infoLog << std::endl;
    }
    
    // Artık ihtiyaç olmayan shader'ları sil
    glad_glDeleteShader(vertex);
    glad_glDeleteShader(fragment);
    
    return program;
}

// Vertex veriler (global) - OpenGL ES 2.0 için VAO yok
float triangleVertices[] = {
     0.0f,  0.05f,  // Üst nokta
    -0.03f, -0.05f,  // Sol alt
     0.03f, -0.05f   // Sağ alt
};

float gridVertices[] = {
    // Yatay çizgi
    -0.8f,  0.0f,
     0.8f,  0.0f,
    // Dikey çizgi  
     0.0f, -0.8f,
     0.0f,  0.8f
};

// Lidar veri simülasyon fonksiyonu
void updateLidarData(float time)
{
    for (int i = 0; i < LIDAR_POINTS; i++)
    {
        float angle = (float)i * M_PI / 180.0f; // Derece cinsinden açı
        
        // Simüle edilmiş mesafe - sinüs dalgası + gürültü
        float baseDistance = 0.3f + 0.2f * sin(angle * 3.0f + time * 2.0f);
        float noise = 0.05f * sin(angle * 7.0f + time * 5.0f);
        float distance = baseDistance + noise;
        
        // Bazı noktalarda "engel" simülasyonu
        if (sin(angle * 2.0f + time) > 0.7f)
        {
            distance *= 0.5f; // Yakın engel
        }
        
        // Polar koordinatları kartezyen'e çevir
        float x = distance * cos(angle);
        float y = distance * sin(angle);
        
        lidarVertices[i * 2] = x;     // x koordinatı
        lidarVertices[i * 2 + 1] = y; // y koordinatı
    }
}

// Kamera matrislerini güncelle
void updateCamera()
{
    // View matrix - pan offset uygula
    viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(panOffset.x, panOffset.y, 0.0f));
    
    // Projection matrix - zoom uygula
    float aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
    float size = 1.0f / zoomLevel;
    projectionMatrix = glm::ortho(-size * aspect, size * aspect, -size, size, -1.0f, 1.0f);
}

// Geometri oluşturma fonksiyonu (OpenGL ES 2.0 uyumlu)
void setupGeometry()
{
    // Üçgen VBO oluştur
    glad_glGenBuffers(1, &triangleVBO);
    glad_glBindBuffer(GL_ARRAY_BUFFER, triangleVBO);
    glad_glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVertices), triangleVertices, GL_STATIC_DRAW);
    
    // Izgara VBO oluştur
    glad_glGenBuffers(1, &gridVBO);
    glad_glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glad_glBufferData(GL_ARRAY_BUFFER, sizeof(gridVertices), gridVertices, GL_STATIC_DRAW);
    
    // Lidar VBO oluştur (dinamik)
    glad_glGenBuffers(1, &lidarVBO);
    glad_glBindBuffer(GL_ARRAY_BUFFER, lidarVBO);
    glad_glBufferData(GL_ARRAY_BUFFER, lidarVertices.size() * sizeof(float), 
                     lidarVertices.data(), GL_DYNAMIC_DRAW);
    
    // Temizlik
    glad_glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Çizim fonksiyonu (OpenGL ES 2.0 uyumlu)
void render()
{
    // Simülasyon zamanını güncelle
    simulationTime = (float)glfwGetTime();
    
    // Lidar verilerini güncelle
    updateLidarData(simulationTime);
    
    // Lidar VBO'yu güncelle
    glad_glBindBuffer(GL_ARRAY_BUFFER, lidarVBO);
    glad_glBufferSubData(GL_ARRAY_BUFFER, 0, lidarVertices.size() * sizeof(float), 
                        lidarVertices.data());
    
    // Kamera matrislerini güncelle
    updateCamera();
    
    // Arka planı temizle
    glad_glClear(GL_COLOR_BUFFER_BIT);
    
    // Shader programını kullan
    glad_glUseProgram(shaderProgram);
    
    // Uniform lokasyonlarını al
    int colorLocation = glad_glGetUniformLocation(shaderProgram, "u_color");
    int viewLocation = glad_glGetUniformLocation(shaderProgram, "u_view");
    int projLocation = glad_glGetUniformLocation(shaderProgram, "u_projection");
    
    // Matrix uniform'larını gönder
    glad_glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(viewMatrix));
    glad_glUniformMatrix4fv(projLocation, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    
    // 1. Izgara çiz (gri renk)
    glad_glUniform3f(colorLocation, 0.3f, 0.3f, 0.3f); // Koyu gri
    glad_glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glad_glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glad_glEnableVertexAttribArray(0);
    glad_glDrawArrays(GL_LINES, 0, 4); // 2 çizgi = 4 vertex
    
    // 2. Lidar noktaları çiz (yeşil renk)
    glad_glUniform3f(colorLocation, 0.2f, 1.0f, 0.2f); // Parlak yeşil
    glad_glBindBuffer(GL_ARRAY_BUFFER, lidarVBO);
    glad_glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glad_glEnableVertexAttribArray(0);
    glad_glDrawArrays(GL_POINTS, 0, LIDAR_POINTS); // 360 nokta
    
    // 3. Üçgen çiz (kırmızı renk - araç)
    glad_glUniform3f(colorLocation, 1.0f, 0.2f, 0.2f); // Kırmızı
    glad_glBindBuffer(GL_ARRAY_BUFFER, triangleVBO);
    glad_glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glad_glEnableVertexAttribArray(0);
    glad_glDrawArrays(GL_TRIANGLES, 0, 3); // 1 üçgen = 3 vertex
    
    // Temizlik
    glad_glBindBuffer(GL_ARRAY_BUFFER, 0);
    glad_glDisableVertexAttribArray(0);
}

// Hata callback fonksiyonu
void error_callback(int error, const char* description)
{
    std::cerr << "GLFW Hatasi " << error << ": " << description << std::endl;
}

// Pencere boyutu değişikliği callback'i
void framebuffer_size_callback(GLFWwindow* /*window*/, int width, int height)
{
    glad_glViewport(0, 0, width, height);
}

// Mouse scroll callback (zoom)
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    zoomLevel += (float)yoffset * 0.1f;
    zoomLevel = glm::clamp(zoomLevel, 0.1f, 5.0f); // Zoom limitleri
}

// Mouse button callback
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            mousePressed = true;
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            lastMousePos = glm::vec2((float)xpos, (float)ypos);
        }
        else if (action == GLFW_RELEASE)
        {
            mousePressed = false;
        }
    }
}

// Mouse move callback (pan)
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (mousePressed)
    {
        glm::vec2 currentPos((float)xpos, (float)ypos);
        glm::vec2 delta = currentPos - lastMousePos;
        
        // Pan sensitivity - koordinat sistemini dikkate al
        float sensitivity = 0.002f / zoomLevel;
        panOffset.x += delta.x * sensitivity;
        panOffset.y -= delta.y * sensitivity; // Y ekseni ters
        
        lastMousePos = currentPos;
    }
}

// Girdi işleme fonksiyonu
void processInput(GLFWwindow* window)
{
    // ESC tuşu ile çıkış
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }
    
    // WASD ile pan
    float panSpeed = 0.01f / zoomLevel;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        panOffset.y += panSpeed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        panOffset.y -= panSpeed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        panOffset.x -= panSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        panOffset.x += panSpeed;
        
    // Q/E ile zoom
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        zoomLevel = glm::max(0.1f, zoomLevel - 0.02f);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        zoomLevel = glm::min(5.0f, zoomLevel + 0.02f);
        
    // R ile reset
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
    {
        zoomLevel = 1.0f;
        panOffset = glm::vec2(0.0f, 0.0f);
    }
}

int main()
{
    std::cout << "=== Basit Arac Sensor Verisi Gorsellestirme Uygulamasi ===" << std::endl;
    std::cout << "OpenGL ES 2.0 + GLFW3 + GLAD" << std::endl;
    std::cout << "Cikis icin ESC tusuna basin" << std::endl << std::endl;

    // GLFW başlatma
    glfwSetErrorCallback(error_callback);
    
    if (!glfwInit())
    {
        std::cerr << "GLFW baslatilamadi!" << std::endl;
        return -1;
    }

    // OpenGL ES 2.0 context hints
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    
    // Pencere ayarları
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4); // 4x MSAA

    // Pencere oluşturma
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, 
                                         "SensorViz - Arac Sensor Gorsellestirme", 
                                         nullptr, nullptr);
    
    if (!window)
    {
        std::cerr << "GLFW penceresi olusturulamadi!" << std::endl;
        glfwTerminate();
        return -1;
    }

    // OpenGL context'ini geçerli yap
    glfwMakeContextCurrent(window);

    // GLAD ile OpenGL ES 2.0 fonksiyonlarını yükle
    if (!gladLoadGLES2Loader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "GLAD baslatilamadi!" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // OpenGL bilgilerini yazdır
    std::cout << "OpenGL Surumu: " << glad_glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Surumu: " << glad_glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Grafik Karti: " << glad_glGetString(GL_RENDERER) << std::endl;
    std::cout << "Uretici: " << glad_glGetString(GL_VENDOR) << std::endl << std::endl;

    // Viewport ayarla
    glad_glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    
    // Callback'leri ayarla
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    // VSync aktif et
    glfwSwapInterval(1);

    // OpenGL ayarları
    // GL_MULTISAMPLE OpenGL ES 2.0'da mevcut değil
    glad_glClearColor(0.1f, 0.1f, 0.2f, 1.0f); // Koyu mavi arka plan

    // Shader programını oluştur
    std::cout << "Shader'lar yukleniyor..." << std::endl;
    shaderProgram = createShaderProgram("../shaders/simple.vert", "../shaders/simple.frag");
    
    if (shaderProgram == 0)
    {
        std::cerr << "Shader programi olusturulamadi!" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    
    // Geometriyi hazırla
    std::cout << "Geometri olusturuluyor..." << std::endl;
    setupGeometry();
    
    // Attribute lokasyonunu şekildir
    glad_glUseProgram(shaderProgram);
    glad_glBindAttribLocation(shaderProgram, 0, "a_position");
    glad_glLinkProgram(shaderProgram);

    std::cout << "Uygulama baslatildi. Render dongusu basliyor..." << std::endl;
    std::cout << "\n=== KONTROLLER ===" << std::endl;
    std::cout << "Fare Tekerlegi: Zoom In/Out" << std::endl;
    std::cout << "Sol Tik + Surukle: Pan (Kaydir)" << std::endl;
    std::cout << "WASD: Klavye ile Pan" << std::endl;
    std::cout << "Q/E: Klavye ile Zoom" << std::endl;
    std::cout << "R: Kamera Reset" << std::endl;
    std::cout << "ESC: Cikis" << std::endl;
    std::cout << "\nKirmizi ucgen = Arac" << std::endl;
    std::cout << "Yesil noktalar = Lidar Sensor Verisi (Gercek Zamanli)" << std::endl;
    std::cout << "Gri cizgiler = Referans Koordinatlari" << std::endl << std::endl;

    // Ana render döngüsü
    while (!glfwWindowShouldClose(window))
    {
        // Girdi kontrolü
        processInput(window);

        // Çizim yap
        render();
        
        // Buffer'ları değiştir ve olayları işle
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    std::cout << "Uygulama kapatiliyor..." << std::endl;

    // OpenGL kaynaklarını temizle
    glad_glDeleteBuffers(1, &triangleVBO);
    glad_glDeleteBuffers(1, &gridVBO);
    glad_glDeleteBuffers(1, &lidarVBO);
    glad_glDeleteProgram(shaderProgram);

    // Temizlik
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
} 