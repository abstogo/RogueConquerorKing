#pragma once
#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonpath/json_query.hpp>
#include <jsoncons/json_type_traits_macros.hpp>
#include <jsoncons_ext/csv/csv.hpp>
#include <fstream>
#include "Class.h"
#include "Character.h"
#include "Game.h"

class PurchasableSlot
{
	std::string Name_;
	int Cost_;

public:
	PurchasableSlot(const std::string& Name, const int Cost) : Name_(Name), Cost_(Cost)
	{

	}

	const std::string& Name() const { return Name_; }
	const int Cost() const { return Cost_; }
};
JSONCONS_ALL_GETTER_CTOR_TRAITS_DECL(PurchasableSlot, Name, Cost)

class BaseType
{
	std::string Name_;
	std::string Buildable_;
	std::vector<std::string> Core_;
	std::vector<std::string> Optional_;
	std::vector<PurchasableSlot> Purchasable_;

	
public:
	BaseType(const std::string& Name, const std::string& Buildable, const std::vector<std::string>& Core, const std::vector<std::string>& Optional,
		std::vector<PurchasableSlot>& Purchasable) :
		Name_(Name), Buildable_(Buildable), Core_(Core), Optional_(Optional), Purchasable_(Purchasable)
	{}

	const std::string Name() const { return Name_; }
	const std::string Buildable() const { return Buildable_; }

	const std::vector<std::string> Core() const { return Core_; }
	const std::vector<std::string> Optional() const { return Optional_; }

	const std::vector<PurchasableSlot> Purchasable() const { return Purchasable_; }
};
JSONCONS_ALL_GETTER_CTOR_TRAITS_DECL(BaseType, Name, Buildable, Core, Optional, Purchasable)


class BaseInfoSet
{
	std::vector<BaseType> BaseTypes_;

public:
	BaseInfoSet(const std::vector<BaseType>& BaseTypes) : BaseTypes_(BaseTypes)
	{}

	const std::vector<BaseType>& BaseTypes() const { return BaseTypes_; }
};
JSONCONS_ALL_GETTER_CTOR_TRAITS_DECL(BaseInfoSet, BaseTypes)

enum BaseTypes
{
	BASE_CAMP = 0,
	BASE_SETTLEMENT = 1,
	BASE_STRONGHOLD = 2
};

class BaseManager
{
	// Party Base Mechanics
	
	// It is quite possible for a party to control more than one base of operations. Indeed at later levels this is entirely expected.
	// There are 3 particular kinds of Base:
	// 1) Camps (Temporary bases created by a Party)
	// 2) Settlement Bases (Towns and Cities, in which the Party has a base of operations such as an Inn or a house etc)
	// 3) Strongholds (Structures constructed or conquered by a Party outside an existing Settlement)
	// These differ largely by capability, and as a result are detailed in a JSON file.
	// Note that while some capabilities are limited by type (eg Camps cannot provide true Bed Rest, do not count as Return to Civilisation for XP purposes and do not allow Living Costs expenditure),
	// other capabilities must be unlocked in some fashion (usually by paying for it such as Storage or Workshops)
	// One final element is that Camps will eventually form the basis of the player construction system, which means we probably want some kind of map representation of the Camp besides the region map icon.

	// NB - This is not the same thing as Settlements. Settlements are a whole other issue and will be dealt with in SettlementManager - although a Settlement can be the site of a Base. The Settlement Interface will handle that transition.

	// Loaded data
	BaseInfoSet baseInfoSet;
	
	// Core Base info
	std::vector<int> baseType; // what type of Base is this? Indexes into the BaseInfoSet
	std::vector<int> controllingPartyID; // what party controls this Base

	// Resident Characters
	// Characters can be either in a Base or in a Party. (We may add the idea of an Army later too.) These work the same way as in PartyManager
	std::vector<std::vector<int>> playerCharacters; // playable characters resident, referenced to CharacterManager
	std::vector<std::vector<int>> henchmen; // non-playable characters resident, referenced to CharacterManager
	std::vector<std::vector<int>> animals; // animal henches etc, referenced to MobManager
	// std::vector<std::vector<int>> mercenaries; // non-playable characters, only present in Region parties and Camps, referenced to TroopManager and gets instantiated into MobManager

	// Inventory
	// Bases do not have a carry capacity. (However we may end up having some kind of mechanics for large-scale storage, secure storage etc.)

	std::vector<std::vector<std::pair<int, int>>> baseInventory; // inventory is set up as pairs of IDs and counts (eg we have 25 flasks of military oil)
	
	std::vector<int> totalSuppliesFood; // in mandays. This is only directly relevant for Camps, since other areas use Living Costs.
	// std::vector<int> totalSuppliesWater; // add this later

	int nextBaseID;

	int shellGenerate(); // just creates all the internal structures for the next party, with nothing added in

	std::vector<int> baseXPos;
	std::vector<int> baseYPos;

	std::map<std::string, int> reverseBaseTypeDictionary;

public:
	BaseManager(BaseInfoSet& _bis) : baseInfoSet(_bis)
	{}
	~BaseManager()
	{}

	// Manager Factory
	static BaseManager* LoadBaseData();

	int GenerateCampAtLocation(int partyID, int basePosX, int basePosY);

	void AddPlayerCharacter(int baseID, int characterID);
	void AddHenchman(int baseID, int characterID);
	void AddAnimal(int baseID, int mobID);

	void RemoveCharacter(int baseID, int entityID);
	void RemoveAnimal(int baseID, int entityID);

	// Accessors
	std::vector<int>& getPlayerCharacters(int partyID) { return playerCharacters[partyID]; }
	std::vector<int>& getHenchmen(int partyID) { return henchmen[partyID]; }
	std::vector<int>& getAnimals(int partyID) { return animals[partyID]; }
	
	int getTotalSuppliesFood(int partyID) { return totalSuppliesFood[partyID]; }

	std::vector<std::pair<int, int>>& getPartyInventory(int partyID) { return baseInventory[partyID]; }

	int GetBaseX(int baseID) { return baseXPos[baseID]; }
	int GetBaseY(int baseID) { return baseYPos[baseID]; }
	void SetBaseX(int baseID, int xpos) { baseXPos[baseID] = xpos; }
	void SetBaseY(int baseID, int ypos) { baseYPos[baseID] = ypos; }
	
	void DumpBase(int partyID);

	void DebugLog(std::string message);

	bool TurnHandler(int entityID, double time);
	// no TargetHandler - there isn't a situation for that
	bool TimeHandler(int rounds, int turns, int hours, int days, int weeks, int months);
};