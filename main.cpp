/**
* Author: Jason Lin
* Assignment: Pong Clone
* Date due: 2025-3-01, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>

enum AppStatus { RUNNING, TERMINATED };

constexpr float WINDOW_SIZE_MULT = 1.0f;

constexpr int WINDOW_WIDTH = 640 * WINDOW_SIZE_MULT,
WINDOW_HEIGHT = 480 * WINDOW_SIZE_MULT;

constexpr float BG_RED = 0.25f,
BG_GREEN = 0.25f,
BG_BLUE = 0.25f,
BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

int GAME_STATE = 0;

constexpr float SCALE_Y = 1.5f,
                SCALE_X = SCALE_Y,
                INIT_X = 5.0f - (SCALE_X / 2);

constexpr float UPPER_BOUND = 3.5f,
                LOWER_BOUND = -UPPER_BOUND,
                WALL = 4.0f;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;
bool SINGLE_PLAYER = false; // set default player mode to 2

constexpr char LEFT_SPRITE_FILEPATH[] = "assets/cat_left.png",
               RIGHT_SPRITE_FILEPATH[] = "assets/cat_right.png",
               BALL_SPRITE_FILEPATH[] = "assets/image.png",
               P1_SPRITE_FILEPATH[] = "assets/p1_win.png",
               P2_SPRITE_FILEPATH[] = "assets/p2_win.png";

constexpr float MINIMUM_COLLISION_DISTANCE = 1.0f;

constexpr glm::vec3 INIT_SCALE_LEFT = glm::vec3(SCALE_X, SCALE_Y, 1.0f),
                    INIT_SCALE_RIGHT = glm::vec3(SCALE_X, SCALE_Y, 1.0f),
                    INIT_SCALE_BALL = glm::vec3(0.5f, 0.5f, 1.0f),
                    INIT_SCALE_WIN = glm::vec3(8.0f, 4.0f, 1.0f);

constexpr glm::vec3 INIT_POS_LEFT = glm::vec3(-INIT_X, 0.0f, 1.0f),
                    INIT_POS_RIGHT = glm::vec3(INIT_X, 0.0f, 1.0f),
                    INIT_POS_BALL = glm::vec3(0.0f, 0.0f, 0.0f),
                    INIT_POS_WIN = glm::vec3(0.0f, 0.0f, 0.0f);


SDL_Window* g_display_window;

AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();
glm::mat4 g_view_matrix, g_projection_matrix, g_left_matrix, g_right_matrix, g_ball_matrix, g_win_matrix;

float g_previous_ticks = 0.0f;

GLuint g_left_texture_id,
       g_right_texture_id,
       g_ball_texture_id,
       g_p1_texture_id,
       g_p2_texture_id;

glm::vec3 g_left_position = glm::vec3(0.0f, 0.0f, 0.0f),
          g_right_position = glm::vec3(0.0f, 0.0f, 0.0f),
          g_ball_position = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_left_movement = glm::vec3(0.0f, 0.0f, 0.0f),
          g_right_movement = glm::vec3(0.0f, 0.0f, 0.0f),
          g_ball_movement = glm::vec3(0.0f, 0.0f, 0.0f);

float g_ball_speed = 1.0f;  // move 1 unit per second
float g_paddle_speed = 2.5f;

void initialise();
void process_input();
void update();
void render();
void shutdown();

// declarations cause c++
void restart();
void coord(glm::vec3&);

constexpr GLint NUMBER_OF_TEXTURES = 1;  // to be generated, that is
constexpr GLint LEVEL_OF_DETAIL = 0;  // base image level; Level n is the nth mipmap reduction image
constexpr GLint TEXTURE_BORDER = 0;  // this value MUST be zero

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

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("It's Another Pong Clone!",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);


    if (g_display_window == nullptr)
    {
        shutdown();
    }
#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_left_matrix = glm::mat4(1.0f);
    g_right_matrix = glm::mat4(1.0f);
    g_ball_matrix = glm::mat4(1.0f);
    g_win_matrix = glm::mat4(1.0f);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_GREEN, BG_BLUE, BG_OPACITY);

    g_left_texture_id = load_texture(LEFT_SPRITE_FILEPATH);
    g_right_texture_id = load_texture(RIGHT_SPRITE_FILEPATH);
    g_ball_texture_id = load_texture(BALL_SPRITE_FILEPATH);
    g_p1_texture_id = load_texture(P1_SPRITE_FILEPATH);
    g_p2_texture_id = load_texture(P2_SPRITE_FILEPATH);


    // initial ball movement
    srand(std::time(nullptr)); // seed with random to ensure randomness
    restart();

    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    // VERY IMPORTANT: If nothing is pressed, we don't want to go anywhere
    g_left_movement = glm::vec3(0.0f);
    g_right_movement = glm::vec3(0.0f);

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            // End game
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_app_status = TERMINATED;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym)
            {
            case SDLK_q:
                // Quit the game with a keystroke
                g_app_status = TERMINATED;
                break;
            case SDLK_t:
                // Toggle between modes
                SINGLE_PLAYER = !SINGLE_PLAYER;
                break;
            case SDLK_r:
                restart();
                break;

            // extra stuff for testing
            case SDLK_b:
                g_ball_movement = -g_ball_movement;
                break;
            // FASTER
            case SDLK_f:
                g_ball_speed += 1.0f;
            default:
                break;
            }

        default:
            break;
        }
    }


    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    if (GAME_STATE == 0)
    {
        if (key_state[SDL_SCANCODE_W])
        {
            g_left_movement.y += 1.0f;
        }
        if (key_state[SDL_SCANCODE_S])
        {
            g_left_movement.y += -1.0f;
        }

        if (!SINGLE_PLAYER)
        {
            if (key_state[SDL_SCANCODE_UP])
            {
                g_right_movement.y += 1.0f;
            }
            if (key_state[SDL_SCANCODE_DOWN])
            {
                g_right_movement.y += -1.0f;
            }
        }
    }

    // This makes sure that the player can't "cheat" their way into moving
    // faster
    if (glm::length(g_left_movement) > 1.0f)
    {
        g_left_movement = glm::normalize(g_left_movement);
    }
    if (glm::length(g_right_movement) > 1.0f)
    {
        g_right_movement = glm::normalize(g_right_movement);
    }
}


// function to restart so i can call call it multiple times
void restart() {
    g_ball_position = glm::vec3(0.0f, 0.0f, 0.0f);
    g_ball_movement.x = float(rand() % 101) - 50;
    g_ball_movement.y = float(rand() % 101) - 50;
    g_ball_movement = glm::normalize(g_ball_movement);
    GAME_STATE = 0;
    std::cout << "g_ball_movement ";
    coord(g_ball_movement);
}

void coord(glm::vec3& values) {
    std::cout << "X: " << values.x << " Y: " << values.y << std::endl;
}

void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND; // get the current number of ticks
    float delta_time = ticks - g_previous_ticks; // the delta time is the difference from the last frame
    g_previous_ticks = ticks;

    if (GAME_STATE == 0)
    {    
        g_ball_position += g_ball_movement * g_ball_speed * delta_time;

        if (g_ball_position.y > UPPER_BOUND)
        {
            g_ball_position.y = UPPER_BOUND;
            g_ball_movement.y = -g_ball_movement.y;
        }
        if (g_ball_position.y < LOWER_BOUND)
        {
            g_ball_position.y = LOWER_BOUND;
            g_ball_movement.y = -g_ball_movement.y;
        }

        g_ball_matrix = glm::mat4(1.0f);
        g_ball_matrix = glm::translate(g_ball_matrix, INIT_POS_BALL);
        g_ball_matrix = glm::translate(g_ball_matrix, g_ball_position);
        g_ball_matrix = glm::scale(g_ball_matrix, INIT_SCALE_BALL);


        // Add direction * units per second * elapsed time
        g_left_position += g_left_movement * g_paddle_speed * delta_time;

        if (g_left_position.y > UPPER_BOUND) {
            g_left_position.y = UPPER_BOUND;
        }
        if (g_left_position.y < LOWER_BOUND) {
            g_left_position.y = LOWER_BOUND;
        }

        g_left_matrix = glm::mat4(1.0f);
        g_left_matrix = glm::translate(g_left_matrix, INIT_POS_LEFT);
        g_left_matrix = glm::translate(g_left_matrix, g_left_position);
        g_left_matrix = glm::scale(g_left_matrix, INIT_SCALE_LEFT);

        if (SINGLE_PLAYER) {
            g_right_movement.y = ((g_ball_position.y > g_right_position.y) ? 1.0f : -1.0f);
        }

        g_right_position += g_right_movement * g_paddle_speed * delta_time;

        if (g_right_position.y > UPPER_BOUND) {
            g_right_position.y = UPPER_BOUND;
        }
        if (g_right_position.y < LOWER_BOUND) {
            g_right_position.y = LOWER_BOUND;
        }

        g_right_matrix = glm::mat4(1.0f);
        g_right_matrix = glm::translate(g_right_matrix, INIT_POS_RIGHT);
        g_right_matrix = glm::translate(g_right_matrix, g_right_position);
        g_right_matrix = glm::scale(g_right_matrix, INIT_SCALE_RIGHT);

        float ball_x = g_ball_position.x + INIT_POS_BALL.x;
        float ball_y = g_ball_position.y + INIT_POS_BALL.y;

        float left_x = g_left_position.x + INIT_POS_LEFT.x;
        float left_y = g_left_position.y + INIT_POS_LEFT.y;

        float right_x = g_right_position.x + INIT_POS_RIGHT.x;
        float right_y = g_right_position.y + INIT_POS_RIGHT.y;

        float x_dist_left = fabs(ball_x - left_x) - ((INIT_SCALE_BALL.x + INIT_SCALE_LEFT.x) / 2.0f);
        float y_dist_left = fabs(ball_y - left_y) - ((INIT_SCALE_BALL.y + INIT_SCALE_LEFT.y) / 2.0f);

        float x_dist_right = fabs(ball_x - right_x) - ((INIT_SCALE_BALL.x + INIT_SCALE_RIGHT.x) / 2.0f);
        float y_dist_right = fabs(ball_y - right_y) - ((INIT_SCALE_BALL.y + INIT_SCALE_RIGHT.y) / 2.0f);

        if (x_dist_left < 0.0f && y_dist_left < 0.0f) 
        {
            g_ball_movement = -g_ball_movement;
        }

        if (x_dist_right < 0.0f && y_dist_right < 0.0f) 
        {
            g_ball_movement = -g_ball_movement;
        }

        if (g_ball_position.x > WALL) 
        {
            GAME_STATE = 1;
        }

        if (g_ball_position.x < -WALL) {
            GAME_STATE = 2;
        }
    }
    else if (GAME_STATE != 0) 
    {
        g_win_matrix = glm::mat4(1.0f);
        g_win_matrix = glm::translate(g_win_matrix, INIT_POS_WIN);
        g_win_matrix = glm::scale(g_win_matrix, INIT_SCALE_WIN);
    }



}

void draw_object(glm::mat4& object_model_matrix, GLuint& object_texture_id)
{
    g_shader_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so we use 6 instead of 3
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Vertices
    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Bind texture
    if (GAME_STATE == 0) 
    {
        draw_object(g_left_matrix, g_left_texture_id);
        draw_object(g_right_matrix, g_right_texture_id);
        draw_object(g_ball_matrix, g_ball_texture_id);
    }
    else if (GAME_STATE == 1)
    {
        draw_object(g_win_matrix, g_p1_texture_id);
    }
    else if (GAME_STATE == 2) 
    {
        draw_object(g_win_matrix, g_p2_texture_id);
    }


    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }


int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
