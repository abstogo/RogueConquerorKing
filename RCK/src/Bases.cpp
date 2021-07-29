#include "Bases.h"

BaseManager* BaseManager::LoadBaseData()
{
	gLog->Log("Base Loader", "Started");
	
	std::string baseFilename = "scripts/bases.json";
	std::ifstream is(baseFilename);

	BaseManager* output = new BaseManager(jsoncons::decode_json<BaseInfoSet>(is));

	is.close();

	gLog->Log("Base Loader", "Decoded " + baseFilename);

	// feed reverse lookup
	for (int i = 0; i < output->baseInfoSet.BaseTypes().size(); i++)
	{
		const BaseType& it = output->baseInfoSet.BaseTypes()[i];
		output->reverseBaseTypeDictionary[it.Name()] = i;
	}
	
	output->nextBaseID = 0;

	gLog->Log("Base Loader", "Completed");
	
	return output;
}

int BaseManager::shellGenerate()
{
	int output = nextBaseID++;

	DebugLog("Generating Empty Base #" + std::to_string(output));

	baseType.push_back(0); // default is camp
	baseType.push_back(-1);
	std::vector<int> pcs;
	playerCharacters.push_back(pcs);
	std::vector<int> henchs;
	henchmen.push_back(henchs);
	std::vector<int> anims;
	animals.push_back(anims);
	std::vector<std::pair<int, int>> inv;
	baseInventory.push_back(inv);
	totalSuppliesFood.push_back(0);
	baseXPos.push_back(-1);
	baseYPos.push_back(-1);

	return output;
}

void BaseManager::AddPlayerCharacter(int baseID, int characterID)
{
	playerCharacters[baseID].push_back(characterID);
	DebugLog("PC (character ID #" + std::to_string(characterID) + ") added to party #" + std::to_string(baseID));
}

void BaseManager::AddHenchman(int baseID, int characterID)
{
	henchmen[baseID].push_back(characterID);
	DebugLog("Henchman (character ID #" + std::to_string(characterID) + ") added to party #" + std::to_string(baseID));
}

void BaseManager::AddAnimal(int baseID, int mobID)
{
	animals[baseID].push_back(mobID);
	DebugLog("Animal (mob ID #" + std::to_string(mobID) + ") added to party #" + std::to_string(baseID));
}

void BaseManager::RemoveCharacter(int baseID, int entityID)
{
	if(std::remove(playerCharacters[baseID].begin(), playerCharacters[baseID].end(), entityID) == playerCharacters[baseID].end())
	{
		// not found in the PC set, check Henches too
		std::remove(henchmen[baseID].begin(), henchmen[baseID].end(), entityID);
	}
}

void BaseManager::RemoveAnimal(int baseID, int entityID)
{
	std::remove(animals[baseID].begin(), animals[baseID].end(), entityID);
}


int BaseManager::GenerateCampAtLocation(int partyID, int basePosX, int basePosY)
{
	DebugLog("Creating a camp controlled by party #" + std::to_string(partyID) + " at (" + std::to_string(basePosX) + "," + std::to_string(basePosY) + ")");
	int output = shellGenerate();

	baseXPos[output] = basePosX;
	baseYPos[output] = basePosY;
	controllingPartyID[output] = partyID;
	int campID = 0;
	try
	{
		campID = reverseBaseTypeDictionary.at("Camp");
	}
	catch(std::out_of_range)
	{
		campID = 0; // default first one
	}
	baseType[output] = campID;
	
	return output;
}

void BaseManager::DebugLog(std::string message)
{
	gLog->Log("Base Manager", message);
}

bool BaseManager::TurnHandler(int entityID, double time)
{
	return true;
}

bool BaseManager::TimeHandler(int rounds, int turns, int hours, int days, int weeks, int months)
{
	return true;
}

void BaseManager::DumpBase(int baseID)
{
	DebugLog("Dumping base");
	
	for(int character : playerCharacters[baseID])
	{
		gGame->mCharacterManager->DumpCharacter(character);
	}

	for(int hench : henchmen[baseID])
	{
		gGame->mCharacterManager->DumpCharacter(hench);
	}

	for(int beast : animals[baseID])
	{
		gGame->mMobManager->DumpMob(beast);
	}
}
