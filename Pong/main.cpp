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
            SPRITE_BALL[] = "sprites/ball.png",
            SPRITE_LINE[] = "sprites/dotted_line.png",
            SPRITE_P1[] = "sprites/player_1.png",
            SPRITE_P2[] = "sprites/player_2.png",
            SPRITE_P1_WIN[] = "sprites/p1_win.png",
            SPRITE_P2_WIN[] = "sprites/p2_win.png";


// Define objects
ShaderProgram   program_left_pad, program_right_pad,
                program_ball, program_line,
                program_p1, program_p2,
                program_p1_win, program_p2_win;

// Texture IDs
GLuint  texture_id_left_pad, texture_id_right_pad,
        texture_id_ball, texture_id_line,
        texture_id_p1, texture_id_p2,
        texture_id_p1_win, texture_id_p2_win;

// Matrices
glm::mat4 g_view_matrix,            // Camera position
          g_projection_matrix;      // Camera characteristics

// Model matrices
glm::mat4   model_matrix_left_pad, model_matrix_right_pad,
            model_matrix_ball, model_matrix_line,
            model_matrix_p1, model_matrix_p2,
            model_matrix_p1_win, model_matrix_p2_win;

// Initial positions
const glm::vec3 INIT_POSITION_LEFT_PAD (-3.5f, 0.0f, 0.0f),
                INIT_POSITION_RIGHT_PAD (3.5f, 0.0f, 0.0f),
                INIT_POSITION_BALL (0.0f, 0.0f, 0.0f),
                INIT_POSITION_LINE (0.0f, 0.0f, 0.0f),
                INIT_POSITION_P1 (-2.5f, 3.0f, 0.0f),
                INIT_POSITION_P2 (2.5f, 3.0f, 0.0f),
                INIT_POSITION_P1_WIN (0.0f, 0.0f, 0.0f),
                INIT_POSITION_P2_WIN (0.0f, 0.0f, 0.0f);

// Sizes
const glm::vec3 SIZE_PADDLE = glm::vec3(1.75f, 3.5f, 1.0f),
                SIZE_BALL = glm::vec3(0.5f, 0.5f, 0.5f),
                SIZE_LINE = glm::vec3(1.0f, 1.0f, 1.0f),
                SIZE_PLAYER = glm::vec3(2.0f, 1.0f, 1.0f),
                SIZE_WIN = glm::vec3(3.0f, 3.0f, 1.0f);

// Whether object is moving
glm::vec3   movement_left_pad,
            movement_right_pad = glm::vec3(0.0f, 0.0f, 0.0f);
// Ball is initialized to move to the left corner
glm::vec3   movement_ball = glm::vec3(-1.0f, -0.5f, 0.0f);

// How much object has moved
glm::vec3   position_left_pad,
            position_right_pad,
            position_ball = glm::vec3(0.0f, 0.0f, 0.0f);

// Speed
const float SPEED_PAD = 3.0f,
            SPEED_BALL = 2.5f,
            ROT_SPEED_BALL = 45.f;

float rot_angle = 0.0f;

// Ticks
float previous_ticks = 0.0f;

// Game const
SDL_Window* display_window;
bool game_is_running = true;
bool end_game = false;
int winner;

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
                  glm::mat4 &view_matrix, glm::mat4 &projection_matrix,
                  const glm::vec3 init_position, glm::vec3 size_vector)
{
    // Load up shaders
    program.Load(V_SHADER_PATH, F_SHADER_PATH);
    
    // Initialize model matrix
    model_matrix = glm::mat4(1.0f);
    // Translate to initial position
    model_matrix  = glm::translate(model_matrix, init_position);
    // Scale to initial size
    model_matrix  = glm::scale(model_matrix, size_vector);
    
    // Load texture
    texture_id = load_texture(sprite);

    // Set matrices
    program.SetProjectionMatrix(projection_matrix);
    program.SetViewMatrix(view_matrix);

    // Object ID
    glUseProgram(program.programID);
}

// RESET
void reset(glm::mat4 &model_matrix, const glm::vec3 init_position)
{
    // Reset model matrix
    model_matrix  = glm::mat4(1.0f);
    // Translate to initial position
    model_matrix  = glm::translate(model_matrix, init_position);
}

// Checks whether objects hit the upper and lower walls
bool is_out_of_bound(const glm::vec3 &init_position, glm::vec3 &position,
                     const glm::vec3 scale_vector)
{
    float window_height = 3.75f;
    float boundary_limit = window_height - 0.5f * scale_vector.y;
    glm::vec3 curr_position = init_position + position;
    if (curr_position.y <= -boundary_limit or curr_position.y >= boundary_limit)
    { return true; }
    else { return false; }
}

// Checks whether ball hits left or right wall
bool ball_hits_vertical_wall(const glm::vec3 &init_position, glm::vec3 &position)
{
    float window_width = 5.0f;
    float boundary_limit = window_width - 0.5f * SIZE_BALL.x;
    glm::vec3 curr_position = init_position + position;
    if (curr_position.x <= -boundary_limit or curr_position.x >= boundary_limit)
    { return true; }
    else { return false; }
}

// Move paddles according to user input and takes care of out-of-bound stuff
void user_move_object(const glm::vec3 &init_position, glm::vec3 &position,
                 const glm::vec3 scale_vector, glm::vec3 &movement,
                 const float speed, glm::mat4 &model_matrix, float delta_time)
{
    // Set new position
    position += movement * speed * delta_time;
    // If out-of-bound, do not let add to position
    if (is_out_of_bound(init_position, position, scale_vector))
    {
        position -= movement * speed * delta_time;
    }
    // Translate to new position
    model_matrix = glm::translate(model_matrix, position);
    // Reset movement vector
    movement = glm::vec3(0.0f, 0.0f, 0.0f);
    
}

// Detects collision
bool collided(glm::vec3 &position_a, glm::vec3 &position_b,
              const glm::vec3 &init_position_a, const glm::vec3 &init_position_b,
              const glm::vec3 &size_vec_a, const glm::vec3 &size_vec_b)
{
    // Collision factor
    float collision_factor = 1.0f;
    // Get current position
    glm::vec3 curr_position_a = init_position_a + position_a;
    glm::vec3 curr_position_b = init_position_b + position_b;
    // Get x_ and y_distances
    float x_distance = fabs(curr_position_a.x - curr_position_b.x) - (collision_factor * (size_vec_a.x + size_vec_b.x) / 2.0f);
    float y_distance = fabs(curr_position_a.y - curr_position_b.y) - (collision_factor * (size_vec_a.y + size_vec_b.y) / 2.0f);

    if (x_distance < 0 && y_distance < 0)
    { return true; }
    else { return false; }
}


// INITIALISE
void initialise()
{

    SDL_Init(SDL_INIT_VIDEO);
    display_window = SDL_CreateWindow("Pokepong",
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
                 model_matrix_left_pad, g_view_matrix, g_projection_matrix,
                 INIT_POSITION_LEFT_PAD, SIZE_PADDLE);

    init_objects(program_right_pad, texture_id_right_pad, SPRITE_RIGHT_PADDLE,
                 model_matrix_right_pad, g_view_matrix, g_projection_matrix,
                 INIT_POSITION_RIGHT_PAD, SIZE_PADDLE);

    init_objects(program_ball, texture_id_ball, SPRITE_BALL,
                 model_matrix_ball, g_view_matrix, g_projection_matrix,
                 INIT_POSITION_BALL, SIZE_BALL);
    
    init_objects(program_line, texture_id_line, SPRITE_LINE,
                 model_matrix_line, g_view_matrix, g_projection_matrix,
                 INIT_POSITION_LINE, SIZE_LINE);
    
    init_objects(program_p1, texture_id_p1, SPRITE_P1,
                 model_matrix_p1, g_view_matrix, g_projection_matrix,
                 INIT_POSITION_P1, SIZE_PLAYER);
    
    init_objects(program_p2, texture_id_p2, SPRITE_P2,
                 model_matrix_p2, g_view_matrix, g_projection_matrix,
                 INIT_POSITION_P2, SIZE_PLAYER);
    
    init_objects(program_p1_win, texture_id_p1_win, SPRITE_P1_WIN,
                 model_matrix_p1_win, g_view_matrix, g_projection_matrix,
                 INIT_POSITION_P1_WIN, SIZE_WIN);
    
    init_objects(program_p2_win, texture_id_p2_win, SPRITE_P2_WIN,
                 model_matrix_p2_win, g_view_matrix, g_projection_matrix,
                 INIT_POSITION_P2_WIN, SIZE_WIN);
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
    
    // Left paddle movement
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

// UPDATE
void update()
{
    // Counting ticks
    float ticks = (float) SDL_GetTicks() / 1000.0f;
    float delta_time = ticks - previous_ticks;
    previous_ticks = ticks;
    
    // Reset model matrix
    reset(model_matrix_left_pad, INIT_POSITION_LEFT_PAD);
    reset(model_matrix_right_pad, INIT_POSITION_RIGHT_PAD);
    reset(model_matrix_ball, INIT_POSITION_BALL);
    
    // Paddles movement according to user input
    user_move_object(INIT_POSITION_LEFT_PAD, position_left_pad, SIZE_PADDLE, movement_left_pad,
                SPEED_PAD, model_matrix_left_pad, delta_time);
    user_move_object(INIT_POSITION_RIGHT_PAD, position_right_pad, SIZE_PADDLE, movement_right_pad,
                SPEED_PAD, model_matrix_right_pad, delta_time);
    
    // Ball movement
    // Set new position
    position_ball += movement_ball * SPEED_BALL * delta_time;
    // If ball hits vertical wall, end game
    if (ball_hits_vertical_wall(INIT_POSITION_BALL, position_ball))
    {
        LOG("SCORE!");
        end_game = true;
        winner = (INIT_POSITION_BALL + position_ball).x < 0 ? 2 : 1;
    }
    // If ball collides with any paddle
    else
    {
        if (collided(position_ball, position_left_pad, INIT_POSITION_BALL,
                      INIT_POSITION_LEFT_PAD, SIZE_BALL, SIZE_PADDLE) or
             collided(position_ball, position_right_pad, INIT_POSITION_BALL,
                      INIT_POSITION_RIGHT_PAD, SIZE_BALL, SIZE_PADDLE))
        {
            position_ball -= movement_ball * SPEED_BALL * delta_time;
            movement_ball = glm::vec3(-movement_ball.x, movement_ball.y, 0.0f);
            position_ball += movement_ball * SPEED_BALL * delta_time;
        }
        else if (is_out_of_bound(INIT_POSITION_BALL, position_ball, SIZE_BALL))
        {
            position_ball -= movement_ball * SPEED_BALL * delta_time;
            movement_ball = glm::vec3(movement_ball.x, -movement_ball.y, 0.0f);
            position_ball += movement_ball * SPEED_BALL * delta_time;
        }

        model_matrix_ball = glm::translate(model_matrix_ball, position_ball);
    }

    // Rotate ball
    rot_angle += ROT_SPEED_BALL * delta_time;
    model_matrix_ball = glm::rotate(model_matrix_ball, glm::radians(rot_angle), glm::vec3(0.0f, 0.0f, 1.0f));
    
    // Scale objects back
    model_matrix_left_pad = glm::scale(model_matrix_left_pad, SIZE_PADDLE);
    model_matrix_right_pad = glm::scale(model_matrix_right_pad, SIZE_PADDLE);
    model_matrix_ball = glm::scale(model_matrix_ball, SIZE_BALL);
}


// RENDER
void render()
    {
    glClear(GL_COLOR_BUFFER_BIT);
    // Show the winner
    if (end_game)
    {
        if (winner == 1)
        {
            // Player 1 wins
            float vertices_p1_win[] = {
                -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
                -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f
            };
            float texture_coordinates_p1_win[] = {
                0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
                0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
            };
            draw_object(program_p1_win, model_matrix_p1_win, texture_id_p1_win,
                        vertices_p1_win, texture_coordinates_p1_win);
        }
        else
        {
            // Player 2 wins
            float vertices_p2_win[] = {
                -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
                -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f
            };
            float texture_coordinates_p2_win[] = {
                0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
                0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
            };
            draw_object(program_p2_win, model_matrix_p2_win, texture_id_p2_win,
                        vertices_p2_win, texture_coordinates_p2_win);
        }
    }
    // Line
    float vertices_line[] = {
        -0.05f, -3.75f, 0.05f, -3.75f, 0.05f, 3.75f,
        -0.05f, -3.75f, 0.05f, 3.75f, -0.05f, 3.75f
    };
    float texture_coordinates_line[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    };
    draw_object(program_line, model_matrix_line, texture_id_line,
                vertices_line, texture_coordinates_line);
    
    // Player 1
    float vertices_p1[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f
    };
    float texture_coordinates_p1[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    };
    draw_object(program_p1, model_matrix_p1, texture_id_p1,
                vertices_p1, texture_coordinates_p1);
    
    // Player 2
    float vertices_p2[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f
    };
    float texture_coordinates_p2[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    };
    draw_object(program_p2, model_matrix_p2, texture_id_p2,
                vertices_p2, texture_coordinates_p2);
    
    // Left paddle
    float vertices_left_pad[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f
    };
    float texture_coordinates_left_pad[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    };
    draw_object(program_left_pad, model_matrix_left_pad, texture_id_left_pad,
                vertices_left_pad, texture_coordinates_left_pad);
    
    // Right paddle
    float vertices_right_pad[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f
    };
    float texture_coordinates_right_pad[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    };
    draw_object(program_right_pad, model_matrix_right_pad, texture_id_right_pad,
                vertices_right_pad, texture_coordinates_right_pad);
    
    // Ball
    float vertices_right_ball[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f
    };
    float texture_coordinates_ball[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    };
    draw_object(program_ball, model_matrix_ball, texture_id_ball,
                vertices_right_ball, texture_coordinates_ball);
    
    SDL_GL_SwapWindow(display_window);
}


// SHUTDOWN
void shutdown() { SDL_Quit(); }


int main(int argc, char* argv[])
{
    initialise();
    
    while (game_is_running)
    {
        LOG(end_game);
        process_input();
        if (end_game == false) { update(); }
        render();
    }
    
    shutdown();
    return 0;
}
