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
const glm::vec3 APPLE_INIT_POSITION (-3.0f, 3.0f, 0.0f),
                BOY_INIT_POSITION (-17.2f, -1.5f, 0.0f),
                SUN_INIT_POSITION (3.5f, 2.5f, 0.0f);

// Sizes
const float APPLE_SIZE = 0.75f,
            BOY_SIZE = 3.5f,
            SUN_SIZE = 1.5f;

// Keeping track of apple's and basket's current position
float   apple_x = APPLE_INIT_POSITION.x,  // center of apple
        apple_y = APPLE_INIT_POSITION.y,  // center of apple
        basket_y_top = BOY_INIT_POSITION.y + BOY_SIZE * 0.5f,
        basket_y_bottom = BOY_INIT_POSITION.y + BOY_SIZE * 0.375f;


// Const for scaling for sun
const float SCALE_FACTOR = 0.25f;
float sun_scale = 1.0f;

// Rotation and translation const
const float ROT_ANGLE = 90.0f;
const float TRAN_VALUE = 2.0f;
float  angle = 0.0f,
       boy_trans_x = 0.0f;

// Ticks
float previous_ticks = 0.0f;

// Game const
SDL_Window* display_window;
bool game_is_running = true;


// LOAD_TEXTURE
GLuint load_texture(const char* filepath)
{
    // Load image file
    int width, height, number_of_components;
    
    unsigned char* image = stbi_load(filepath, &width, &height,
                                     &number_of_components, STBI_rgb_alpha);
    
    // If load fails, quit game
    if (image == NULL)
    {
        LOG("Unable to load image.");
        assert(false);
    }
    
    // Generate and bind texture ID to game
    GLuint texture_id;
    
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, image);
    
    // Set texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    // Release file from memory
    stbi_image_free(image);
    
    return texture_id;
}

// DRAW_OBJECT
void draw_object(ShaderProgram &program, glm::mat4 &object_model_matrix,
                 GLuint &object_texture_id, float* vertices,
                 float* texture_coordinates)
{
    
    // Vertices
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program.positionAttribute);

    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(program.texCoordAttribute);
    
    // Bind texture
    program.SetModelMatrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    // Disable attribute arrays
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);

}

// INITIALISE OBJECTS
void init_objects(ShaderProgram &program, GLuint &texture_id,
                  const char* sprite, glm::mat4 &model_matrix,
                  glm::mat4 &view_matrix, glm::mat4 &projection_matrix,
                  const glm::vec3 &starting_position, const float size)
{
    // Load up shaders
    program.Load(V_SHADER_PATH, F_SHADER_PATH);
    
    // Load texture
    texture_id = load_texture(sprite);
    
    // Initialize model matrix and set initial position and size
    model_matrix  = glm::mat4(1.0f);
    model_matrix  = glm::translate(model_matrix, starting_position);
    model_matrix = glm::scale(model_matrix, glm::vec3(size, size, 0.0f));

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
    
    // Initialise view, model, and projection matrices
    g_view_matrix       = glm::mat4(1.0f);
    
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    init_objects(program_apple, texture_id_apple, SPRITE_APPLE,
                 model_matrix_apple, g_view_matrix, g_projection_matrix,
                 APPLE_INIT_POSITION, APPLE_SIZE);
    
    init_objects(program_boy, texture_id_boy, SPRITE_BOY,
                 model_matrix_boy, g_view_matrix, g_projection_matrix,
                 BOY_INIT_POSITION, BOY_SIZE);
    
    init_objects(program_sun, texture_id_sun, SPRITE_SUN,
                 model_matrix_sun, g_view_matrix, g_projection_matrix,
                 SUN_INIT_POSITION, SUN_SIZE);
    
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
    if (int(floor(ticks)) % 2 == 0) {
        sun_scale += SCALE_FACTOR * delta_time;
        boy_trans_x += TRAN_VALUE * delta_time;
        apple_x += TRAN_VALUE * delta_time;
    } else {
        sun_scale -= SCALE_FACTOR * delta_time;
        boy_trans_x -= TRAN_VALUE * delta_time;
        apple_x -= TRAN_VALUE * delta_time;
    }
    angle += ROT_ANGLE * delta_time;
    apple_y -= TRAN_VALUE * delta_time;
    
    float   basket_x_right = BOY_INIT_POSITION.x + boy_trans_x,
            basket_x_left = BOY_INIT_POSITION.x + boy_trans_x - BOY_SIZE * 0.5f;


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
    glm::vec3 trans_vector_boy = glm::vec3(boy_trans_x, 0.0f, 0.0f);
    model_matrix_boy = glm::translate(model_matrix_boy, trans_vector_boy);
    
    // If boy catches apple or apple falls out of bound, apple respawn
    if (apple_y <= -8.0f)
    {
        apple_y = APPLE_INIT_POSITION.y;
    }
    
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
