#pragma once
#include "Manager.hpp"
#include <random>

#define BAD_ARTISTS_FILE "badArtists.txt"
#define BAD_SONGIDS_FILE "badSongIDs.txt"

#define JUKEBOX "fleym.nongd"
#define ALREADY_JUKEBOX "a replacement song from Jukebox for song ID"
#define JUKEBOX_INFO_SUFFIX "a Jukebox replacement song, don't worry--your Jukebox replacement song will still play."
#define SHORT_JUKEBOX_SUFFIX "\n<cy>Double-check your setup before trying again.</c>"
#define HOW_TO_UNBAN "\nVisit your mod settings if you would like to revert this."

using namespace geode::prelude;

namespace Utils {
	template<class T> T getSetting(const std::string& setting);
	bool getBool(const std::string& setting);
	int64_t getInt(const std::string& setting);
	double getDouble(const std::string& setting);
	std::string getString(const std::string& setting, const bool isPath = true);
	ccColor3B getColor(const std::string& setting);
	ccColor4B getColorAlpha(const std::string& setting);
	bool modEnabled();
	
	bool isModLoaded(const std::string& modID);
	Mod* getMod(const std::string& modID);
	std::string getModVersion(Mod* mod);

	template<typename T> bool contains(std::span<T> const& span, T const& value) {
		return std::find(span.begin(), span.end(), value) != span.end();
	}

	template<typename T> bool containsAnyFrom(std::span<T> const& span, std::span<T> const& cspan) {
		for (T const& c : cspan) if (contains<T>(span, c)) return true;
		return false;
	}

	template<typename T> void writeToFile(const std::string_view fileName, T artistOrSong, const std::string_view songName) {
		std::ofstream output;
		output.open((Mod::get()->getConfigDir() / fileName), std::ios_base::app);
		output << std::endl << fmt::format("{} # [NBS] Song Name: {} [NBS] #", artistOrSong, songName);
		output.close();
	}

	bool isSupportedExtension(std::string_view extension);
	bool isIDFromJukebox(const int& songID, MusicDownloadManager* mdm = MusicDownloadManager::sharedState());
	bool isStringFromJukebox(const std::string& path);
	bool bothBanListsEmpty(const Manager* manager = Manager::getSharedInstance());

	bool isBadArtist(const std::string& artistName, Manager* manager = Manager::getSharedInstance());
	bool isBadSong(const int& songID, Manager* manager = Manager::getSharedInstance());
	bool isInNeitherBanList(const int& songID, const std::string& artistName);

	void showAlert(const std::string &title, const std::string &desc);

	bool addBadSongID(const int& songID, const std::string_view songName, Manager* manager = Manager::getSharedInstance());
	bool addBadArtist(const int& songID, const std::string& artistName, const std::string_view songName, const bool& multipleArtists = false, Manager* manager = Manager::getSharedInstance());

	void addDirToReplacementSongPool(std::vector<std::string> &configDirSongs, const std::filesystem::path& folderPath = Mod::get()->getConfigDir());
	bool refreshLists();
	void refreshReplacementPool(Manager* manager = Manager::getSharedInstance());
	void readTextFiles(Manager* manager = Manager::getSharedInstance());
	std::string randomSongFromConfigDir(Manager* manager = Manager::getSharedInstance());
}