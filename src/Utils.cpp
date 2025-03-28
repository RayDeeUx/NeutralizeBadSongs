#include "Utils.hpp"

using namespace geode::cocos;

namespace Utils {
	template<class T> T getSetting(const std::string& setting) { return Mod::get()->getSettingValue<T>(setting); }

	bool getBool(const std::string& setting) { return getSetting<bool>(setting); }
	
	int64_t getInt(const std::string& setting) { return getSetting<int64_t>(setting); }
	
	double getDouble(const std::string& setting) { return getSetting<double>(setting); }

	std::string getString(const std::string& setting, bool isPath) {
		if (!isPath) return getSetting<std::string>(setting);
		return getSetting<std::filesystem::path>(setting).string();
	}

	ccColor3B getColor(const std::string& setting) { return getSetting<ccColor3B>(setting); }

	ccColor4B getColorAlpha(const std::string& setting) { return getSetting<ccColor4B>(setting); }

	bool modEnabled() { return getBool("enabled"); }
	
	bool isModLoaded(const std::string& modID) { return Loader::get()->isModLoaded(modID); }

	Mod* getMod(const std::string& modID) { return Loader::get()->getLoadedMod(modID); }

	std::string getModVersion(Mod* mod) { return mod->getVersion().toNonVString(); }

	bool isSupportedExtension(const std::string_view extension) {
		return !extension.empty() && (extension == ".mp3" || extension == ".ogg" || extension == ".oga" || extension == ".wav" || extension == ".flac");
	}

	bool isIDFromJukebox(const int& songID, MusicDownloadManager* mdm) {
		return isStringFromJukebox(mdm->pathForSong(songID));
	}

	bool isStringFromJukebox(const std::string& path) {
		return utils::string::contains(path, JUKEBOX);
	}

	bool bothBanListsEmpty(const Manager* manager) {
		return manager->badArtists.empty() && manager->badSongIDs.empty();
	}

	bool isBadArtist(const std::string& artistName, Manager* manager) {
		return Utils::contains<std::string>(manager->badArtists, utils::string::toLower(artistName));
	}

	bool isBadSong(const int& songID, Manager* manager) {
		return Utils::contains<int>(manager->badSongIDs, songID);
	}

	bool isInNeitherBanList(const int& songID, const std::string& artistName) {
		return  !bothBanListsEmpty() &&
				!isBadArtist(artistName) &&
				!isBadSong(songID);
	}

	void showAlert(const std::string &title, const std::string &desc) {
		// if (!Utils::getBool("useNotificationInstead")) return FLAlertLayer::create(nullptr, title.c_str(), desc, "Close", nullptr, 400.f)->show();
		if (!utils::string::contains(desc, ALREADY_JUKEBOX)) return Notification::create(fmt::format("{} Visit mod settings to revert.", title), NotificationIcon::Info, 2.f)->show();
		return Notification::create(fmt::format("{} Please double-check your setup.", title), NotificationIcon::Info, 2.f)->show();
	}

	bool addBadSongID(const int& songID, const std::string_view songName, Manager* manager) {
		if (Utils::isIDFromJukebox(songID)) {
			showAlert("Cannot ban song!", fmt::format("<cl>\"{}\"</c> is {} <cl>{}</c>.{}", songName, ALREADY_JUKEBOX, songID, SHORT_JUKEBOX_SUFFIX));
			return false;
		}
		if (Utils::contains<int>(manager->badSongIDs, songID)) {
			if (Utils::getBool("logging")) log::info("tried to ban song: {} (songName: {}) but it is already banned", songID, songName);
			const std::string& jukeboxInfo = Utils::isModLoaded("fleym.nongd") ? fmt::format("\n\nIf this song is {}", JUKEBOX_INFO_SUFFIX) : "";
			showAlert("Song already banned!", fmt::format("<cl>\"{}\"</c> (ID <cl>{}</c>) is already banned.{}{}", songName, songID, HOW_TO_UNBAN, jukeboxInfo));
			return false;
		}
		if (Utils::getBool("logging")) log::info("banning song: {} (songName: {})", songID, songName);
		manager->badSongIDs.push_back(songID);
		Utils::writeToFile<int>(BAD_SONGIDS_FILE, songID, songName);
		return true;
	}

	bool addBadArtist(const int& songID, const std::string& artistName, const std::string_view songName, const bool& multipleArtists, Manager* manager) {
		if (Utils::isIDFromJukebox(songID)) {
			showAlert("Cannot ban artist!", fmt::format("<cl>{}</c> is the artist for {} <cl>{}</c>.{}", artistName, ALREADY_JUKEBOX, songID, SHORT_JUKEBOX_SUFFIX));
			return false;
		}
		const std::string& toAdd = utils::string::toLower(artistName);
		if (Utils::contains<std::string>(manager->badArtists, toAdd)) {
			if (Utils::getBool("logging")) log::info("tried to ban artist: {} (songName: {}) but they are already banned", artistName, songName);
			const std::string& multipleArtistsInfo = multipleArtists ? fmt::format(" To ban the other artists from <cl>\"{}\"</c>, add them manually with the mod's text files.", songName) : "";
			const std::string& jukeboxInfo = Utils::isModLoaded(JUKEBOX) ? fmt::format("\n\nIf this artist is from {}", JUKEBOX_INFO_SUFFIX) : "";
			showAlert("Artist already banned!", fmt::format("<cl>{}</c> is already banned.{}{}{}", artistName, multipleArtistsInfo, HOW_TO_UNBAN, jukeboxInfo));
			return false;
		}
		if (Utils::getBool("logging")) log::info("banning artist: {} (songName: {})", artistName, songName);
		manager->badArtists.push_back(toAdd);
		Utils::writeToFile<std::string>(BAD_ARTISTS_FILE, toAdd, songName);
		return true;
	}

	void addDirToReplacementSongPool(std::vector<std::string>& configDirSongs, const std::filesystem::path& folderPath) {
		if (!std::filesystem::exists(folderPath)) return;
		for (const auto& file : std::filesystem::directory_iterator(folderPath)) {
			#ifdef GEODE_IS_WINDOWS
			std::string tempPath = geode::utils::string::wideToUtf8(file.path().wstring());
			#else
			std::string tempPath = file.path().string();
			#endif
			std::string tempExtension = file.path().extension().string();
			if (Utils::isSupportedExtension(tempExtension)) configDirSongs.push_back(tempPath);
		}
	}

	bool refreshLists() {
		Utils::refreshReplacementPool();
		Utils::readTextFiles();
		return true;
	}

	void refreshReplacementPool(Manager *manager) {
		manager->oneReplacementSong = getString("oneReplacementSong");
		if (!PlayLayer::get()) manager->songIDToReplacement.clear(); // better safe than sorry :3
		std::vector<std::string>& configDirSongs = manager->replacementSongsPool;
		configDirSongs.clear();
		Utils::addDirToReplacementSongPool(configDirSongs);
		Utils::addDirToReplacementSongPool(configDirSongs, Utils::getString("additionalFolder"));
	}

	void readTextFiles(Manager* manager) {
		auto configDir = Mod::get()->getConfigDir();
		auto pathBadArtists = (configDir / BAD_ARTISTS_FILE);
		auto pathBadSongIDs = (configDir / BAD_SONGIDS_FILE);
		manager->badSongIDs.clear();
		manager->badArtists.clear();
		if (!std::filesystem::exists(pathBadArtists)) {
			std::string content = R"(# hello there
# this is the text file where you add the names of artists whose songs you dislike
# separate artist names by new lines like you see in this file
# also, i didn't include any artists in here by default as that would
# cause more confusion for you in the long run, let's be honest
# don't worry, the mod ignores all lines that start with "#"
# you will need to restart the game to apply your changes when editing this file
# all matches will be case insensitive, keep that in mind! :)
# have fun!
# --raydeeux)";
			(void) utils::file::writeString(pathBadArtists, content);
		} else {
			std::ifstream badArtistsFile(pathBadArtists);
			std::string dislikedArtistString;
			while (std::getline(badArtistsFile, dislikedArtistString)) {
				if (dislikedArtistString.starts_with('#') || dislikedArtistString.empty()) continue;
				std::string dislikedArtistStringModified = dislikedArtistString;
				if (dislikedArtistStringModified.ends_with(" [NBS] #"))
					dislikedArtistStringModified = dislikedArtistStringModified.substr(0, dislikedArtistStringModified.find(" # [NBS] Song Name: "));
				manager->badArtists.push_back(utils::string::toLower(dislikedArtistStringModified));
			}
		}
		if (!std::filesystem::exists(pathBadSongIDs)) {
			std::string content = R"(# hello there
# this is the text file where you add song IDs that you dislike
# separate song IDs by new lines like you see in this file
# also, i didn't include any song IDs in here by default as that would
# cause more confusion for you in the long run, let's be honest
# don't worry, the mod ignores all lines that start with "#" and aren't exclusively numbers
# you will need to restart the game to apply your changes when editing this file
# any crashes or bugs caused by improperly editing this file will be ignored
# have fun!
# --raydeeux)";
			(void) utils::file::writeString(pathBadSongIDs, content);
		} else {
			std::ifstream badSongIDsFile(pathBadSongIDs);
			std::string badSongIDstring;
			while (std::getline(badSongIDsFile, badSongIDstring)) {
				if (badSongIDstring.starts_with('#') || badSongIDstring.empty()) continue;
				std::string badSongIDstringModified = badSongIDstring;
				if (badSongIDstringModified.ends_with(" [NBS] #"))
					badSongIDstringModified = badSongIDstringModified.substr(0, badSongIDstringModified.find(" # [NBS] Song Name: "));
				if (int badSongIDInt = utils::numFromString<int>(badSongIDstringModified).unwrapOr(-2); badSongIDInt > 0 && badSongIDInt != 71)
					manager->badSongIDs.push_back(badSongIDInt);
			}
		}
	}

	std::string randomSongFromConfigDir(Manager* manager) {
		std::vector<std::string>& vec = manager->replacementSongsPool;
		if (vec.empty()) return "";
		static std::mt19937_64 engine(std::random_device{});
		std::uniform_int_distribution<size_t> dist(0, vec.size() - 1);
		return vec.at(dist(engine));
	}

}