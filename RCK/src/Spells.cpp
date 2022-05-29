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
		// spells by index
		masterSpellList.push_back(s);
		int index = nextSpellIndex++;

		// name lookup
		spellLookup[s.Name()] = index;

		// add magic type to spell matrix if not exists
		if (spellMatrix.find(s.Source()) == spellMatrix.end())
		{
			std::vector<std::vector<int>> spellSource;
			for (int i = 0; i < 6; i++)
			{
				spellSource.push_back(std::vector<int>());
			}
			spellMatrix[s.Source()] = spellSource;
		}
		// add spell to matrix
		spellMatrix[s.Source()][s.Level()].push_back(index);
	}
}

std::vector<int>& SpellManager::getSpellsAtLevel(std::string magicType, int level)
{
	return spellMatrix[magicType][level];
}

Spell& SpellManager::getSpell(int index)
{
	return masterSpellList[index];
}