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
const float BG_RED     = 1.0f,
            BG_GREEN   = 1.0f,
            BG_BLUE    = 1.0f,
            BG_OPACITY = 1.0f;

// Viewport
const int VIEWPORT_X      = 0,
          VIEWPORT_Y      = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

// Texture constants
const int NUMBER_OF_TEXTURES = 1;
const GLint LEVEL_OF_DETAIL = 0;
const GLint TEXTURE_BORDER = 0;


// Shader filepaths
const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

// Sprites
const char  SPRITE_LEFT_PADDLE[] = "sprites/left_paddle.png",
            SPRITE_RIGHT_PADDLE[] = "sprites/right_paddle.png",
            SPRITE_BALL[] = "sprites/ball.png";


// Define objects
ShaderProgram   program_left_pad,
                program_right_pad,
                program_ball;

// Texture IDs
GLuint  texture_id_left_pad,
        texture_id_right_pad,
        texture_id_ball;

// Matrices
glm::mat4 g_view_matrix,            // Camera position
          g_projection_matrix;      // Camera characteristics

// Model matrices
glm::mat4   model_matrix_left_pad,
            model_matrix_right_pad,
            model_matrix_ball;

// Initial positions
const glm::vec3 INIT_POSITION_LEFT_PAD (-3.5f, 0.0f, 0.0f),
                INIT_POSITION_RIGHT_PAD (3.5f, 0.0f, 0.0f),
                INIT_POSITION_BALL (0.0f, 0.0f, 0.0f);

// Sizes
const glm::vec3 SIZE_PADDLE = glm::vec3(1.75f, 3.5f, 1.0f),
                SIZE_BALL = glm::vec3(0.5f, 0.5f, 0.5f);

// Tracking movement
glm::vec3   movement_left_pad,
            movement_right_pad,
            movement_ball = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3   position_left_pad,
            position_right_pad,
            position_ball = glm::vec3(0.0f, 0.0f, 0.0f);

// Speed
const float SPEED_PAD = 1.0f,
            SPEED_BALL = 1.0f,
            ROT_SPEED_BALL = 45.f;

float rot_angle = 0.0f;

// Ticks
float previous_ticks = 0.0f;

// Game const
SDL_Window* display_window;
bool game_is_running = true;


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

// RESET
void reset(glm::mat4 &model_matrix, const glm::vec3 &starting_position,
           const glm::vec3 size_vector)
{
    // Reset model matrix
    model_matrix  = glm::mat4(1.0f);
    // Set initial position
    model_matrix  = glm::translate(model_matrix, starting_position);
    // Set initial size
    model_matrix = glm::scale(model_matrix, size_vector);
}

bool is_out_of_bound(glm::vec3 &position, float height)
{
    float boundary_limit = 1.0f - 0.5f;
    if (position.y <= -boundary_limit or position.y >= boundary_limit)
    {
        return true;
    }
    else
    {
        return false;
    }
}


// INITIALISE
void initialise()
{

    SDL_Init(SDL_INIT_VIDEO);
    display_window = SDL_CreateWindow("Squirtle Headbutt Game",
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
    
    // Initialise objects
    init_objects(program_left_pad, texture_id_left_pad, SPRITE_LEFT_PADDLE,
                 model_matrix_left_pad, g_view_matrix, g_projection_matrix);

    init_objects(program_right_pad, texture_id_right_pad, SPRITE_RIGHT_PADDLE,
                  model_matrix_right_pad, g_view_matrix, g_projection_matrix);

    init_objects(program_ball, texture_id_ball, SPRITE_BALL,
                 model_matrix_ball, g_view_matrix, g_projection_matrix);
    
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
        switch (event.type)
        {
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                game_is_running = !game_is_running;
                break;
            
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                        game_is_running = !game_is_running;
                        break;
                        
                    default: break;
                }
        }
    }

    const Uint8 *key_state = SDL_GetKeyboardState(NULL);
    
    // Right paddle movement
    LOG(position_left_pad.y);
    if (is_out_of_bound(position_right_pad, SIZE_PADDLE.y) == false)
    {
        if (key_state[SDL_SCANCODE_UP])
        {
            movement_right_pad.y = 1.0f;
        }
        else if (key_state[SDL_SCANCODE_DOWN])
        {
            movement_right_pad.y = -1.0f;
        }
        if (glm::length(movement_right_pad) > 1.0f)
        {
            movement_right_pad = glm::normalize(movement_right_pad);
        }
    }
    
    // Left paddle movement
    if (is_out_of_bound(position_left_pad, SIZE_PADDLE.y) == false)
    {
        if (key_state[SDL_SCANCODE_W])
        {
            movement_left_pad.y = 1.0f;
        }
        else if (key_state[SDL_SCANCODE_S])
        {
            movement_left_pad.y = -1.0f;
        }
        if (glm::length(movement_left_pad) > 1.0f)
        {
            movement_left_pad = glm::normalize(movement_left_pad);
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
    
    // Reset model matrix
    reset(model_matrix_left_pad, INIT_POSITION_LEFT_PAD, SIZE_PADDLE);
    reset(model_matrix_right_pad, INIT_POSITION_RIGHT_PAD, SIZE_PADDLE);
    reset(model_matrix_ball, INIT_POSITION_BALL, SIZE_BALL);
    
    // Set new position
    position_left_pad += movement_left_pad * SPEED_PAD * delta_time;
    model_matrix_left_pad = glm::translate(model_matrix_left_pad, position_left_pad);
    movement_left_pad = glm::vec3(0.0f, 0.0f, 0.0f);

    position_right_pad += movement_right_pad * SPEED_PAD * delta_time;
    model_matrix_right_pad = glm::translate(model_matrix_right_pad, position_right_pad);
    movement_right_pad = glm::vec3(0.0f, 0.0f, 0.0f);
    
    // Rotate ball
    rot_angle += ROT_SPEED_BALL * delta_time;
    model_matrix_ball = glm::rotate(model_matrix_ball, glm::radians(rot_angle), glm::vec3(0.0f, 0.0f, 1.0f));
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
    draw_object(program_left_pad, model_matrix_left_pad, texture_id_left_pad,
                vertices, texture_coordinates);
    draw_object(program_right_pad, model_matrix_right_pad, texture_id_right_pad,
                vertices, texture_coordinates);
    draw_object(program_ball, model_matrix_ball, texture_id_ball,
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
