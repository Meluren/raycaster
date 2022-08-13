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

#define MAPSIZE 256
#define COLL_ENEMIES 16
#define ENEMIES 64


// Game engine constants.
const double MOVESPEED = 0.0875;
const double ANGLESPEED = 0.0025625;
const double FOG = 20.0;
const double VERTICALFOV = 1.257;
const double DIST_TO_PROJ_PLANE = 1.0;
const int FPS = 60;


// Useful struct typedefs.
typedef struct fVector2 {
    double x;
    double y;
} fVector2;

typedef struct ray {
    double distance;
    double coord;
    int type;
} ray;

typedef struct Enemy {
    int id;
    int type;
    fVector2 pos;
} Enemy;


// Functions.
ray distanceToWall(fVector2 playerPos, double playerA);
void readMap(fVector2* playerPos, double *playerA);
void playerMove(double dx, double dy);
bool handleEvents(SDL_Event);
double veclen(fVector2 vec);
void drawScreen(SDL_Renderer* renderer, SDL_Texture* screen);


// Screen dimension constants.
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;


// Set enemy id counter to zero.
int enemyIdCtr = 0;


// Create player position.
fVector2 playerPos;
double playerA = 0.0;


// Define map globally -- a lot of functions care about it.
int map[MAPSIZE][MAPSIZE];


// Textures can be global as well.
SDL_Surface* wall_img_surf1;
SDL_Surface* wall_img_surf2;
SDL_Surface* wall_img_surf3;
SDL_Surface* wall_img_surf4;
SDL_Surface* wall_img_surf5;
SDL_Surface* wall_img_surf6;
SDL_Surface* wall_img_surf7;
SDL_Surface* wall_img_surf8;
SDL_Surface* wall_img_surf9;


// Enemy textures.
SDL_Surface* skeleton_img_surf;
SDL_Texture* skeleton_texture;



// This array will be filled with pointers to all enemies on the map.
Enemy* enemies[ENEMIES] = { NULL };



int main() {
    // This controls the main loop.
    bool quit = false;

    // Initialize SDL.
    SDL_Init(SDL_INIT_VIDEO);

    // Create main window and renderer.
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer);

    // Create a new texture representing the screen. We will draw on this.
    SDL_Texture* screen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                 SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH,
                                                              SCREEN_HEIGHT);


    // Grab mouse and setup event handler.
    SDL_SetRelativeMouseMode(SDL_TRUE);
    SDL_Event e;

    // Read in map from input.
    readMap(&playerPos, &playerA);

    // Load in wall texture image.
    wall_img_surf1 = SDL_LoadBMP("wall1.bmp");
    wall_img_surf2 = SDL_LoadBMP("wall2.bmp");
    wall_img_surf3 = SDL_LoadBMP("wall3.bmp");
    wall_img_surf4 = SDL_LoadBMP("wall4.bmp");
    wall_img_surf5 = SDL_LoadBMP("wall5.bmp");
    wall_img_surf6 = SDL_LoadBMP("wall6.bmp");
    wall_img_surf7 = SDL_LoadBMP("wall7.bmp");
    wall_img_surf8 = SDL_LoadBMP("wall8.bmp");
    wall_img_surf9 = SDL_LoadBMP("wall9.bmp");


    // Load in enemy textures.
    skeleton_img_surf = SDL_LoadBMP("skeleton.bmp");
    skeleton_texture  = SDL_CreateTextureFromSurface(renderer, skeleton_img_surf);


    Enemy testEnemy;
    testEnemy.pos = ((fVector2) {4.0, 16.0});
    testEnemy.type = 0;
    testEnemy.id = 0;
    enemies[0] = &testEnemy;

    // Main loop.
    while (!quit) {
        // Get time at start of frame.
        Uint32 framestart = SDL_GetTicks();

        quit = handleEvents(e);
        drawScreen(renderer, screen);
        //SDL_UpdateWindowSurface(window);

        // Get time that frame took.
        Uint32 frameTime = SDL_GetTicks() - framestart;
        
        // Wait until we hit 60 fps.
        Uint32 delayTime = 1000 / FPS - frameTime;
        //printf("%u\n", frameTime);
        if (delayTime > 1000 / FPS) delayTime = 0;
        SDL_Delay(delayTime);
    }

    // Destroy main screen texture.
    SDL_DestroyTexture(screen);

    // Destroy window.
    SDL_DestroyWindow(window);

    // Quit SDL subsystems.
    SDL_Quit();


    return EXIT_SUCCESS;
}


/* Returns the distance to the wall from the player by casting a ray. */
ray distanceToWall(fVector2 playerPos, double playerA) {
    const double raystep = 0.0150; // TO-DO: This should not be fixed-sized.
    // Ray dx and dy are calculated here.
    double dx = raystep * cos(playerA);
    double dy = raystep * sin(playerA);
    double distance = 0.0;



    // Cast ray out from player.
    ray r;
    double x, y;
    for (x = playerPos.x, y = playerPos.y;
            !map[(int)round(x)][(int)round(y)]; x += dx, y += dy) {
        // Increase distance ray has travelled.
        distance += raystep;

    }

    // Record what type of wall we hit.
    r.type = map[(int)round(x)][(int)round(y)];
    

    // Decide where we hit the wall. If an x or y value is very close to
    // 0.50, it has just hit a wall, so we want the OTHER value. This is
    // due to rounding, where 0.50 -> 1 and 0.49 -> 0, i.e. we just hit
    // a wall.
    // The &dx is a dummy. The integral part is thrown away, but we need
    // to store it somewhere to not throw a segfault.
    // The 0.50 is needed to align textures properly. Again, due to rounding.
    // I fucking hate rounding.
    if (fabs(x - round(x)) > fabs(y - round(y))) {
        r.coord = modf(y+0.50, &dx);
        //printf("%.3f\n", r.coord);
    }
    else {
        r.coord = modf(x+0.50, &dx);
        //printf("%.3f\n", r.coord);
    }
    r.distance = distance;

    return r;
}


/* Reads in the map from stdin. It sets the user position as well. */
void readMap(fVector2* playerPos, double *playerA) {
    // Zero out map to make sure no garbage is left.
    for (int i = 0; i < MAPSIZE; i++)
        for (int j = 0; j < MAPSIZE; j++)
            map[i][j] = 0;

    // Open map file for reading.
    FILE* mapfile = fopen("map.txt", "r");
    // Read player starting x and y positions, as well as starting angle.
    char s[MAPSIZE] = { 0 };
    fgets(s, MAPSIZE, mapfile);
    playerPos->x = atof(s);
    fgets(s, MAPSIZE, mapfile);
    playerPos->y = atof(s);
    fgets(s, MAPSIZE, mapfile);
    *playerA = atof(s);

    // Read in map.
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

    // Close map file.
    fclose(mapfile);
}

bool obstructed(Enemy* enemy) {
    // Create direction vector from the player to the enemy and normalize it.
    fVector2 directionVector;
    directionVector.x = enemy->pos.x - playerPos.x;
    directionVector.y = enemy->pos.y - playerPos.y;
    double len = veclen(directionVector);
    directionVector.x /= len;
    directionVector.y /= len;

    for (double x = 0, y = 0; veclen((fVector2){x, y}) < len; x += directionVector.x, y += directionVector.y) {
        if (map[(int)round(x+playerPos.x)][(int)round(y+playerPos.y)]) return true;
    }

    return false;
}


void drawEnemies(SDL_Renderer* renderer) {
    for (int i = 0; i < ENEMIES; i++) {
        if (enemies[i] == NULL) continue;
        if (!obstructed(enemies[i])) {
            // Determine where on screen to draw the enemy.
            fVector2 vecToEnemy;
            vecToEnemy.x = enemies[i]->pos.x - playerPos.x;
            vecToEnemy.y = enemies[i]->pos.y - playerPos.y;
            double angleToEnemy = atan(vecToEnemy.y / vecToEnemy.x) - playerA;
            //printf("Angle to enemy: %.3f\n", angleToEnemy);
            if (angleToEnemy > 3.1415) angleToEnemy -= 2*3.1415;
            if (angleToEnemy < -3.1415) angleToEnemy += 2*3.1415;
            printf("Angle: %.3f\n", angleToEnemy);
        }
    }
}


/* Handles the events queue and keyboard state. Returns signal indicating
 * if the user wishes to quit the program. */
bool handleEvents(SDL_Event e) {
    // Handle events in the queue.
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT)
            return true;
        if (e.type == SDL_MOUSEMOTION) {
            playerA += e.motion.xrel * ANGLESPEED;
            if (playerA > 3.1415) playerA -= 2*3.1415;
            if (playerA < -3.1415) playerA += 2*3.1415;
        }
    }

    // Scan keyboard.
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
        double dx = cos(playerA - 1.57) * MOVESPEED;
        double dy = sin(playerA - 1.57) * MOVESPEED;
        playerMove(dx, dy);
    }
    if (currentKeyStates[SDL_SCANCODE_D]) {
        double dx = cos(playerA + 1.57) * MOVESPEED;
        double dy = sin(playerA + 1.57) * MOVESPEED;
        playerMove(dx, dy);
    }

    return false;
}


void drawScreen(SDL_Renderer* renderer, SDL_Texture* screen) {
    // Each row has SCREEN_WIDTH * 4 bytes, one byte each for R, G, B, and A.
    void *pixels = NULL;
    int pitch;
    SDL_LockTexture(screen, NULL, &pixels, &pitch);

    // Draw the floor and ceiling.
    for (int y = 0; y < SCREEN_HEIGHT / 2; y++) {
        // Set gradient color depending on how far from the edge of the screen
        // we draw.
        Uint8 color = 56.0 * exp(-1.5 * (float)y / (float)SCREEN_HEIGHT);
        Uint32 ceilColor  = ((((((color) << 8) + color) << 8) + color) << 8) + 255;
        Uint32 floorColor  = ((((((color) << 8) + color) << 8) + color/3) << 8) + 255;

        // Set texture pixels to the gradient colors.
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            *(((Uint32*) pixels) + SCREEN_WIDTH * y + x) = ceilColor;
            *(((Uint32*) pixels) + SCREEN_WIDTH * (SCREEN_HEIGHT - 1) - y * SCREEN_WIDTH + x) = floorColor;
        }
    }

    // Draw the wall.
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        // Sweep ray in front of player.
        double rayAngle = atan((x - SCREEN_WIDTH / 2.0) / (SCREEN_WIDTH / 2.0));
        double castAngle = playerA + rayAngle;
        ray rayReturn = distanceToWall(playerPos, castAngle);

        // Modify the distance to counter fisheye effect.
        double distance =  cos(rayAngle) * rayReturn.distance;

        // Calculate what color the wall should be based on distance.
        Uint8 color = 255.0 * exp(-distance/FOG);

        // Calculate apparent height of wall to the player.
        int wallHeight = DIST_TO_PROJ_PLANE / distance * SCREEN_HEIGHT;
        int actualWallHeight = wallHeight;
        wallHeight = (wallHeight > SCREEN_HEIGHT) ? SCREEN_HEIGHT : wallHeight;
        int wallY = (int) ((SCREEN_HEIGHT - wallHeight) / 2);

        // Get where we hit the wall and what type of wall we hit.
        double wallcoord = rayReturn.coord;
        int typeOfWall = rayReturn.type;

        // Select proper texture to draw.
        SDL_Surface* wall_img_surf;
        if (typeOfWall == 1) wall_img_surf = wall_img_surf1;
        if (typeOfWall == 2) wall_img_surf = wall_img_surf2;
        if (typeOfWall == 3) wall_img_surf = wall_img_surf3;
        if (typeOfWall == 4) wall_img_surf = wall_img_surf4;
        if (typeOfWall == 5) wall_img_surf = wall_img_surf5;
        if (typeOfWall == 6) wall_img_surf = wall_img_surf6;
        if (typeOfWall == 7) wall_img_surf = wall_img_surf7;
        if (typeOfWall == 8) wall_img_surf = wall_img_surf8;
        if (typeOfWall == 9) wall_img_surf = wall_img_surf9;

        // Draw the wall.
        for (int y = wallY; y < wallHeight+wallY; y++) {
            // Get where on the texture we are.
            int texture_x = 0;
            int texture_y = 0;
            if (distance > DIST_TO_PROJ_PLANE) {
                texture_x = wallcoord * 127; 
                texture_y = (y - wallY) / (float)wallHeight * 127;
            }
            else {
                texture_x = wallcoord * 127; 
                texture_y = ((actualWallHeight - wallHeight) / 2.0 + y) / (float)actualWallHeight * 127;
            }


            // Read the color on the texture.
            Uint8 r = *(((Uint8*) (wall_img_surf->pixels)) + texture_y * 3*128 + 3*texture_x + 0);
            Uint8 g = *(((Uint8*) (wall_img_surf->pixels)) + texture_y * 3*128 + 3*texture_x + 1);
            Uint8 b = *(((Uint8*) (wall_img_surf->pixels)) + texture_y * 3*128 + 3*texture_x + 2);
            r *= color / 255.0;
            g *= color / 255.0;
            b *= color / 255.0;


            Uint32 wallColor = 0;
            wallColor = (((((b << 8) + g) << 8) + r) << 8) + 255;
            *(((Uint32*) pixels) + SCREEN_WIDTH * y + x) = wallColor;
        }
    }




    // Actually draw the applied changes.
    SDL_UnlockTexture(screen);
    SDL_RenderCopy(renderer, screen, NULL, NULL);

    // Draw enemies.
    drawEnemies(renderer);

    // Draw HUD.
    //SDL_RenderCopy(renderer, hud_texture, NULL, NULL);

    // Present screen.
    SDL_RenderPresent(renderer);
}


/* Moves the player by (dx, dy). */
void playerMove(double dx, double dy) {
    if (!map[(int)round(playerPos.x + dx)][(int)round(playerPos.y)])
        playerPos.x += dx;
    if (!map[(int)round(playerPos.x)][(int)round(playerPos.y + dy)])
        playerPos.y += dy;
}
double veclen(fVector2 vec) {
    return sqrt(vec.x * vec.x + vec.y * vec.y);
}
