// Fragment Shader - OpenGL ES 2.0
#version 100

// OpenGL ES 2.0'da precision belirtmek zorunlu
precision mediump float;

// Çıkış rengi için uniform (tek renkli nesneler için)
uniform vec3 u_color;

void main()
{
    // Fragment'in rengini belirle
    gl_FragColor = vec4(u_color, 1.0);
} 