#pragma once
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stack>
#include "libtcod.hpp"
#include <vector>
#include <list>
#include <queue>
#include <string>
#include "OutputLog.h"

// sample screen size
#define SAMPLE_SCREEN_WIDTH 46
#define SAMPLE_SCREEN_HEIGHT 20

#define OUTDOOR_MAP_WIDTH (SAMPLE_SCREEN_WIDTH / 2)
#define OUTDOOR_MAP_HEIGHT (SAMPLE_SCREEN_HEIGHT / 2)

#define INDOOR_MAP_WIDTH SAMPLE_SCREEN_WIDTH
#define INDOOR_MAP_HEIGHT SAMPLE_SCREEN_HEIGHT


enum ContentTypes
{
	CONTENT_NONE = 0,
	CONTENT_TREE,
	CONTENT_ROCKS,
	CONTENT_WALL,
	CONTENT_TRANSITION_STAIRS, // indoor-indoor transition
	CONTENT_TRANSITION_DOOR, // outdoor-indoor-outdoor transition
	CONTENT_TRANSITION_ZONE, // outdoor-outdoor transition
	CONTENT_MAX
};

enum MapTypes
{
	MAP_DUNGEON = 0,	// 1 square = 5ft
	MAP_WILDERNESS,		// 1 hex = 5yd
	MAP_REGION,			// 1 hex = 6 miles
	MAP_MAX
};

enum Hex_Movement
{
	HEX_RIGHTUP = 0,
	HEX_RIGHT,
	HEX_RIGHTDOWN,
	HEX_LEFTDOWN,
	HEX_LEFT,
	HEX_LEFTUP,
	HEX_MAX
};

static int move_map_odd[6][2] =
{
	{1,-1}, // rightup
	{1,0},	// right
	{1,1},	// rightdown
	{0,1},	// leftdown
	{-1,0},	// left
	{0,-1}	// leftup
};

static int move_map_even[6][2] =
{
	{ 0,-1 }, // rightup
	{ 1,0 },	// right
	{ 0,1 },	// rightdown
	{ -1,1 },	// leftdown
	{ -1,0 },	// left
	{ -1,-1 }	// leftup
};

// now handle the keyboard input, which is a little different for hex maps (89UOJK)
static int move_map_ortho[8][2] =
{
	{0 ,-1},    // up
	{1 , 0},	// right
	{0 , 1},	// down
	{-1, 0}, 	// left
	{-1, -1}, 	// up-left
	{-1, 1}, 	// down-left
	{1, 1}, 	// down-right
	{1, -1} 	// up-right
};


enum Ortho_Movement
{
	ORTHO_UP = 0,
	ORTHO_RIGHT,
	ORTHO_DOWN,
	ORTHO_LEFT,
	ORTHO_UPLEFT,
	ORTHO_DOWNLEFT,
	ORTHO_DOWNRIGHT,
	ORTHO_UPRIGHT
};

// ARRAY OF STRUCTS OF ARRAYS MOFO
// (I'm breaking with the plan a little)
struct Map
{
	TCODMap* map = NULL;

	bool outdoor = false;
	int mapType;

	int width;
	int height;

	// vectors of every cell
	
	std::vector<std::stack<int>> items;
	std::vector<int> content;
	std::vector<int> transition;

	// vectors of lists of things
	
	std::vector<int> reverse_transition_mapindex;
	std::vector<int> reverse_transition_xpos;
	std::vector<int> reverse_transition_ypos;

	std::vector<int> mobs;						// monsters and non-fully-fleshed NPCS, ref to MobManager, position is stored there
	std::vector<int> characters;				// characters uncontrolled by the player, ref to CharacterManager, position is stored there

	int getCharacterAt(int x, int y);
	void setCharacter(int x, int y, int characterID);
	int removeCharacter(int characterID);

	int getMobAt(int x, int y);
	void setMob(int x, int y, int mobID);
	int removeMob(int mobID);

	void getManagedEntityAt(int x, int y, int& manager, int& entityID);
	
	int getContent(int x, int y)
	{
		return(content[y * width + x]);
	}

	void setContent(int x, int y, int c)
	{
		content[y * width + x] = c;
	}

	void addItem(int x, int y, int item)
	{
		items[y * width + x].push(item);
	}

	std::stack<int>* getItems(int x, int y)
	{
		return &items[y * width + x];
	}

	void setTransition(int x, int y, int target)
	{
		transition[y * width + x] = target;
	}

	int getTransition(int x, int y)
	{
		return transition[y * width + x];
	}
};


class MapManager : public ITCODPathCallback
{
	std::vector<Map*> maps;
	
public:
	MapManager();
	~MapManager();

	Map* getMap(int index);

	//builds a new map, adds it to the map store, returns the id (distinct for indoor and outdoor maps)
	int createMap(bool outdoor);
	
	// builds an empty map of the specified type (useful for open playfields and spawners)
	int buildEmptyMap(int width, int height, int type);

	// builds an outdoor map from an array of strings (could be loaded from a file etc)
	int buildMapFromText(const char* hmap[], bool outdoor);
	Map* mapFromText(const char* hmap[], bool outdoor);

	// this manager handles the main rendering, since it controls the map status & context (hex/square, lighting etc)
	void renderMap(TCODConsole* sampleConsole, int index);

	// mobile element (player, monster) are rendered through here too
    void renderAtPosition(TCODConsole* sampleConsole, int mapIndex, int x, int y, char c, TCODColor foreground = TCODColor::lighterGrey);

	// connects one map to another at the specified point
	void connectMaps(int map1, int map2, int x1, int y1, int x2, int y2);

	// factory
	static MapManager* LoadMaps();

	// item management
	void AddItem(int mapID, int x, int y, std::string item);
	void AddItem(int mapID, int x, int y, int itemID);
	int TakeTopItem(int mapID, int x, int y);
	std::string ItemDesc(int mapID, int x, int y);

	// movement multiplier (based on map level - higher level maps have larger sized cells, therefore take longer to move through)
	double getMovementTime(int mapID, double speed);

	// hex mobility functions
	float getWalkCost(int xFrom, int yFrom, int xTo, int yTo, void* userData) const;
	void shift(int mapID, int& new_x, int& new_y, int player_x, int player_y, int move_value);

	// bounds checking
	bool isOutOfBounds(int mapID, int x, int y);

	bool isInFOV(int sourceManager, int sourceID, int targetManager, int targetID, int range = 0);
	std::vector<int> filterByFOV(int sourceManager, int sourceID, int targetManager, std::vector<int> targets, int range = 0);

	// handlers
	bool TurnHandler(int entityID, double time);
	bool TargetHandler(int entityID, int returnCode);
	bool TimeHandler(int rounds, int turns, int hours, int days, int weeks, int months);
	
	void DebugLog(std::string message);
};
