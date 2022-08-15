// TO-DO: Wall textures should not be integers. Instead, use character codes
// read from the map file. Store in an array and do array indexing instead of
// this manual copy-pasted 1-9 code.

// TO-DO: Animations should be easy. Just keep track of some animation index
// and increase it modulo each n frames.

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>


// Defines a floating point 2-vector.
typedef struct fVector2 {
    double x;
    double y;
} fVector2;


int min(int a, int b);
double castRay(fVector2 playerPos, double playerA);
void readMap(fVector2* playerPos, double *playerA);
void playerMove(double dx, double dy);
bool handleEvents(SDL_Event);
double veclen(fVector2 vec);
void drawScreen(SDL_Renderer* renderer, SDL_Texture* screen);

// Define math constants.
const double PI = 3.1415;

// Defines the height and width of a map in tiles.
#define MAPSIZE 256

// Frames-per-second to cap game at.
const int FPS = 60;

// Player's movement and turning (angle) speed.
const double MOVESPEED   = 0.0875;
const double TURNSPEED   = 0.0025625;

// Constant controlling how fast distant things become dark.
const double FOG         = 20.0;

// Defines how wide and tall the walls are.
const double WALL_WIDTH  = 1.0;
const double WALL_HEIGHT = 2.0;

// Screen dimension constants.
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

// Defines the projection plane characteristics.
const int PROJ_PLANE_W    = SCREEN_WIDTH;
const int PROJ_PLANE_H    = SCREEN_HEIGHT;
const double PROJ_PLANE_D = 240.0;



// Global variables are defined here. This includes the player position, angle,
// map array, and wall surface pointer array.
fVector2 playerPos;
double playerA = 0.0;
int map[MAPSIZE][MAPSIZE];
SDL_Surface* wallSurfaces[128];



int main(int argc, char** argv) {
    SDL_Init(SDL_INIT_EVERYTHING);

    // Create main window and renderer.
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer);

    // Create a new texture representing the screen. We will draw on this.
    SDL_Texture* plane = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                 SDL_TEXTUREACCESS_STREAMING, PROJ_PLANE_W,
                                                              PROJ_PLANE_H);

    // Grab mouse and setup event handler, and read the map.
    SDL_SetRelativeMouseMode(SDL_TRUE);
    SDL_Event e;
    readMap(&playerPos, &playerA);


    // Load in wall texture image.
    char buffer[20];
    for (int i = 0; i < 9; i++) {
        snprintf(buffer, 20, "textures/wall%d.bmp", i+1);
        printf("%s\n", buffer);
        wallSurfaces[i] = SDL_LoadBMP(buffer);
    }


    /* MAIN LOOP */
    bool quit = false;
    while (!quit) {
        // Get time at start of frame.
        Uint32 framestart = SDL_GetTicks();

        // Handle events and draw the screen.
        quit = handleEvents(e);
        drawScreen(renderer, plane);

        // Get time that frame took.
        Uint32 frameTime = SDL_GetTicks() - framestart;
        
        // Wait until we hit our target FPS.
        Uint32 delayTime = 1000 / FPS - frameTime;
        if (delayTime > 1000 / FPS) delayTime = 0;
        SDL_Delay(delayTime);
    }

    // Destroy main screen texture.
    SDL_DestroyTexture(plane);

    // Destroy window.
    SDL_DestroyWindow(window);

    // Quit SDL subsystems.
    SDL_Quit();


    return EXIT_SUCCESS;
}


/* Function: castRay
 * -----------------
 *  Casts a ray from `origin` at `angle`, and returns the distance until a wall
 *  is hit.
 * 
 *  origin: Origin of the ray.
 *  angle:  Angle of the ray in radians.
 *
 *  returns: length of ray.
 */
double castRay(fVector2 origin, double angle) {
    // TO-DO: This should not be fixed-size step iteration!
    const double raystep = 0.0150;
    double dx = raystep * cos(angle);
    double dy = raystep * sin(angle);
    double distance = 0.0;

    for (double x = origin.x, y = origin.y; !map[(int)round(x)][(int)round(y)]; x += dx, y += dy)
        distance += raystep;
    return distance;
}


/* Function: readMap
 * -----------------
 *  Reads the file "map.txt" which sets the level geometry as well as player
 *  start position and angle.
 *
 *  *playerPos: Pointer to the player position.
 *  *playerA:   Pointer to the player angle.
 */
void readMap(fVector2* playerPos, double *playerA) {
    // Zero out map to make sure no garbage is left.
    for (int i = 0; i < MAPSIZE; i++)
        for (int j = 0; j < MAPSIZE; j++)
            map[i][j] = 0;

    // Read map file.
    FILE* mapfile = fopen("map.txt", "r");

    // Read player starting x and y positions, as well as starting angle.
    char s[MAPSIZE] = { 0 };
    fgets(s, MAPSIZE, mapfile);
    playerPos->x = atof(s);
    fgets(s, MAPSIZE, mapfile);
    playerPos->y = atof(s);
    fgets(s, MAPSIZE, mapfile);
    *playerA = atof(s);

    // Read level geometry.
    for (int i = 0; fgets(s, MAPSIZE, mapfile) != NULL; i++) {
        for (int j = 0; s[j] != '\0'; j++) {
            if (s[j] == ' ')
                map[j][i] = 0;
            else if (isdigit(s[j]))
                map[j][i] = s[j] - '0';  // Not sure why i and j need to be transposed
                                         // here. Possible x/y row/col dominance issue.
            else if (s[j] != '\n')
                printf("Error: Unknown character found in map file: %c\n", s[j]);
        }
    }

    fclose(mapfile);
}

/* Function: los
 * -------------
 *  Takes two points and determines if an unbroken line can be drawn between them.
 *
 *  P, Q: Points in 2-space.
 *
 *  returns: A boolean value representing if there is line-of-sight between
 *           the points.
 */
bool los(fVector2 P, fVector2 Q) {
    // Create a normalized direction vector from P to Q.
    fVector2 dirVec = {Q.x - P.x, Q.y - P.y};
    double len = veclen(dirVec);
    dirVec.x /= len;
    dirVec.y /= len;

    // Step through from P to Q until we reach Q. If we hit a wall return false.
    for (double x = P.x, y = P.y; veclen((fVector2) {x-P.x, y-P.y}) < len; x += dirVec.x, y += dirVec.y)
        if (map[(int)round(x)][(int)round(y)]) return false;
    return true;
}

/* Function: handleEvents
 * ----------------------
 *  Handles the events queue and keyboard state. Returns signal indicating 
 *  if the user wishes to quit the program.
 *
 *  e: SDL Event handler.
 *
 *  returns: A boolean value of user sending QUIT-signal.  
 */ 
bool handleEvents(SDL_Event e) {
    // Handle potential events in the queue.
    while (SDL_PollEvent(&e) != 0) {
        // "x-ing" out sends a quit singal.
        if (e.type == SDL_QUIT)
            return true;

        // If we move the mouse update the player's angle but keep it modulo pi.
        if (e.type == SDL_MOUSEMOTION) {
            playerA += e.motion.xrel * TURNSPEED;
            if (playerA > PI) playerA -= 2*PI;
            if (playerA < -PI) playerA += 2*PI;

        }
    }

    // Scan keyboard for keyboard state.
    const Uint8* currentKeyStates = SDL_GetKeyboardState(0);
    if (currentKeyStates[SDL_SCANCODE_ESCAPE])
        return true;

    if (currentKeyStates[SDL_SCANCODE_W]) {
        double dx = cos(playerA) * MOVESPEED;
        double dy = sin(playerA) * MOVESPEED;
        playerMove(dx, dy);
    }

    if (currentKeyStates[SDL_SCANCODE_S]) {
        double dx = cos(playerA) * MOVESPEED;
        double dy = sin(playerA) * MOVESPEED;
        playerMove(-dx, -dy);
    }

    if (currentKeyStates[SDL_SCANCODE_A]) {
        double dx = cos(playerA - PI/2) * MOVESPEED;
        double dy = sin(playerA - PI/2) * MOVESPEED;
        playerMove(dx, dy);
    }
    if (currentKeyStates[SDL_SCANCODE_D]) {
        double dx = cos(playerA + PI/2) * MOVESPEED;
        double dy = sin(playerA + PI/2) * MOVESPEED;
        playerMove(dx, dy);
    }

    return false;
}


/* Function: drawScreen
 * --------------------
 *  Renders one frame of gameplay.
 *
 *  renderer: Pointer to SDL Renderer to use for rendering.
 *  plane:    Pointer to SDL Texture of the projection plane.
 */
void drawScreen(SDL_Renderer* renderer, SDL_Texture* plane) {
    // Lock the plane texture because oh boy we're about to draw on it.
    void *pixels = NULL;
    int pitch;
    SDL_LockTexture(plane, NULL, &pixels, &pitch);

    // Draw the floor and ceiling. TO-DO: This is a placeholder.
    for (int y = 0; y < PROJ_PLANE_H / 2; y++) {
        // Set gradient color depending on how far from the top and bottom
        // edges of the screen we draw.
        Uint8 color = 56.0 * exp(-1.5 * (float)y / (float)PROJ_PLANE_H);
        Uint32 ceilColor   = ((((((color) << 8) + color) << 8) + color) << 8) + 255;
        Uint32 floorColor  = ((((((color) << 8) + color) << 8) + color/3) << 8) + 255;

        // Set texture pixels to the gradient colors.
        for (int x = 0; x < PROJ_PLANE_W; x++) {
            *(((Uint32*) pixels) + PROJ_PLANE_W * y + x) = ceilColor;
            *(((Uint32*) pixels) + PROJ_PLANE_W * (PROJ_PLANE_H - 1) - y * PROJ_PLANE_W + x) = floorColor;
        }
    }



    // Draw the walls by sweeping a ray in front of the player.
    for (int plane_x = -PROJ_PLANE_W / 2; plane_x < PROJ_PLANE_W / 2; plane_x++) {
        // Cast a ray and record the distance to the nearest wall.
        double angle = atan((float)plane_x / (float)PROJ_PLANE_D) + playerA;
        double distance = castRay(playerPos, angle);

        // Get x and y coordinates of collision point.
        double x = distance * cos(angle) + playerPos.x;
        double y = distance * sin(angle) + playerPos.y;

        // Record what type of wall we hit.
        int type = map[(int)round(x)][(int)round(y)];
        

        // Decide where we hit the wall. If an x or y value is very close to
        // 0.50, it has just hit a wall, so we want the OTHER value. This is
        // due to rounding, where 0.50 -> 1 and 0.49 -> 0, i.e. we just hit
        // a wall.
        // The &_ is a dummy. The integral part is thrown away, but we need
        // to store it somewhere to not throw a segfault.
        // The 0.50 is needed to align textures properly. Again, due to rounding.
        // I fucking hate rounding.
        double wallcoord = 0.0;
        double _;
        if (fabs(x - round(x)) > fabs(y - round(y)))
            wallcoord = modf(y+0.50, &_);
        else
            wallcoord = modf(x+0.50, &_);


        // Get perpendicular distance from the player to the wall. This is what
        // we care about for drawing.
        distance *= cos(angle - playerA);

        // Calculate what color the wall should be based on distance.
        Uint8 color = 255.0 * exp(-distance/FOG);

        // Select proper texture to draw.
        SDL_Surface* wall_img_surf = wallSurfaces[type-1];

        // Calculate apparent height of the wall as it appears to the player.
        double height        = WALL_HEIGHT * PROJ_PLANE_D / distance;
        double clampedHeight = min(height, PROJ_PLANE_H);

        // Draw the strip of wall corresponding to this ray.
        int wallY = (int) ((PROJ_PLANE_H - clampedHeight) / 2);
        for (int plane_y = wallY; plane_y < clampedHeight+wallY; plane_y++) {
            // Get where on the texture we are.
            int texture_x = wallcoord * 127;
            int texture_y;
            if (height < PROJ_PLANE_H) {
                texture_y = (plane_y - wallY) / (float)clampedHeight * 127;
            }
            else {
                texture_y = 0;
                texture_y = ((height - clampedHeight) / 2.0 + plane_y) / (float)height * 127;
            }


            // Read the color on the texture.
            Uint8 r = *(((Uint8*) (wall_img_surf->pixels)) + texture_y * 3*128 + 3*texture_x + 0);
            Uint8 g = *(((Uint8*) (wall_img_surf->pixels)) + texture_y * 3*128 + 3*texture_x + 1);
            Uint8 b = *(((Uint8*) (wall_img_surf->pixels)) + texture_y * 3*128 + 3*texture_x + 2);
            r *= color / 255.0;
            g *= color / 255.0;
            b *= color / 255.0;

            // Write that color to the projection plane texture.
            Uint32 wallColor = 0;
            wallColor = (((((b << 8) + g) << 8) + r) << 8) + 255;
            *(((Uint32*) pixels) + PROJ_PLANE_W * plane_y + (plane_x + PROJ_PLANE_W / 2)) = wallColor;
        }
    }




    // Actually draw the applied changes.
    SDL_UnlockTexture(plane);
    SDL_RenderCopy(renderer, plane, NULL, NULL);

    // Present screen.
    SDL_RenderPresent(renderer);
}


/* Function: playerMove
 * --------------------
 *  Updates the player's position by (dx, dy).
 *
 *  dx: Delta-x.
 *  dy: Delta-y.
 */
void playerMove(double dx, double dy) {
    if (!map[(int)round(playerPos.x + dx)][(int)round(playerPos.y)])
        playerPos.x += dx;
    if (!map[(int)round(playerPos.x)][(int)round(playerPos.y + dy)])
        playerPos.y += dy;
}


/* Function: veclen
 * ----------------
 *  Calculates the Euclidean distance of a 2-vector.
 *
 *  v: Vector to calculate length of.
 *
 *  returns: Length of the vector v .
 */
double veclen(fVector2 v) {
    return sqrt(v.x * v.x + v.y * v.y);
}


int min(int a, int b) {
    return (a < b) ? a : b;
}
