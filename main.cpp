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
                WALL = INIT_X,
                SPEED_INC = 0.000f, // turned off cause bugs with collision
                START_AI = -1.0f;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;
bool SINGLE_PLAYER = false; // set default player mode to 2

constexpr char LEFT_SPRITE_FILEPATH[] = "assets/cat_left.png",
               RIGHT_SPRITE_FILEPATH[] = "assets/cat_right.png",
               BALL_SPRITE_FILEPATH[] = "assets/mouse.png",
               BALL2_SPRITE_FILEPATH[] = "assets/ball2.png",
               BALL3_SPRITE_FILEPATH[] = "assets/ball3.png",
               P1_SPRITE_FILEPATH[] = "assets/p1_win.png",
               P2_SPRITE_FILEPATH[] = "assets/p2_win.png";

constexpr float MINIMUM_COLLISION_DISTANCE = 1.0f;

constexpr glm::vec3 INIT_SCALE_LEFT = glm::vec3(SCALE_X, SCALE_Y, 1.0f),
                    INIT_SCALE_RIGHT = glm::vec3(SCALE_X, SCALE_Y, 1.0f),
                    INIT_SCALE_BALL = glm::vec3(0.5f, 0.5f, 1.0f),
                    INIT_SCALE_WIN = glm::vec3(1.0f, 0.5f, 1.0f);

constexpr glm::vec3 INIT_POS_LEFT = glm::vec3(-INIT_X, 0.0f, 1.0f),
                    INIT_POS_RIGHT = glm::vec3(INIT_X, 0.0f, 1.0f),
                    INIT_POS_BALL = glm::vec3(0.0f, 0.0f, 0.0f),
                    INIT_POS_WIN = glm::vec3(0.0f, 0.0f, 0.0f);

float g_angle = 0.0f;    // current angle
float ROT_SPEED = 300.0f;


SDL_Window* g_display_window;

AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();
glm::mat4 g_view_matrix, g_projection_matrix, g_left_matrix, g_right_matrix, g_ball_matrix, g_ball_matrix2, g_ball_matrix3, g_win_matrix;

float g_previous_ticks = 0.0f;

GLuint g_left_texture_id,
       g_right_texture_id,
       g_ball_texture_id,
       g_ball_texture_id2,
       g_ball_texture_id3,
       g_p1_texture_id,
       g_p2_texture_id;

glm::vec3 g_left_position = glm::vec3(0.0f, 0.0f, 0.0f),
          g_right_position = glm::vec3(0.0f, 0.0f, 0.0f),
          g_ball_position = glm::vec3(0.0f, 0.0f, 0.0f),
          g_ball_position2 = glm::vec3(0.0f, 0.0f, 0.0f),
          g_ball_position3 = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_left_movement = glm::vec3(0.0f, 0.0f, 0.0f),
          g_right_movement = glm::vec3(0.0f, 0.0f, 0.0f),
          g_ball_movement = glm::vec3(0.0f, 0.0f, 0.0f),
          g_ball_movement2 = glm::vec3(0.0f, 0.0f, 0.0f),
          g_ball_movement3 = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_win_size = glm::vec3(1.0f, 1.0f, 0.0f),
          g_win_scale = glm::vec3(0.0f, 0.0f, 0.0f);

constexpr float g_start_speed = 2.0f;
float g_ball_speed = g_start_speed; 
float g_paddle_speed = 4.0f;
int NUM_BALLS = 1;

void initialise();
void process_input();
void update();
void render();
void shutdown();

// declarations cause c++
void restart();
bool check_collision(glm::vec3& ball_pos, const glm::vec3& ball_init, const glm::vec3& ball_scale, glm::vec3& paddle_pos, const glm::vec3& paddle_init, const glm::vec3& paddle_scale);
void updateBall(glm::vec3& ball_pos, glm::vec3& ball_move);


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
    g_display_window = SDL_CreateWindow("A Purr-fect Game of Cat-ch!",
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
    g_ball_matrix2 = glm::mat4(1.0f);
    g_ball_matrix3 = glm::mat4(1.0f);
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
    g_ball_texture_id2 = load_texture(BALL2_SPRITE_FILEPATH);
    g_ball_texture_id3 = load_texture(BALL3_SPRITE_FILEPATH);
    g_p1_texture_id = load_texture(P1_SPRITE_FILEPATH);
    g_p2_texture_id = load_texture(P2_SPRITE_FILEPATH);

    // initial ball movement
    srand(unsigned int(std::time(nullptr))); // seed with random to ensure randomness
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
            case SDLK_1:
                NUM_BALLS = 1;
                break;
            case SDLK_2:
                if (NUM_BALLS == 1) // if only one ball, create another
                {
                    updateBall(g_ball_position2, g_ball_movement2);
                }
                NUM_BALLS = 2;
                break;
            case SDLK_3:
                if (NUM_BALLS != 3) // if not three balls create the third
                {
                    updateBall(g_ball_position3, g_ball_movement3);
                }
                if (NUM_BALLS == 1) // if only one ball create the second as well
                {
                    updateBall(g_ball_position2, g_ball_movement2);
                }
                NUM_BALLS = 3;
                break;

            // extra stuff for testing
            case SDLK_r:
                restart();
                break;
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
    updateBall(g_ball_position, g_ball_movement);
    updateBall(g_ball_position2, g_ball_movement2);
    updateBall(g_ball_position3, g_ball_movement3);

    g_ball_speed = 1.0f;
    g_win_size = glm::vec3(1.0f, 1.0f, 0.0f);
    GAME_STATE = 0;
    NUM_BALLS = 1;
}

void updateBall(glm::vec3& ball_pos, glm::vec3& ball_move) 
{
    ball_pos = glm::vec3(0.0f);
    ball_move.x = float(rand() % 101) - 50;
    ball_move.y = float(rand() % 101) - 50;
    ball_move = glm::normalize(ball_move);
    if (ball_move.x < 0.1f || ball_move.y < 0.1f) { updateBall(ball_pos, ball_move); }
}

bool check_collision(glm::vec3& ball_pos, const glm::vec3& ball_init, const glm::vec3& ball_scale, glm::vec3& paddle_pos, const glm::vec3& paddle_init, const glm::vec3& paddle_scale)
{
    float ball_x = ball_pos.x + ball_init.x;
    float ball_y = ball_pos.y + ball_init.y;

    float paddle_x = paddle_pos.x + paddle_init.x;
    float paddle_y = paddle_pos.y + paddle_init.y;

    float dist_x = fabs(ball_x - paddle_x) - ((ball_scale.x + paddle_scale.x) / 2.0f);
    float dist_y = fabs(ball_y - paddle_y) - ((ball_scale.y + paddle_scale.y) / 2.0f);

    bool front = fabs(ball_x) < fabs(paddle_x);
    
    // extra condition so top/bottom hits don't count and to prevent weird bugs
    return (dist_x < 0.0f && dist_y < 0.0f && fabs(dist_x) < 0.1f);
}

void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND; // get the current number of ticks
    float delta_time = ticks - g_previous_ticks; // the delta time is the difference from the last frame
    g_previous_ticks = ticks;

    // make ball speed a function of time so ball speed increases as match drags on
    g_ball_speed = g_start_speed + g_start_speed * (ticks / 6000.0f);

    g_angle += ROT_SPEED * delta_time;

    // if not end screen
    if (GAME_STATE == 0)
    {    
        /*------------BALL ONE------------*/
        g_ball_position += g_ball_movement * g_ball_speed * delta_time;
        // bouncing off top and bottom
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
        g_ball_matrix = glm::rotate(g_ball_matrix, glm::radians(g_angle), glm::vec3(0.0f, 0.0f, 1.0f));
        g_ball_matrix = glm::scale(g_ball_matrix, INIT_SCALE_BALL);

        /*------------BALL TWO------------*/
        if (NUM_BALLS >= 2) {
            g_ball_position2 += g_ball_movement2 * g_ball_speed * delta_time;
            // bouncing off top and bottom
            if (g_ball_position2.y > UPPER_BOUND)
            {
                g_ball_position2.y = UPPER_BOUND;
                g_ball_movement2.y = -g_ball_movement2.y;
            }
            if (g_ball_position2.y < LOWER_BOUND)
            {
                g_ball_position2.y = LOWER_BOUND;
                g_ball_movement2.y = -g_ball_movement2.y;
            }
            g_ball_matrix2 = glm::mat4(1.0f);
            g_ball_matrix2 = glm::translate(g_ball_matrix2, INIT_POS_BALL);
            g_ball_matrix2 = glm::translate(g_ball_matrix2, g_ball_position2);
            g_ball_matrix2 = glm::rotate(g_ball_matrix2, glm::radians(-g_angle), glm::vec3(0.0f, 0.0f, 1.0f));
            g_ball_matrix2 = glm::scale(g_ball_matrix2, INIT_SCALE_BALL);
        }

        /*------------BALL THREE------------*/
        if (NUM_BALLS >= 3) {
            g_ball_position3 += g_ball_movement3 * g_ball_speed * delta_time;
            // bouncing off top and bottom
            if (g_ball_position3.y > UPPER_BOUND)
            {
                g_ball_position3.y = UPPER_BOUND;
                g_ball_movement3.y = -g_ball_movement3.y;
            }
            if (g_ball_position3.y < LOWER_BOUND)
            {
                g_ball_position3.y = LOWER_BOUND;
                g_ball_movement3.y = -g_ball_movement3.y;
            }
            g_ball_matrix3 = glm::mat4(1.0f);
            g_ball_matrix3 = glm::translate(g_ball_matrix3, INIT_POS_BALL);
            g_ball_matrix3 = glm::translate(g_ball_matrix3, g_ball_position3);
            g_ball_matrix3 = glm::rotate(g_ball_matrix3, glm::radians(g_angle), glm::vec3(0.0f, 0.0f, 1.0f));
            g_ball_matrix3 = glm::scale(g_ball_matrix3, INIT_SCALE_BALL);
        }

        // if t was pressed 
        if (SINGLE_PLAYER) {
            // one ball move towards that ball if ball is moving towards it
            if (NUM_BALLS == 1) {
                if (g_ball_position.x > START_AI && g_ball_movement.x > 0.0f)
                {
                    g_right_movement.y = ((g_ball_position.y > g_right_position.y) ? 1.0f : -1.0f);
                }
            }
            // two ball, choose to move based on whichever is closer and moving in its direction
            else if (NUM_BALLS == 2) {
                // ball move towards paddle AND ball greater than distance && ball1 closer than ball2 or second ball might be closer but moving away
                if (g_ball_movement.x > 0.0f && g_ball_position.x > START_AI && (g_ball_position.x > g_ball_position2.x || g_ball_movement2.x < 0.0f))
                {
                    g_right_movement.y = ((g_ball_position.y > g_right_position.y) ? 1.0f : -1.0f);
                }
                // last option so move towards ball2 if ball moving towards
                else if (g_ball_movement2.x > 0.0f && g_ball_position2.x > START_AI)
                {
                    g_right_movement.y = ((g_ball_position2.y > g_right_position.y) ? 1.0f : -1.0f);
                }
            }
            else if (NUM_BALLS == 3) {
                if (g_ball_movement.x > 0.0f && g_ball_position.x > START_AI && (g_ball_position.x > g_ball_position2.x || g_ball_movement2.x < 0.0f) && (g_ball_position.x > g_ball_position3.x || g_ball_movement3.x < 0.0f))
                {
                    g_right_movement.y = ((g_ball_position.y > g_right_position.y) ? 1.0f : -1.0f);
                }
                else if (g_ball_movement2.x > 0.0f && g_ball_position2.x > START_AI && (g_ball_position2.x > g_ball_position.x || g_ball_movement.x < 0.0f) && (g_ball_position2.x > g_ball_position3.x || g_ball_movement3.x < 0.0f))
                {
                    g_right_movement.y = ((g_ball_position2.y > g_right_position.y) ? 1.0f : -1.0f);
                }
                else if (g_ball_movement3.x > 0.0f && g_ball_position3.x > START_AI && (g_ball_position3.x > g_ball_position2.x || g_ball_movement2.x < 0.0f) && (g_ball_position3.x > g_ball_position.x || g_ball_movement.x < 0.0f))
                {
                    g_right_movement.y = ((g_ball_position3.y > g_right_position.y) ? 1.0f : -1.0f);
                }
            }
        }

        // Add direction * units per second * elapsed time
        g_left_position += g_left_movement * g_paddle_speed * delta_time;

        // prevent paddle from going out of bounds
        if (g_left_position.y > UPPER_BOUND) 
        {
            g_left_position.y = UPPER_BOUND;
        }
        if (g_left_position.y < LOWER_BOUND) 
        {
            g_left_position.y = LOWER_BOUND;
        }

        g_left_matrix = glm::mat4(1.0f);
        g_left_matrix = glm::translate(g_left_matrix, INIT_POS_LEFT);
        g_left_matrix = glm::translate(g_left_matrix, g_left_position);
        g_left_matrix = glm::scale(g_left_matrix, INIT_SCALE_LEFT);


        g_right_position += g_right_movement * g_paddle_speed * delta_time;

        // prevent paddle from going out of bounds
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


        // check collision
        bool hit_left = check_collision(g_ball_position, INIT_POS_BALL, INIT_SCALE_BALL, g_left_position, INIT_POS_LEFT, INIT_SCALE_LEFT);
        bool hit_right = check_collision(g_ball_position, INIT_POS_BALL, INIT_SCALE_BALL, g_right_position, INIT_POS_RIGHT, INIT_SCALE_RIGHT);
       
        bool hit_left2 = check_collision(g_ball_position2, INIT_POS_BALL, INIT_SCALE_BALL, g_left_position, INIT_POS_LEFT, INIT_SCALE_LEFT);
        bool hit_right2 = check_collision(g_ball_position2, INIT_POS_BALL, INIT_SCALE_BALL, g_right_position, INIT_POS_RIGHT, INIT_SCALE_RIGHT);

        bool hit_left3 = check_collision(g_ball_position3, INIT_POS_BALL, INIT_SCALE_BALL, g_left_position, INIT_POS_LEFT, INIT_SCALE_LEFT);
        bool hit_right3 = check_collision(g_ball_position3, INIT_POS_BALL, INIT_SCALE_BALL, g_right_position, INIT_POS_RIGHT, INIT_SCALE_RIGHT);


        // update state of game
        if (hit_left || hit_right) 
        {
            g_ball_movement.x = (g_ball_movement.x > 0) ? -(g_ball_movement.x + SPEED_INC) : -(g_ball_movement.x - SPEED_INC);
        }
        if (NUM_BALLS >= 2 && (hit_left2 || hit_right2)) 
        {
            g_ball_movement2.x = (g_ball_movement2.x > 0) ? -(g_ball_movement2.x + SPEED_INC) : -(g_ball_movement2.x - SPEED_INC);
        }
        if (NUM_BALLS >= 3 && (hit_left3 || hit_right3)) 
        {
            g_ball_movement3.x = (g_ball_movement3.x > 0) ? -(g_ball_movement3.x + SPEED_INC) : -(g_ball_movement3.x - SPEED_INC);
        }

        if (g_ball_position.x > WALL || g_ball_position2.x > WALL || g_ball_position3.x > WALL)  
        {
            GAME_STATE = 1;
        }
        if (g_ball_position.x < -WALL || g_ball_position2.x < -WALL || g_ball_position3.x < -WALL) {
            GAME_STATE = 2;
        }
    }
    // someone has won
    else
    {
        g_win_size += g_win_scale * delta_time;
        if (g_win_size.x < 6.0f) 
        {
            g_win_scale.x = 1.0f;
            g_win_scale.y = 1.0f;
        }
        else 
        {
            g_win_scale = glm::vec3(0.0f);
        }
        g_win_matrix = glm::mat4(1.0f);
        g_win_matrix = glm::translate(g_win_matrix, INIT_POS_WIN);
        g_win_matrix = glm::scale(g_win_matrix, INIT_SCALE_WIN);
        g_win_matrix = glm::scale(g_win_matrix, g_win_size);
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
        if (NUM_BALLS >= 2) { draw_object(g_ball_matrix2, g_ball_texture_id2); }
        if (NUM_BALLS >= 3) { draw_object(g_ball_matrix3, g_ball_texture_id3); }
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
