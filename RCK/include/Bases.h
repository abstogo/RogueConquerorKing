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

class BaseTag
{
	std::string Tag_;
	std::string Type_;
	std::string Indicator_;
	std::string MenuText_;
	std::vector<std::string> Requires_;

public:
	BaseTag(const std::string& Tag, const std::string& Type, const std::string& Indicator, const std::string& MenuText, const std::vector<std::string>& Requires) :
	Tag_(Tag), Type_(Type), Indicator_(Indicator), MenuText_(MenuText), Requires_(Requires)
	{
		
	}

	const std::string& Tag() const { return Tag_; }
	const std::string& Type() const { return Type_; }
	const std::string& Indicator() const { return Indicator_; }
	const std::string& MenuText() const { return MenuText_; }
	const std::vector<std::string>& Requires() const { return Requires_; }
};
JSONCONS_ALL_GETTER_CTOR_TRAITS_DECL(BaseTag, Tag, Type, Indicator, MenuText, Requires)

class BaseType
{
	std::string Name_;
	std::string Buildable_;
	std::vector<std::string> Upkeep_;
	std::vector<std::string> Core_;
	std::vector<std::string> Options_;
	std::vector<PurchasableSlot> Purchasable_;

	
public:
	BaseType(const std::string& Name, const std::string& Buildable, const std::vector<std::string>& Upkeep, const std::vector<std::string>& Core, const std::vector<std::string>& Options,
		std::vector<PurchasableSlot>& Purchasable) :
		Name_(Name), Buildable_(Buildable), Upkeep_(Upkeep), Core_(Core), Options_(Options), Purchasable_(Purchasable)
	{}

	const std::string Name() const { return Name_; }
	const std::string Buildable() const { return Buildable_; }
	const std::vector<std::string> Upkeep() const { return Upkeep_; }
	const std::vector<std::string> Core() const { return Core_; }
	const std::vector<std::string> Options() const { return Options_; }

	const std::vector<PurchasableSlot> Purchasable() const { return Purchasable_; }
};
JSONCONS_ALL_GETTER_CTOR_TRAITS_DECL(BaseType, Name, Buildable, Upkeep, Core, Options, Purchasable)


class BaseInfoSet
{
	std::vector<BaseType> BaseTypes_;
	std::vector<BaseTag> Tags_;

public:
	BaseInfoSet(const std::vector<BaseType>& BaseTypes, const std::vector<BaseTag>& Tags) : BaseTypes_(BaseTypes), Tags_(Tags)
	{}

	const std::vector<BaseType>& BaseTypes() const { return BaseTypes_; }
	const std::vector<BaseTag>& Tags() const { return Tags_; }
};
JSONCONS_ALL_GETTER_CTOR_TRAITS_DECL(BaseInfoSet, BaseTypes, Tags)

// screens are in fact handled in the tags. Each one has 3 sections: Base, Party and Manipulator.

enum BaseMenuPanes
{
	PANE_BASE_CHARACTERS = 0,
	PANE_PARTY_CHARACTERS = 1,
	PANE_CHARACTER_OPTIONS = 2,
	PANE_PARTY_INVENTORY = 3,
	PANE_BASE_INVENTORY = 4,
	PANE_PURCHASES = 5
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
	//std::vector<int> controllingFaction; // to be used when we have Factions
	
	// Base Party
	// When the Base is created it takes on the current Party. Whenever a Party returns to the base it merges with the Base Party.
	// When the Base is packed up, the Base Party becomes the current Party.
	// In future we'll have more complex controls for managing characters between parties (sub-groups etc) to make big camps easier to manage,
	// but for now this will do

	std::vector<int> basePartyID;

	// However, we need to store which characters will be in the split-off Party *before* they leave.
	// The chosen characters are not yet a Party and should not be ticked until they leave.
	// They should also be maintained where possible between trips to minimise clicks
	// But note there is only eveer one of these as it's pure interface
	std::vector<int> playerCharacters; 
	std::vector<int> henchmen; 
	std::vector<int> animals; 

	std::vector<std::pair<int, int>> inventory; // inventory is set up as pairs of IDs and counts (eg we have 25 flasks of military oil)
	
	int nextBaseID;

	int shellGenerate(); // just creates all the internal structures for the next party, with nothing added in

	std::vector<int> baseXPos;
	std::vector<int> baseYPos;

	std::map<std::string, int> reverseBaseTypeDictionary;
	std::map<std::string, int> reverseBaseTagDictionary;

	int controlPane = 0;
	int menuPosition = 0;

	std::vector<std::vector<int>> pcSelectedAction;
	std::vector<std::vector<std::map<std::string, int>>> pcActiveTags;

	std::vector<int> GetBasePartyCharacters(int baseID);
	std::vector<int> GetOutPartyCharacters(int baseID);
	
public:
	BaseManager(BaseInfoSet& _bis) : baseInfoSet(_bis)
	{}
	~BaseManager()
	{}

	// Manager Factory
	static BaseManager* LoadBaseData();

	int GenerateCampAtLocation(int partyID, int basePosX, int basePosY);

	void MergeInParty(int baseID, int partyID);
	int SpawnOutParty(int baseID);

	// GUI output control functions - interface is same as Party!

	void AddPlayerCharacter(int characterID);
	void AddHenchman(int characterID);
	void AddAnimal(int mobID);
	void RemoveCharacter(int entityID);
	void RemoveAnimal(int entityID);

	// Accessors
	std::vector<int>& getPlayerCharacters() { return playerCharacters; }
	std::vector<int>& getHenchmen() { return henchmen; }
	std::vector<int>& getAnimals() { return animals; }

	bool CharacterCanUseAction(int baseID, int characterID, int tag);
	std::vector<BaseTag> GetCharacterActionList(int baseID, int charID);
	int GetSelectedCharacter(int baseID);
	
	int GetBaseX(int baseID) { return baseXPos[baseID]; }
	int GetBaseY(int baseID) { return baseYPos[baseID]; }
	void SetBaseX(int baseID, int xpos) { baseXPos[baseID] = xpos; }
	void SetBaseY(int baseID, int ypos) { baseYPos[baseID] = ypos; }

	void ControlCommand(TCOD_key_t* key,int baseID);
	
	void RenderBaseMenu(int xpos, int ypos);
	void RenderBaseMenu(int baseID);
	
	void DumpBase(int partyID);

	void DebugLog(std::string message);

	bool TurnHandler(int entityID, double time);
	// no TargetHandler - there isn't a situation for that
	bool TimeHandler(int rounds, int turns, int hours, int days, int weeks, int months);
};