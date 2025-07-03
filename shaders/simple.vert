// Vertex Shader - OpenGL ES 2.0
#version 100

// Giriş: vertex pozisyonu
attribute vec2 a_position;

// Kamera matrisleri
uniform mat4 u_view;
uniform mat4 u_projection;

void main()
{
    // 2D pozisyonu 4D'ye çevir ve transformation uygula
    vec4 worldPos = vec4(a_position, 0.0, 1.0);
    gl_Position = u_projection * u_view * worldPos;
    
    // Lidar noktaları için nokta boyutu
    gl_PointSize = 3.0;
} 