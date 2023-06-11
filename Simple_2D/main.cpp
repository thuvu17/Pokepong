/**
* Author: Thu Vu
* Assignment: Simple 2D Scene
* Date due: 2023-06-11, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define LOG(argument) std::cout << argument << '\n'
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include <stdlib.h>


// Window dimensions
const int WINDOW_WIDTH  = 640,
          WINDOW_HEIGHT = 480;

// Background color
const float BG_RED     = 0.410f,
            BG_GREEN   = 0.686f,
            BG_BLUE    = 0.985f,
            BG_OPACITY = 1.0f;

// Viewport
const int VIEWPORT_X      = 0,
          VIEWPORT_Y      = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

// Shader filepaths
const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

// Sprites
const char SPRITE_APPLE[] = "sprites/apple.png";
const char SPRITE_BOY[]   = "sprites/basket_boy.png";
const char SPRITE_SUN[]   = "sprites/sun.png";


// Define objects
ShaderProgram program_apple;
ShaderProgram program_boy;
ShaderProgram program_sun;

// Texture IDs
GLuint texture_id_apple;
GLuint texture_id_boy;
GLuint texture_id_sun;

// Matrices
glm::mat4 g_view_matrix,            // Camera position
          model_matrix_apple,       // Apple's model matrix
          model_matrix_boy,         // Boy's model matrix
          model_matrix_sun,         // Sun's model matrix
          g_projection_matrix;      // Camera characteristics

// Initial positions
const glm::vec3 APPLE_INIT_POSITION (0.0f, 3.0f, 0.0f),
                BOY_INIT_POSITION (0.0f, -1.5f, 0.0f),
                SUN_INIT_POSITION (3.5f, 2.5f, 0.0f);

// Sizes
const float APPLE_SIZE = 0.75f,
            BOY_SIZE = 3.5f,
            SUN_SIZE = 1.5f;

// Current position
float   apple_x = APPLE_INIT_POSITION.x,
        apple_y = APPLE_INIT_POSITION.y,
        boy_x = BOY_INIT_POSITION.x;


// Const for scaling for sun
const float SCALE_FACTOR = 0.25f;
float sun_scale = 1.0f;

// Rotation and translation const
const float ROT_ANGLE = 90.0f;
const float TRAN_VALUE = 1.0f;
float angle = 0.0f;
bool moving_right = true;

// Ticks
float previous_ticks = 0.0f;

// Game const
SDL_Window* display_window;
bool game_is_running = true;

// Load texture const
const int NUMBER_OF_TEXTURES = 1;
const GLint LEVEL_OF_DETAIL = 0;
const GLint TEXTURE_BORDER = 0;


// LOAD TEXTURE
GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);
    
    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }
    
    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);
    
    return textureID;
}

// DRAW_OBJECT
void draw_object(ShaderProgram &program, glm::mat4 &model_matrix,
                 GLuint &texture_id, float* vertices,
                 float* texture_coordinates)
{
    
    // Vertices
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program.positionAttribute);

    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(program.texCoordAttribute);
    
    // Bind texture
    program.SetModelMatrix(model_matrix);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    // Disable attribute arrays
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);

}

// INITIALISE OBJECTS
void init_objects(ShaderProgram &program, GLuint &texture_id,
                  const char* sprite, glm::mat4 &model_matrix, 
                  glm::mat4 &view_matrix, glm::mat4 &projection_matrix)
{
    // Load up shaders
    program.Load(V_SHADER_PATH, F_SHADER_PATH);
    
    // Initialize model matrix
    model_matrix = glm::mat4(1.0f);
    
    // Load texture
    texture_id = load_texture(sprite);

    // Set matrices
    program.SetProjectionMatrix(projection_matrix);
    program.SetViewMatrix(view_matrix);

    // Object ID
    glUseProgram(program.programID);
}

void reset(glm::mat4 &model_matrix, const glm::vec3 &starting_position,
           const float size)
{
    // Reset model matrix
    model_matrix  = glm::mat4(1.0f);
    // Set initial position
    model_matrix  = glm::translate(model_matrix, starting_position);
    // Set initial size
    model_matrix = glm::scale(model_matrix, glm::vec3(size, size, 0.0f));
}


// INITIALISE
void initialise()
{

    SDL_Init(SDL_INIT_VIDEO);
    display_window = SDL_CreateWindow("Simple 2D Scene",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(display_window);
    SDL_GL_MakeCurrent(display_window, context);
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    // Initialise camera
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    // Initialise view and projection matrices
    g_view_matrix       = glm::mat4(1.0f);
    
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    

    init_objects(program_apple, texture_id_apple, SPRITE_APPLE,
                 model_matrix_apple, g_view_matrix, g_projection_matrix);

    init_objects(program_boy, texture_id_boy, SPRITE_BOY,
                 model_matrix_boy, g_view_matrix, g_projection_matrix);

    init_objects(program_sun, texture_id_sun, SPRITE_SUN,
                 model_matrix_sun, g_view_matrix, g_projection_matrix);
    
    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(BG_RED, BG_GREEN, BG_BLUE, BG_OPACITY);
}


// PROCESS INPUT
void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE)
        {
            game_is_running = false;
        }
    }
}

// UPDATE
void update()
{
    // Counting ticks
    float ticks = (float) SDL_GetTicks() / 1000.0f;
    float delta_time = ticks - previous_ticks;
    previous_ticks = ticks;


    // Updating motion values
    if (moving_right == true) {
        sun_scale += SCALE_FACTOR * delta_time;
        boy_x += TRAN_VALUE * delta_time;
        apple_x -= TRAN_VALUE * delta_time;
    } else {
        sun_scale -= SCALE_FACTOR * delta_time;
        boy_x -= TRAN_VALUE * delta_time;
        apple_x += TRAN_VALUE * delta_time;
    }
    angle += ROT_ANGLE * delta_time;
    apple_y -= TRAN_VALUE * delta_time;
    
    // Out-of-bound conditions
    if (boy_x < -0.5f) {
        boy_x = -0.5f;
        moving_right = true;
    } else if (boy_x > 0.5f) {
        boy_x = 0.5f;
        moving_right = false; }
    
    // If apple falls out of bound, apple respawn
    if (apple_y <= -8.0f)
    {
        apple_x = rand() % 4 - 2;
        apple_y = APPLE_INIT_POSITION.y;
    }

    // Reset model matrices
    reset(model_matrix_apple, APPLE_INIT_POSITION, APPLE_SIZE);
    reset(model_matrix_boy, BOY_INIT_POSITION, BOY_SIZE);
    reset(model_matrix_sun, SUN_INIT_POSITION, SUN_SIZE);
    
    
    // Sun is rotating and scaling
    glm::vec3 rot_vector    = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 scale_vector  = glm::vec3(sun_scale, sun_scale, 0.0f);
    model_matrix_sun = glm::rotate(model_matrix_sun, glm::radians(angle), rot_vector);
    model_matrix_sun = glm::scale(model_matrix_sun, scale_vector);
    
    // Apple is falling down left and right
    glm::vec3 trans_vector_apple = glm::vec3(apple_x, apple_y, 0.0f);
    model_matrix_apple = glm::translate(model_matrix_apple, trans_vector_apple);

    // Boy is moving left and right
    glm::vec3 trans_vector_boy = glm::vec3(boy_x, 0.0f, 0.0f);
    model_matrix_boy = glm::translate(model_matrix_boy, trans_vector_boy);
    
}


// RENDER
void render()
    {
    
    glClear(GL_COLOR_BUFFER_BIT);
    
    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f
    };
    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    };
    

    // Draw objects
    draw_object(program_apple, model_matrix_apple, texture_id_apple,
                vertices, texture_coordinates);
    draw_object(program_boy, model_matrix_boy, texture_id_boy,
                vertices, texture_coordinates);
    draw_object(program_sun, model_matrix_sun, texture_id_sun,
                vertices, texture_coordinates);
    
    SDL_GL_SwapWindow(display_window);
}


// SHUTDOWN
void shutdown() { SDL_Quit(); }


int main(int argc, char* argv[])
{
    initialise();
    
    while (game_is_running)
    {
        process_input();
        update();
        render();
    }
    
    shutdown();
    return 0;
}
