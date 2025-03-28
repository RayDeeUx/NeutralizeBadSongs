#include <Geode/modify/MusicDownloadManager.hpp>
#include <Geode/modify/CustomSongWidget.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include "Settings.hpp"
#include "Manager.hpp"
#include "Utils.hpp"

#define BAN_MENU "ban-menu"_spr

using namespace geode::prelude;

$on_mod(Loaded) {
	(void) Mod::get()->registerCustomSettingType("configdir", &MyButtonSettingV3::parse);
	(void) Mod::get()->registerCustomSettingType("updatelists", &MyButtonSettingV3::parse);
	(void) Utils::refreshLists();
	listenForSettingChanges("oneReplacementSong", [](std::string oneReplacementSong) {
		if (!PlayLayer::get()) Manager::getSharedInstance()->oneReplacementSong = std::move(oneReplacementSong);
	});
	listenForSettingChanges("randomReplacementEveryAttempt", [](bool unused) {
		if (!PlayLayer::get()) Manager::getSharedInstance()->songIDToReplacement.clear();
	});
	listenForSettingChanges("additionalFolder", [](std::filesystem::path additionalFolder) {
		Manager* manager = Manager::getSharedInstance();
		std::vector<std::string>& configDirSongs = manager->replacementSongsPool;
		configDirSongs.clear();
		Utils::addDirToReplacementSongPool(configDirSongs);
		Utils::addDirToReplacementSongPool(configDirSongs, additionalFolder);
		if (!PlayLayer::get()) {
			if (additionalFolder.string().empty()) return manager->songIDToReplacement.clear();
			return FLAlertLayer::create("Success!", "Your songs were loaded from your folder.", "Close")->show();
		}
	});
}

class $modify(MyMusicDownloadManager, MusicDownloadManager) {
	static void onModify(auto& self) {
		(void) self.setHookPriority("MusicDownloadManager::pathForSong", -3999);
	}
	gd::string pathForSong(int id) {
		const std::string& originalPath = MusicDownloadManager::pathForSong(id); // have jukebox's code run first
		Manager* manager = Manager::getSharedInstance();

		if (!Utils::modEnabled() || Utils::isStringFromJukebox(originalPath) || !PlayLayer::get()) return originalPath;

		const SongInfoObject* songInfo = MusicDownloadManager::getSongInfoObject(id);
		if (!songInfo) return originalPath;
		if (Utils::isInNeitherBanList(id, songInfo->m_artistName)) {
			if (!Utils::getBool("treatUndownloadedSongsAsBlacklistedSongs")) return originalPath;
			if (std::filesystem::exists(originalPath)) return originalPath;
		}

		if (Utils::getBool("dontPlaySongAtAll")) return "empty.mp3"_spr;

		if (Utils::getBool("onlyOneReplacement")) {
			if (manager->oneReplacementSong.empty() || !std::filesystem::exists(manager->oneReplacementSong))
				return originalPath;
			return manager->oneReplacementSong;
		}

		if (!Utils::getBool("randomReplacementEveryAttempt") && manager->songIDToReplacement.contains(id)) return manager->songIDToReplacement.find(id)->second;

		if (manager->replacementSongsPool.empty()) return originalPath;
		const std::string& file = Utils::randomSongFromConfigDir();
		if (file.empty() || !std::filesystem::exists(file)) return originalPath;
		if (!Utils::getBool("randomReplacementEveryAttempt")) manager->songIDToReplacement.insert({id, file});
		return file;
	}
};

class $modify(MyCustomSongWidget, CustomSongWidget) {
	struct Fields {
		std::string artistName;
		std::string songName;
		std::string extraArtistIDs;
		int songID = 0;
		bool isJukeboxPinMenu = false;
	};
	static void onModify(auto& self) {
		(void) self.setHookPriority("CustomSongWidget::init", -3999);
		(void) self.setHookPriority("CustomSongWidget::updateSongInfo", -3999);
	}
	bool init(SongInfoObject *songInfo, CustomSongDelegate *songDelegate, bool showSongSelect, bool showPlayMusic, bool showDownload, bool isRobtopSong, bool unkBool, bool isMusicLibrary, int unk) {
		bool isJukebox = Utils::isModLoaded(JUKEBOX);
		if (isJukebox) isJukebox = !Utils::getMod(JUKEBOX)->getSettingValue<bool>("old-label-display");
		m_fields->isJukeboxPinMenu = isJukebox;
		return CustomSongWidget::init(songInfo, songDelegate, showSongSelect, showPlayMusic, showDownload, isRobtopSong, unkBool, isMusicLibrary, unk);
	}
	void updateSongInfo() {
		CustomSongWidget::updateSongInfo();
		if (!Utils::modEnabled() || !m_songInfoObject || m_isRobtopSong) return;

		const auto fields = m_fields.self();
		const SongInfoObject* songInfo = m_songInfoObject;
		fields->songID = songInfo->m_songID;
		fields->songName = songInfo->m_songName;
		fields->artistName = songInfo->m_artistName;
		fields->extraArtistIDs = songInfo->m_extraArtists;

		if (CCNode* banMenu = this->getChildByID(BAN_MENU)) banMenu->removeMeAndCleanup();
		this->addChild(MyCustomSongWidget::createBanMenu(fields->isJukeboxPinMenu));
	}
	CCMenu* createBanMenu(const bool isJukeboxPinMenu) {
		const auto fields = m_fields.self();
		const bool inMusicLibrary = m_isMusicLibrary;

		CCMenu* banMenu = CCMenu::create();
		AxisLayout* layout = ColumnLayout::create()->setGap(-1.f)->setDefaultScaleLimits(.001f, .9f);
		banMenu->setAnchorPoint({.5, 1});
		banMenu->setContentHeight(52.f);

		if (!inMusicLibrary) banMenu->addChild(MyCustomSongWidget::createBanButton("settings", menu_selector(MyCustomSongWidget::onSettingsNBSAA)));
		if (!Utils::isIDFromJukebox(fields->songID)) {
			if (!Utils::isBadArtist(fields->artistName)) banMenu->addChild(MyCustomSongWidget::createBanButton("artist", menu_selector(MyCustomSongWidget::onBanArtistNBSAA)));
			if (!Utils::isBadSong(fields->songID)) banMenu->addChild(MyCustomSongWidget::createBanButton("song", menu_selector(MyCustomSongWidget::onBanSongNBSAA)));
		}

		banMenu->setLayout(layout);
		banMenu->setID(BAN_MENU);

		banMenu->setPosition(MyCustomSongWidget::findPos());
		if (banMenu->getChildrenCount() == 1 && !inMusicLibrary) MyCustomSongWidget::checkOnBanMenu(banMenu);
		else if (isJukeboxPinMenu && !inMusicLibrary) banMenu->setPositionY(banMenu->getPositionY() - 16);

		if (!inMusicLibrary) return banMenu;
		banMenu->setScale(.75f);
		banMenu->setZOrder(10);

		return banMenu;
	}
	CCMenuItemSpriteExtra* createBanButton(const std::string_view name, const cocos2d::SEL_MenuHandler menuHandler) {
		CircleButtonSprite* sprite = CircleButtonSprite::createWithSpriteFrameName(fmt::format("{}.png"_spr, name).c_str(), 1, CircleBaseColor::DarkPurple, CircleBaseSize::Tiny);
		CCMenuItemSpriteExtra* button = CCMenuItemSpriteExtra::create(sprite, this, menuHandler);
		button->setID(fmt::format("ban-{}-button"_spr, name));

		auto* nameSprite = typeinfo_cast<CCSprite*>(sprite->getTopNode());
		if (!nameSprite) return button;

		const bool isJukeboxSong = Utils::isIDFromJukebox(m_customSongID);

		ccColor3B color;
		if (isJukeboxSong) color = {255, 128, 0}; // plain settings logo
		else if (m_ncsLogo->isVisible()) color = {255, 255, 130}; // NCS
		else if (m_customSongID < 10000000) color = {255, 130, 255}; // newgrounds
		else color = {130, 255, 255}; // music library
		nameSprite->setColor(color);
		nameSprite->setScale(nameSprite->getScale() * 1.15f);

		if (name == "settings") return button;

		if (!isJukeboxSong) {
			CCSprite* minus = CCSprite::createWithSpriteFrameName("minus.png"_spr);
			minus->setID(fmt::format("minus-{}-sprite"_spr, name));
			minus->setAnchorPoint({0.f, 0.f});
			nameSprite->addChild(minus);
		}

		return button;
	}
	[[nodiscard]] CCPoint findPos() const {
		// adapted from Jukebox source code by Fleym
		// then i realized i could reduce it to two lines lmao
		CCNode* referenceNode = (m_ncsLogo && m_ncsLogo->isVisible()) ? m_ncsLogo : static_cast<CCNode*>(m_songLabel);
		return referenceNode->getPosition() + (m_isMusicLibrary ? CCPoint{-9.0f, 21.0f} : CCPoint{-9.0f, 13.0f});
	}
	void onBanArtistNBSAA(CCObject*) {
		if (!Utils::modEnabled()) return;

		if (Utils::getBool("autoRefreshReplacements")) Utils::refreshReplacementPool();

		const auto fields = m_fields.self();
		if (!Utils::addBadArtist(fields->songID, fields->artistName, fields->songName, !fields->extraArtistIDs.empty())) return;

		Notification::create(fmt::format("{}'s songs are now banned.", fields->artistName), NotificationIcon::Success, 3.0f)->show();
		MyCustomSongWidget::cleanupBanMenu("ban-artist-button"_spr);
	}
	void onBanSongNBSAA(CCObject*) {
		if (!Utils::modEnabled()) return;

		if (Utils::getBool("autoRefreshReplacements")) Utils::refreshReplacementPool();

		const auto fields = m_fields.self();
		if (!Utils::addBadSongID(fields->songID, fields->songName)) return;

		Notification::create(fmt::format("\"{}\" is now banned.", fields->songName), NotificationIcon::Success, 3.0f)->show();
		MyCustomSongWidget::cleanupBanMenu("ban-song-button"_spr);
	}
	void onSettingsNBSAA(CCObject*) {
		if (!Utils::modEnabled()) return;
		openSettingsPopup(Mod::get());
	}
	void cleanupBanMenu(std::string&& nodeID) {
		CCNode* toRemove = this->getChildByIDRecursive(nodeID);
		if (!toRemove) return;
		toRemove->removeMeAndCleanup();
		CCNode* banMenu = this->getChildByID(BAN_MENU);
		banMenu->updateLayout();
		if (m_isMusicLibrary || banMenu->getChildrenCount() > 1) return;
		MyCustomSongWidget::checkOnBanMenu(banMenu);
	}
	void checkOnBanMenu(CCNode* banMenu) const {
		if (m_isMusicLibrary || banMenu->getChildrenCount() > 1) return;
		banMenu->setContentHeight(26.f);
		banMenu->setAnchorPoint({.5, .5});
		banMenu->updateLayout();
		banMenu->setPositionY(-31.f);
	}
};

class $modify(MyPlayLayer, PlayLayer) {
	void onQuit() {
		Manager::getSharedInstance()->songIDToReplacement.clear();
		PlayLayer::onQuit();
	}
};