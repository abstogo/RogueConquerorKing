#include "Spells.h"
#include <filesystem>

namespace fs = std::filesystem;

SpellManager* SpellManager::LoadSpells()
{
	gLog->Log("Spell Loader", "Started");

	std::string scriptsFolder = "RCK/scripts";
	
	SpellManager* sm = new SpellManager();
	for (const auto& entry : fs::directory_iterator(scriptsFolder))
	{
		std::string filename = entry.path().filename().string();
		std::string pathString = entry.path().string();
		// see if it starts with "spellbook_"
		std::string prefix = "spellbook_";
		if (filename.substr(0, prefix.size()).compare(prefix) == 0)
		{
			gLog->Log("Spell Loader", "Decoding " + pathString);
			sm->LoadSpellSet(pathString);
		}
		
	}
	
	gLog->Log("Spell Loader", "Completed");

	return sm;
}

void SpellManager::LoadSpellSet(std::string path)
{
	std::ifstream is(path);

	SpellSet& spellSet = jsoncons::decode_json<SpellSet>(is);

	// add spellset to selection
	spellSets[path] = spellSet;

	// now add each spell to the master set

	for (Spell s : spellSet.Spells())
	{
		masterSpellList.push_back(s);
		spellLookup[s.Name()] = nextSpellIndex++;
	}
}