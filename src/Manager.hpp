#pragma once

// Manager.hpp structure by acaruso
// reused with explicit permission and strong encouragement

using namespace geode::prelude;

class Manager {

protected:
	static Manager* instance;
public:

	bool calledAlready = false;

	std::vector<std::string> replacementSongsPool;
	std::string oneReplacementSong;

	std::vector<int> badSongIDs;
	std::vector<std::string> badArtists;

	std::map<int, std::string> songIDToReplacement;

	static Manager* getSharedInstance() {
		if (!instance) instance = new Manager();
		return instance;
	}

};