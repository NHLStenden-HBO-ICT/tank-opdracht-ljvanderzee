#include "precomp.h" // include (only) this in every .cpp file

constexpr auto num_tanks_blue = 2048;
constexpr auto num_tanks_red = 2048;

constexpr auto tank_max_health = 1000;
constexpr auto rocket_hit_value = 60;
constexpr auto particle_beam_hit_value = 50;

constexpr auto tank_max_speed = 1.0;

constexpr auto health_bar_width = 70;

constexpr auto max_frames = 2000;

//Cell size for the spatial partitioning grid
constexpr auto cell_size = 12;

//Global performance timer
constexpr auto REF_PERFORMANCE = 138835; //UPDATE THIS WITH YOUR REFERENCE PERFORMANCE (see console after 2k frames)
static timer perf_timer;
static float duration;

//Load sprite files and initialize sprites
static Surface* tank_red_img = new Surface("assets/Tank_Proj2.png");
static Surface* tank_blue_img = new Surface("assets/Tank_Blue_Proj2.png");
static Surface* rocket_red_img = new Surface("assets/Rocket_Proj2.png");
static Surface* rocket_blue_img = new Surface("assets/Rocket_Blue_Proj2.png");
static Surface* particle_beam_img = new Surface("assets/Particle_Beam.png");
static Surface* smoke_img = new Surface("assets/Smoke.png");
static Surface* explosion_img = new Surface("assets/Explosion.png");

static Sprite tank_red(tank_red_img, 12);
static Sprite tank_blue(tank_blue_img, 12);
static Sprite rocket_red(rocket_red_img, 12);
static Sprite rocket_blue(rocket_blue_img, 12);
static Sprite smoke(smoke_img, 4);
static Sprite explosion(explosion_img, 9);
static Sprite particle_beam_sprite(particle_beam_img, 3);

const static vec2 tank_size(7, 9);
const static vec2 rocket_size(6, 6);

const static float tank_radius = 3.f;
const static float rocket_radius = 5.f;

// -----------------------------------------------------------
// Initialize the simulation state
// This function does not count for the performance multiplier
// (Feel free to optimize anyway though ;) )
// -----------------------------------------------------------
void Game::init()
{
    frame_count_font = new Font("assets/digital_small.png", "ABCDEFGHIJKLMNOPQRSTUVWXYZ:?!=-0123456789.");

    tanks.reserve(num_tanks_blue + num_tanks_red);

    uint max_rows = 24;

    float start_blue_x = tank_size.x + 40.0f;
    float start_blue_y = tank_size.y + 30.0f;

    float start_red_x = 1088.0f;
    float start_red_y = tank_size.y + 30.0f;

    float spacing = 7.5f;

    //Init grid for spatial partitioning
    gamegrid = make_unique<Grid>((SCRWIDTH - (HEALTHBAR_OFFSET *2)), SCRHEIGHT, cell_size);

    //Spawn blue tanks
    for (int i = 0; i < num_tanks_blue; i++)
    {
        vec2 position{ start_blue_x + ((i % max_rows) * spacing), start_blue_y + ((i / max_rows) * spacing) };
        tanks.push_back(Tank(position.x, position.y, BLUE, &tank_blue, &smoke, 1100.f, position.y + 16, tank_radius, tank_max_health, tank_max_speed));
        gamegrid->addTank(&tanks.back());
    }
    //Spawn red tanks
    for (int i = 0; i < num_tanks_red; i++)
    {
        vec2 position{ start_red_x + ((i % max_rows) * spacing), start_red_y + ((i / max_rows) * spacing) };
        tanks.push_back(Tank(position.x, position.y, RED, &tank_red, &smoke, 100.f, position.y + 16, tank_radius, tank_max_health, tank_max_speed));
        gamegrid->addTank(&tanks.back());
    }

    particle_beams.push_back(Particle_beam(vec2(590, 327), vec2(100, 50), &particle_beam_sprite, particle_beam_hit_value));
    particle_beams.push_back(Particle_beam(vec2(64, 64), vec2(100, 50), &particle_beam_sprite, particle_beam_hit_value));
    particle_beams.push_back(Particle_beam(vec2(1200, 600), vec2(100, 50), &particle_beam_sprite, particle_beam_hit_value));
}

// -----------------------------------------------------------
// Close down application
// -----------------------------------------------------------
void Game::shutdown()
{
}

// -----------------------------------------------------------
// Iterates through all tanks and returns the closest enemy tank for the given tank
// -----------------------------------------------------------
Tank& Game::find_closest_enemy(Tank& current_tank)
{
    float closest_distance = numeric_limits<float>::infinity();
    int closest_index = 0;

    for (int i = 0; i < tanks.size(); i++)
    {
        if (tanks.at(i).allignment != current_tank.allignment && tanks.at(i).active)
        {
            float sqr_dist = fabsf((tanks.at(i).get_position() - current_tank.get_position()).sqr_length());
            if (sqr_dist < closest_distance)
            {
                closest_distance = sqr_dist;
                closest_index = i;
            }
        }
    }

    return tanks.at(closest_index);
}

//Checks if a point lies on the left of an arbitrary angled line
bool Tmpl8::Game::left_of_line(vec2 line_start, vec2 line_end, vec2 point)
{
    return ((line_end.x - line_start.x) * (point.y - line_start.y) - (line_end.y - line_start.y) * (point.x - line_start.x)) < 0;
}

//This function contains the original collision detection code
void Game::tankCollision(Tank* tank, vector<Tank*>& tanksToCheck, int index)
{
    for (int i = index; i < tanksToCheck.size(); i++)
    {
        vec2 dir = tank->get_position() - tanksToCheck[i]->get_position();
        float dir_squared_len = dir.sqr_length();

        float col_squared_len = (tank->get_collision_radius() + tanksToCheck[i]->get_collision_radius());
        col_squared_len *= col_squared_len;

        if (dir_squared_len < col_squared_len)
        {
            tank->push(dir.normalized(), 1.f);
        }
    }
};

void Game::getActiveTanksPositions(const vector<Tank>& tanks)
{
    for (Tank tank : tanks)
    {
        if (tank.active)
        {
            forcefield_hull.push_back(tank.position);
        }
    }
};


// -----------------------------------------------------------
// Update the game state:
// Move all objects
// Update sprite frames
// Collision detection
// Targeting etc..
// -----------------------------------------------------------
void Game::update(float deltaTime)
{
    //Calculate the route to the destination for each tank using BFS
    //Initializing routes here so it gets counted for performance..
    if (frame_count == 0)
    {
        for (Tank& t : tanks)
        {
            t.set_route(background_terrain.get_route(t, t.target));
        }
    }

    //Loops through the cells in the gamegrid and checks the tanks in that cell for collision
    //Also check the neigbouring cells (Top, Top-Left, Left, Bottom-Left) of the current cell
    //This patterns ensures that cells are not checked against eachother more than once
    //Replaces the original code which was n*n with a grid based implementation which runs in O(n * m)
    for (int i = 0; i < gamegrid->cells.size(); i++)
    {
        int x = i % gamegrid->xCells;
        int y = i / gamegrid->xCells;

        Cell& cell = gamegrid->cells[i];

        //Loop through the tanks in the current cell
        for (int j = 0; j < cell.tanks.size(); j++)
        {
            Tank* tank = cell.tanks[j];
            if (tank->active)
            {
                //Check the tanks in the own cell for collision
                tankCollision(tank, cell.tanks, j + 1);
                if (x > 0)
                {
                    //Left cell
                    tankCollision(tank, gamegrid->getCell(x - 1, y)->tanks, 0);
                    if (y > 0)
                    {
                        //Top left cell
                        tankCollision(tank, gamegrid->getCell(x - 1, y - 1)->tanks, 0);
                    }
                    if (y < gamegrid->yCells - 1)
                    {
                        //Bottom lef cell
                        tankCollision(tank, gamegrid->getCell(x - 1, y + 1)->tanks, 0);
                    }
                }
                //Top cell
                if (y > 0)
                {
                    tankCollision(tank, gamegrid->getCell(x, y - 1)->tanks, 0);
                }
            }
        }
    }

    //Update tanks
    for (Tank& tank : tanks)
    {
        if (tank.active)
        {
            //Move tanks according to speed and nudges (see above) also reload
            tank.tick(background_terrain);

            //Get and store the cell of the current tank in checkCell
            Cell* checkCell = gamegrid->getCell(tank.position);
            //If checkCell does not match the cell the tank is currently in
            if (checkCell != tank.currentCell)
            {
                //Move the tank to checkCell
                gamegrid->removeTankFromCell(&tank);
                gamegrid->addTank(&tank, checkCell);
            }

            //Shoot at closest target if reloaded
            if (tank.rocket_reloaded())
            {
                Tank& target = find_closest_enemy(tank);

                rockets.push_back(Rocket(tank.position, (target.get_position() - tank.position).normalized() * 3, rocket_radius, tank.allignment, ((tank.allignment == RED) ? &rocket_red : &rocket_blue)));

                tank.reload_rocket();
            }
        }
    }

    //Update smoke plumes
    for (Smoke& smoke : smokes)
    {
        smoke.tick();
    }

    //Calculate "forcefield" around active tanks
    forcefield_hull.clear();

    getActiveTanksPositions(tanks);

    forcefield_hull = convexHullTest(forcefield_hull);

    //Original convex hull calculation code
    //Find first active tank (this loop is a bit disgusting, fix?)
    //int first_active = 0;
    //for (Tank& tank : tanks)
    //{
    //    if (tank.active)
    //    {
    //        break;
    //    }
    //    first_active++;
    //}
    //vec2 point_on_hull = tanks.at(first_active).position;
    ////Find left most tank position
    //for (Tank& tank : tanks)
    //{
    //    if (tank.active)
    //    {
    //        if (tank.position.x <= point_on_hull.x)
    //        {
    //            point_on_hull = tank.position;
    //        }
    //    }
    //}

    ////Calculate convex hull for 'rocket barrier'
    //for (Tank& tank : tanks)    
    //{
    //    if (tank.active)
    //    {
    //        forcefield_hull.push_back(point_on_hull);
    //        vec2 endpoint = tanks.at(first_active).position;

    //        for (Tank& tank : tanks)
    //        {
    //            if (tank.active)
    //            {
    //                if ((endpoint == point_on_hull) || left_of_line(point_on_hull, endpoint, tank.position))
    //                {
    //                    endpoint = tank.position;
    //                }
    //            }
    //        }
    //        point_on_hull = endpoint;

    //        if (endpoint == forcefield_hull.at(0))
    //        {
    //            break;
    //        }
    //    }
    //}

    //Update rockets
    for (Rocket& rocket : rockets)
    {
        rocket.tick();

        //Check if rocket collides with enemy tank, spawn explosion, and if tank is destroyed spawn a smoke plume
        for (Tank& tank : tanks)
        {
            if (tank.active && (tank.allignment != rocket.allignment) && rocket.intersects(tank.position, tank.collision_radius))
            {
                explosions.push_back(Explosion(&explosion, tank.position));

                if (tank.hit(rocket_hit_value))
                {
                    smokes.push_back(Smoke(smoke, tank.position - vec2(7, 24)));
                }

                rocket.active = false;
                break;
            }
        }
    }

    //Disable rockets if they collide with the "forcefield"
    //Hint: A point to convex hull intersection test might be better here? :) (Disable if outside)
    for (Rocket& rocket : rockets)
    {
        if (rocket.active)
        {
            for (size_t i = 0; i < forcefield_hull.size(); i++)
            {
                if (circle_segment_intersect(forcefield_hull.at(i), forcefield_hull.at((i + 1) % forcefield_hull.size()), rocket.position, rocket.collision_radius))
                {
                    explosions.push_back(Explosion(&explosion, rocket.position));
                    rocket.active = false;
                }
            }
        }
    }



    //Remove exploded rockets with remove erase idiom
    rockets.erase(std::remove_if(rockets.begin(), rockets.end(), [](const Rocket& rocket) { return !rocket.active; }), rockets.end());

    //Update particle beams
    for (Particle_beam& particle_beam : particle_beams)
    {
        particle_beam.tick(tanks);

        //Damage all tanks within the damage window of the beam (the window is an axis-aligned bounding box)
        for (Tank& tank : tanks)
        {
            if (tank.active && particle_beam.rectangle.intersects_circle(tank.get_position(), tank.get_collision_radius()))
            {
                if (tank.hit(particle_beam.damage))
                {
                    smokes.push_back(Smoke(smoke, tank.position - vec2(0, 48)));
                }
            }
        }
    }

    //Update explosion sprites and remove when done with remove erase idiom
    for (Explosion& explosion : explosions)
    {
        explosion.tick();
    }

    explosions.erase(std::remove_if(explosions.begin(), explosions.end(), [](const Explosion& explosion) { return explosion.done(); }), explosions.end());
}

// -----------------------------------------------------------
// Draw all sprites to the screen
// (It is not recommended to multi-thread this function)
// -----------------------------------------------------------
void Game::draw()
{
    // clear the graphics window
    screen->clear(0);

    //Draw background
    background_terrain.draw(screen);

    //Draw sprites
    for (int i = 0; i < num_tanks_blue + num_tanks_red; i++)
    {
        tanks.at(i).draw(screen);

        vec2 tank_pos = tanks.at(i).get_position();
    }

    for (Rocket& rocket : rockets)
    {
        rocket.draw(screen);
    }

    for (Smoke& smoke : smokes)
    {
        smoke.draw(screen);
    }

    for (Particle_beam& particle_beam : particle_beams)
    {
        particle_beam.draw(screen);
    }

    for (Explosion& explosion : explosions)
    {
        explosion.draw(screen);
    }

    //Draw forcefield (mostly for debugging, its kinda ugly..)
    for (size_t i = 0; i < forcefield_hull.size(); i++)
    {
        vec2 line_start = forcefield_hull.at(i);
        vec2 line_end = forcefield_hull.at((i + 1) % forcefield_hull.size());
        line_start.x += HEALTHBAR_OFFSET;
        line_end.x += HEALTHBAR_OFFSET;
        screen->line(line_start, line_end, 0x0000ff);
    }

    //Draw sorted health bars
    for (int t = 0; t < 2; t++)
    {
        const int NUM_TANKS = ((t < 1) ? num_tanks_blue : num_tanks_red);

        const int begin = ((t < 1) ? 0 : num_tanks_blue);
        std::vector<const Tank*> sorted_tanks;
        counting_sort_tanks_health(tanks, sorted_tanks, begin, begin + NUM_TANKS);
        sorted_tanks.erase(std::remove_if(sorted_tanks.begin(), sorted_tanks.end(), [](const Tank* tank) { return !tank->active; }), sorted_tanks.end());

        draw_health_bars(sorted_tanks, t);
    }
}

// -----------------------------------------------------------
// Sort tanks by health value using counting sort
// Replaces the original insertion sort function with a counting sort
// The original code had a worst case O(n*n)
// Counting sort has a worst case O(n+k)
// -----------------------------------------------------------
void Tmpl8::Game::counting_sort_tanks_health(const std::vector<Tank>& original, std::vector<const Tank*>& sorted_tanks, int begin, int end)
{
    const int NUM_TANKS = end - begin;
    int max = 0;
    sorted_tanks.resize(NUM_TANKS);

    //Find the tank with the highest amount of health and assign that value to max
    for (int i = begin; i < (begin + NUM_TANKS); i++)
    {
        const Tank& current_tank = original.at(i);
        if (current_tank.get_health() > max)
        {
            max = current_tank.get_health();
        }
    }

    // Max value is used to initialize a count vector with all zeros
    std::vector<int> count(max + 1);
    
    //Loop through the current tank vector (red or blue) and increase the count of the count vector at the index of the current tank health
    for (int i = begin; i < (begin + NUM_TANKS); i++)
    {
        const Tank& current_tank = original.at(i);

        count.at(current_tank.get_health())++;
    }

    //Store the cummulative count in the count array
    for (int i = 1; i < count.size(); i++)
    {
        count.at(i) += count.at(i - 1);
    }

    //Loop over the tanks in the original vector and place them in the right position in the sorted_tanks vector
    for (int i = (begin + NUM_TANKS -1); i >= begin; i--)
    {
        const Tank& current_tank = original.at(i);

        int countIndex = count.at(current_tank.get_health());

        sorted_tanks.at(countIndex - 1) = &current_tank;

        count.at(current_tank.get_health())--;

    }
    //return count instead of tank vector
}

// -----------------------------------------------------------
// Draw the health bars based on the given tanks health values
// -----------------------------------------------------------
void Tmpl8::Game::draw_health_bars(const std::vector<const Tank*>& sorted_tanks, const int team)
{
    int health_bar_start_x = (team < 1) ? 0 : (SCRWIDTH - HEALTHBAR_OFFSET) - 1;
    int health_bar_end_x = (team < 1) ? health_bar_width : health_bar_start_x + health_bar_width - 1;

    for (int i = 0; i < SCRHEIGHT - 1; i++)
    {
        //Health bars are 1 pixel each
        int health_bar_start_y = i * 1;
        int health_bar_end_y = health_bar_start_y + 1;

        screen->bar(health_bar_start_x, health_bar_start_y, health_bar_end_x, health_bar_end_y, REDMASK);
    }

    //Draw the <SCRHEIGHT> least healthy tank health bars
    int draw_count = std::min(SCRHEIGHT, (int)sorted_tanks.size());
    for (int i = 0; i < draw_count - 1; i++)
    {
        //Health bars are 1 pixel each
        int health_bar_start_y = i * 1;
        int health_bar_end_y = health_bar_start_y + 1;

        float health_fraction = (1 - ((double)sorted_tanks.at(i)->health / (double)tank_max_health));

        if (team == 0) { screen->bar(health_bar_start_x + (int)((double)health_bar_width * health_fraction), health_bar_start_y, health_bar_end_x, health_bar_end_y, GREENMASK); }
        else { screen->bar(health_bar_start_x, health_bar_start_y, health_bar_end_x - (int)((double)health_bar_width * health_fraction), health_bar_end_y, GREENMASK); }
    }
}

// -----------------------------------------------------------
// When we reach max_frames print the duration and speedup multiplier
// Updating REF_PERFORMANCE at the top of this file with the value
// on your machine gives you an idea of the speedup your optimizations give
// -----------------------------------------------------------
void Tmpl8::Game::measure_performance()
{
    char buffer[128];
    if (frame_count >= max_frames)
    {
        if (!lock_update)
        {
            duration = perf_timer.elapsed();
            cout << "Duration was: " << duration << " (Replace REF_PERFORMANCE with this value)" << endl;
            lock_update = true;
        }

        frame_count--;
    }

    if (lock_update)
    {
        screen->bar(420 + HEALTHBAR_OFFSET, 170, 870 + HEALTHBAR_OFFSET, 430, 0x030000);
        int ms = (int)duration % 1000, sec = ((int)duration / 1000) % 60, min = ((int)duration / 60000);
        sprintf(buffer, "%02i:%02i:%03i", min, sec, ms);
        frame_count_font->centre(screen, buffer, 200);
        sprintf(buffer, "SPEEDUP: %4.1f", REF_PERFORMANCE / duration);
        frame_count_font->centre(screen, buffer, 340);
    }
}

// -----------------------------------------------------------
// Main application tick function
// -----------------------------------------------------------
void Game::tick(float deltaTime)
{
    if (!lock_update)
    {
        update(deltaTime);
    }
    draw();

    measure_performance();

    // print something in the graphics window
    //screen->Print("hello world", 2, 2, 0xffffff);

    // print something to the text window
    //cout << "This goes to the console window." << std::endl;

    //Print frame count
    frame_count++;
    string frame_count_string = "FRAME: " + std::to_string(frame_count);
    frame_count_font->print(screen, frame_count_string.c_str(), 350, 580);
}

//TESTCODE FOR CONVEXHULL

vector<vec2> Game::convexHullTest(vector<vec2>& activeTanksPositions)
{
    int sizeOfVector = activeTanksPositions.size();

    //Find lowestLeftTank as a starting point.

    for (int i = 1; i < activeTanksPositions.size() - 1; i++)
    {
        if (activeTanksPositions[i].y < activeTanksPositions[0].y)
        {
            swap(activeTanksPositions[0], activeTanksPositions[i]);
            continue;
        }
        if (activeTanksPositions[i].y == activeTanksPositions[0].y && activeTanksPositions[i].x < activeTanksPositions[0].x)
        {
            swap(activeTanksPositions[0], activeTanksPositions[i]);
        }
    }

    vec2 lowestLeftTank = activeTanksPositions[0];

    heapSort(activeTanksPositions, sizeOfVector, lowestLeftTank);

    int pointer = 2;

    while (pointer < (activeTanksPositions.size() - 1))
    {
        vec2 p0 = lowestLeftTank;
        vec2 p1 = activeTanksPositions[pointer - 1];
        vec2 p2 = activeTanksPositions[pointer];

        if (convex(p0, p1, p2) == 0)
        {
            activeTanksPositions.erase(activeTanksPositions.begin() + pointer - 1);
        }
        else 
        {
            pointer++;
        }
    }

    vector<vec2> st;
    st.push_back(activeTanksPositions[0]);
    st.push_back(activeTanksPositions[1]);

    pointer = 2;

    while (pointer != (activeTanksPositions.size() - 1))
    {
        vec2 p0 = st[st.size() - 2];
        vec2 p1 = st[st.size() - 1];
        vec2 p2 = activeTanksPositions[pointer];

        if (convex(p0, p1, p2) < 0)
        {
            st.push_back(p2);
            pointer++;
        }

        else {
            // The middle point does not lie on the convex hull.
            st.pop_back();
            if (st.size() < 2) {
                st.push_back(p2);
                ++pointer;
            }
        }
    }

    // If three collinear points are found at the end, we
  // remove the middle one.
    vec2 a = st[st.size() - 2];
    vec2 b = st[st.size() - 1];
    vec2 c = st[0];
    if (convex(a, b, c) > 0) {
        st.pop_back();
    }

    return st;
};

//This function assumes that the input vector at index 0 contains the reference point.
void Game::heapSort(vector<vec2>& activeTanksPositions, int sizeOfVector, vec2 origin)
{
    //Build a maxheap
    //This for loop makes sure the leaf nodes are not checked seprately
    for (int i = (sizeOfVector / 2) - 1; i > 0; i--)
    {
        maxHeapify(activeTanksPositions, i, sizeOfVector, origin);
    }

    for (int i = (sizeOfVector - 1); i > 0; i--)
    {
        swap(activeTanksPositions[0], activeTanksPositions[i]);
        maxHeapify(activeTanksPositions, 0, i, origin);
    }
    swap(activeTanksPositions[0], activeTanksPositions[sizeOfVector - 1]);
};

void Game::maxHeapify(vector<vec2>& tanks, int index, int sizeOfVector, vec2 origin)
{
    int largest = index;
    int left = 2 * index + 1;
    int right = 2 * index + 2;

    if (left < sizeOfVector && compareAngles(origin, tanks[left], tanks[largest]))
    {
        largest = left;
    }

    if (right < sizeOfVector && compareAngles(origin, tanks[right], tanks[largest]))
    {
        largest = right;
    }

    if (largest != index)
    {
        swap(tanks[index], tanks[largest]);
        maxHeapify(tanks, largest, sizeOfVector, origin);
    }
};

//void Game::minHeapify(vector<vec2>& tanks, int index, int sizeOfVector, vec2 origin)
//{
//    int smallest = index;
//    int left = 2 * index + 1;
//    int right = 2 * index + 2;
//
//    if (left < sizeOfVector && compareAngles(origin, tanks[left], tanks[smallest]))
//    {
//        smallest = left;
//    }
//
//    if (right < sizeOfVector && compareAngles(origin, tanks[right], tanks[smallest]))
//    {
//        smallest = right;
//    }
//
//    if (smallest != index)
//    {
//        swap(tanks[index], tanks[smallest]);
//        minHeapify(tanks, smallest, sizeOfVector, origin);
//    }
//};

bool Game::compareAngles(vec2 p0, vec2 p1, vec2 p2)
{
    int c = convex(p0, p1, p2);

    //Returns true when these three point make a counter-clockwise turn
    if (c == 0)
    {
        const int distP1 = std::abs(p1.x - p0.x) + std::abs(p1.y - p0.y);
        const int distP2 = std::abs(p2.x - p0.x) + std::abs(p2.y - p0.y);

        if (distP1 < distP2)
        {
            return true;
        }
        return false;
    }
    return (c > 0) ? true : false;

};

//Returns 0 if the points are collinear, 1 for counter-clockwise turn and -1 for a clockwise turn
int Game::convex(const vec2& p0, const vec2& p1, const vec2& p2)
{
    long long crossProduct = (p1.x - p0.x) * (p2.y - p0.y) - (p1.y - p0.y) * (p2.x - p0.x);

    if (crossProduct == 0)
    {
        return 0;
    }
    return (crossProduct > 0) ? 1 : -1;
};