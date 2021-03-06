
#include <Menu/MultiPlayerMenu.h>
#include <Menu/CustomGameMenu.h>
#include <Menu/CustomGamePlayers.h>

#include <FileClasses/GFXManager.h>
#include <FileClasses/TextManager.h>

#include <Network/NetworkManager.h>
#include <Network/ENetHelper.h>

#include <GUI/MsgBox.h>

#include <globals.h>

#include <misc/string_util.h>

MultiPlayerMenu::MultiPlayerMenu() : MenuBase() {
	// set up window
	SDL_Surface *surf;
	surf = pGFXManager->getUIGraphic(UI_MenuBackground);

	setBackground(surf,false);
	resize(surf->w,surf->h);

	setWindowWidget(&windowWidget);

	windowWidget.addWidget(&mainVBox, Point(24,23),	Point(screen->w - 48, screen->h - 46));

    captionLabel.setText("Multiplayer Game");
    captionLabel.setAlignment(Alignment_HCenter);
    mainVBox.addWidget(&captionLabel, 24);
    mainVBox.addWidget(VSpacer::create(24));

	connectHBox.addWidget(Label::create("Host:"), 50);
	connectHostTextBox.setText("localhost");
	connectHBox.addWidget(&connectHostTextBox);
	connectHBox.addWidget(HSpacer::create(20));
	connectHBox.addWidget(Label::create("Port:"), 50);
	connectPortTextBox.setText(stringify(DEFAULT_PORT));
	connectHBox.addWidget(&connectPortTextBox, 90);
	connectHBox.addWidget(HSpacer::create(20));
	connectButton.setText(_("Connect"));
	connectButton.setOnClick(std::bind(&MultiPlayerMenu::onConnect, this));
	connectHBox.addWidget(&connectButton, 100);

    mainVBox.addWidget(Spacer::create(), 0.05);
	mainVBox.addWidget(&connectHBox, 28);

	mainVBox.addWidget(VSpacer::create(16));

    mainVBox.addWidget(Spacer::create(), 0.05);
    mainVBox.addWidget(&mainHBox, 0.85);

    mainHBox.addWidget(&leftVBox, 180);

	createLANGameButton.setText(_("Create LAN Game"));
	createLANGameButton.setOnClick(std::bind(&MultiPlayerMenu::onCreateLANGame, this));
	leftVBox.addWidget(&createLANGameButton, 0.1);

	leftVBox.addWidget(VSpacer::create(8));

	createInternetGameButton.setText(_("Create Internet Game"));
	createInternetGameButton.setOnClick(std::bind(&MultiPlayerMenu::onCreateInternetGame, this));
	leftVBox.addWidget(&createInternetGameButton, 0.1);

	leftVBox.addWidget(Spacer::create(), 0.8);

    rightVBox.addWidget(&gameTypeButtonsHBox, 24);

    mainHBox.addWidget(HSpacer::create(8));
    mainHBox.addWidget(Spacer::create(), 0.05);

    LANGamesButton.setText(_("LAN Games"));
    LANGamesButton.setToggleButton(true);
    LANGamesButton.setOnClick(std::bind(&MultiPlayerMenu::onGameTypeChange, this, 0));
    gameTypeButtonsHBox.addWidget(&LANGamesButton, 0.35);

    internetGamesButton.setText(_("Internet Games"));
    internetGamesButton.setToggleButton(true);
    internetGamesButton.setOnClick(std::bind(&MultiPlayerMenu::onGameTypeChange, this, 1));
    gameTypeButtonsHBox.addWidget(&internetGamesButton, 0.35);

	gameTypeButtonsHBox.addWidget(Spacer::create(), 0.3);

    gameList.setAutohideScrollbar(false);
    gameList.setOnSelectionChange(std::bind(&MultiPlayerMenu::onGameListSelectionChange, this, std::placeholders::_1));
    gameList.setOnDoubleClick(std::bind(&MultiPlayerMenu::onJoin, this));
    rightVBox.addWidget(&gameList, 0.95);

    mainHBox.addWidget(&rightVBox, 0.9);
    mainHBox.addWidget(Spacer::create(), 0.05);

    mainVBox.addWidget(Spacer::create(), 0.05);
    mainVBox.addWidget(VSpacer::create(10));

	mainVBox.addWidget(&buttonHBox, 24);

    buttonHBox.addWidget(HSpacer::create(70));
    backButton.setText(_("Back"));
	backButton.setOnClick(std::bind(&MultiPlayerMenu::onQuit, this));
	buttonHBox.addWidget(&backButton, 0.1);

	buttonHBox.addWidget(Spacer::create(), 0.8);

    joinButton.setText(_("Join"));
	joinButton.setOnClick(std::bind(&MultiPlayerMenu::onJoin, this));
	buttonHBox.addWidget(&joinButton, 0.1);
	buttonHBox.addWidget(HSpacer::create(90));

	// Start Network Manager
	fprintf(stdout, "starting network..."); fflush(stdout);
    pNetworkManager = new NetworkManager(settings.network.serverPort, settings.network.metaServer);
	LANGameFinderAndAnnouncer* pLANGFAA = pNetworkManager->getLANGameFinderAndAnnouncer();
	pLANGFAA->setOnNewServer(std::bind(&MultiPlayerMenu::onNewLANServer, this, std::placeholders::_1));
	pLANGFAA->setOnUpdateServer(std::bind(&MultiPlayerMenu::onUpdateLANServer, this, std::placeholders::_1));
	pLANGFAA->setOnRemoveServer(std::bind(&MultiPlayerMenu::onRemoveLANServer, this, std::placeholders::_1));
	pLANGFAA->refreshServerList();
    fprintf(stdout, "\t\tfinished\n"); fflush(stdout);

	onGameTypeChange(0);
}


MultiPlayerMenu::~MultiPlayerMenu() {
	fprintf(stdout, "stopping network..."); fflush(stdout);
	delete pNetworkManager;
	pNetworkManager = NULL;
    fprintf(stdout, "\t\tfinished\n");
}


/**
	This method is called, when the child window is about to be closed.
	This child window will be closed after this method returns.
	\param	pChildWindow	The child window that will be closed
*/
void MultiPlayerMenu::onChildWindowClose(Window* pChildWindow) {
    // Connection canceled
    // TODO
}

void MultiPlayerMenu::onCreateLANGame() {
    CustomGameMenu* pCustomGameMenu = new CustomGameMenu(true, true);
    pCustomGameMenu->showMenu();
    delete pCustomGameMenu;
}


void MultiPlayerMenu::onCreateInternetGame() {
    CustomGameMenu* pCustomGameMenu = new CustomGameMenu(true, false);
    pCustomGameMenu->showMenu();
    delete pCustomGameMenu;
}


void MultiPlayerMenu::onConnect() {
	std::string hostname = connectHostTextBox.getText();
	int port = atol(connectPortTextBox.getText().c_str());

    pNetworkManager->setOnReceiveGameInfo(std::bind(&MultiPlayerMenu::onReceiveGameInfo, this, std::placeholders::_1, std::placeholders::_2));
    pNetworkManager->setOnPeerDisconnected(std::bind(&MultiPlayerMenu::onPeerDisconnected, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    pNetworkManager->connect(hostname, port, settings.general.playerName);

    openWindow(MsgBox::create(_("Connecting...")));
}


void MultiPlayerMenu::onPeerDisconnected(std::string playername, bool bHost, int cause) {
    if(bHost) {
        pNetworkManager->setOnReceiveGameInfo(std::function<void (GameInitSettings, ChangeEventList)>());
        pNetworkManager->setOnPeerDisconnected(std::function<void (std::string, bool, int)>());
        closeChildWindow();

        showDisconnectMessageBox(cause);
    }
}

void MultiPlayerMenu::onJoin() {
	int selectedEntry = gameList.getSelectedIndex();
	if(selectedEntry >= 0) {
		GameServerInfo* pGameServerInfo = (GameServerInfo*) gameList.getEntryPtrData(selectedEntry);

        pNetworkManager->setOnReceiveGameInfo(std::bind(&MultiPlayerMenu::onReceiveGameInfo, this, std::placeholders::_1, std::placeholders::_2));
        pNetworkManager->setOnPeerDisconnected(std::bind(&MultiPlayerMenu::onPeerDisconnected, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        pNetworkManager->connect(pGameServerInfo->serverAddress, settings.general.playerName);

        openWindow(MsgBox::create(_("Connecting...")));
	}
}


void MultiPlayerMenu::onQuit() {
	SDL_Event quitEvent;
	quitEvent.type = SDL_QUIT;
	SDL_PushEvent(&quitEvent);
}


void MultiPlayerMenu::onGameTypeChange(int buttonID) {
    MetaServerClient* pMetaServerClient = pNetworkManager->getMetaServerClient();
    if((buttonID == 0) && internetGamesButton.getToggleState() == true) {
        // LAN Games

        gameList.clearAllEntries();
        InternetGameList.clear();

        std::list<GameServerInfo>::iterator iter;
        for(iter = LANGameList.begin(); iter != LANGameList.end(); ++iter) {
            GameServerInfo& gameServerInfo = *iter;
        	std::string description = gameServerInfo.serverName + " (" + Address2String(gameServerInfo.serverAddress) + " : " + stringify(gameServerInfo.serverAddress.port) + ") - "
                                + gameServerInfo.mapName + " (" + stringify(gameServerInfo.numPlayers) + "/" + stringify(gameServerInfo.maxPlayers) + ")";
            gameList.addEntry(description, &*iter);
        }

        // stop listening on internet games
        pMetaServerClient->setOnGameServerInfoList(std::function<void (std::list<GameServerInfo>&)>());
        pMetaServerClient->setOnMetaServerError(std::function<void (int, std::string)>());
    } else if((buttonID == 1) && (LANGamesButton.getToggleState() == true)) {
        // Internet Games

        gameList.clearAllEntries();
        InternetGameList.clear();

        // start listening on internet games
        pMetaServerClient->setOnGameServerInfoList(std::bind(&MultiPlayerMenu::onGameServerInfoList, this, std::placeholders::_1));
        pMetaServerClient->setOnMetaServerError(std::bind(&MultiPlayerMenu::onMetaServerError, this, std::placeholders::_1, std::placeholders::_2));
    }

    LANGamesButton.setToggleState(buttonID == 0);
    internetGamesButton.setToggleState(buttonID == 1);

}


void MultiPlayerMenu::onGameListSelectionChange(bool bInteractive) {
}


void MultiPlayerMenu::onNewLANServer(GameServerInfo gameServerInfo) {
	LANGameList.push_back(gameServerInfo);
	std::string description = gameServerInfo.serverName + " (" + Address2String(gameServerInfo.serverAddress) + " : " + stringify(gameServerInfo.serverAddress.port) + ") - "
                                + gameServerInfo.mapName + " (" + stringify(gameServerInfo.numPlayers) + "/" + stringify(gameServerInfo.maxPlayers) + ")";
	gameList.addEntry(description, &LANGameList.back());
}

void MultiPlayerMenu::onUpdateLANServer(GameServerInfo gameServerInfo) {
    size_t index = 0;
    std::list<GameServerInfo>::iterator iter;
    for(iter = LANGameList.begin(); iter != LANGameList.end(); ++iter) {
        if(*iter == gameServerInfo) {
            *iter = gameServerInfo;
            break;
        }

        index++;
    }

	if(index < LANGameList.size()) {
        std::string description = gameServerInfo.serverName + " (" + Address2String(gameServerInfo.serverAddress) + " : " + stringify(gameServerInfo.serverAddress.port) + ") - "
                                    + gameServerInfo.mapName + " (" + stringify(gameServerInfo.numPlayers) + "/" + stringify(gameServerInfo.maxPlayers) + ")";

        gameList.setEntry(index, description);
	}
}

void MultiPlayerMenu::onRemoveLANServer(GameServerInfo gameServerInfo) {
	for(int i=0;i<gameList.getNumEntries();i++) {
		GameServerInfo* pGameServerInfo = (GameServerInfo*) gameList.getEntryPtrData(i);
		if(*pGameServerInfo == gameServerInfo) {
			gameList.removeEntry(i);
			break;
		}
	}

	LANGameList.remove(gameServerInfo);
}

void MultiPlayerMenu::onGameServerInfoList(std::list<GameServerInfo>& gameServerInfoList) {
    // remove all game servers from the list that are not included in the sent list
    std::list<GameServerInfo>::iterator oldListIter = InternetGameList.begin();
    int index = 0;
    while(oldListIter != InternetGameList.end()) {
        GameServerInfo& gameServerInfo = *oldListIter;

        if(std::find(gameServerInfoList.begin(), gameServerInfoList.end(), gameServerInfo) == gameServerInfoList.end()) {
            // not found => remove
            gameList.removeEntry(index);
            oldListIter = InternetGameList.erase(oldListIter);
        } else {
            // found => move on
            ++oldListIter;
            ++index;
        }
    }

    // now add all servers that are included for the first time and update the others
    std::list<GameServerInfo>::iterator newListIter;
    for(newListIter = gameServerInfoList.begin(); newListIter != gameServerInfoList.end(); ++newListIter) {
        GameServerInfo& gameServerInfo = *newListIter;

        size_t oldListIndex = 0;
        std::list<GameServerInfo>::iterator oldListIter;
        for(oldListIter = InternetGameList.begin(); oldListIter != InternetGameList.end(); ++oldListIter) {
            if(*oldListIter == gameServerInfo) {
                // found => update
                *oldListIter = gameServerInfo;
                break;
            }

            oldListIndex++;
        }


        if(oldListIndex >= InternetGameList.size()) {
            // not found => add at the end
            InternetGameList.push_back(gameServerInfo);
            std::string description = gameServerInfo.serverName + " (" + Address2String(gameServerInfo.serverAddress) + " : " + stringify(gameServerInfo.serverAddress.port) + ") - "
                                + gameServerInfo.mapName + " (" + stringify(gameServerInfo.numPlayers) + "/" + stringify(gameServerInfo.maxPlayers) + ")";
            gameList.addEntry(description, &InternetGameList.back());
        } else {
            // found => update
            std::string description = gameServerInfo.serverName + " (" + Address2String(gameServerInfo.serverAddress) + " : " + stringify(gameServerInfo.serverAddress.port) + ") - "
                                    + gameServerInfo.mapName + " (" + stringify(gameServerInfo.numPlayers) + "/" + stringify(gameServerInfo.maxPlayers) + ")";

            gameList.setEntry(oldListIndex, description);
        }

    }
}

void MultiPlayerMenu::onMetaServerError(int errorcause, std::string errorMessage) {
    switch(errorcause) {
        case METASERVERCOMMAND_ADD: {
            openWindow(MsgBox::create("MetaServer error on adding game server:\n" + errorMessage));
        } break;

        case METASERVERCOMMAND_UPDATE: {
            openWindow(MsgBox::create("MetaServer error on updating game server:\n" + errorMessage));
        } break;

        case METASERVERCOMMAND_REMOVE: {
            openWindow(MsgBox::create("MetaServer error on removing game server:\n" + errorMessage));
        } break;

        case METASERVERCOMMAND_LIST	: {
            openWindow(MsgBox::create("MetaServer error on list game servers:\n" + errorMessage));
        } break;

        case METASERVERCOMMAND_EXIT:
        default: {
            openWindow(MsgBox::create("MetaServer error:\n" + errorMessage));
        } break;
    }
}

void MultiPlayerMenu::onReceiveGameInfo(GameInitSettings gameInitSettings, ChangeEventList changeEventList) {
    closeChildWindow();

    pNetworkManager->setOnPeerDisconnected(std::function<void (std::string, bool, int)>());

    CustomGamePlayers* pCustomGamePlayers = new CustomGamePlayers(gameInitSettings, false);
    pCustomGamePlayers->onReceiveChangeEventList(changeEventList);
    int ret = pCustomGamePlayers->showMenu();
    delete pCustomGamePlayers;
    switch(ret) {
        case MENU_QUIT_DEFAULT: {
            // nothing
        } break;

        case MENU_QUIT_GAME_FINISHED: {
            quit(MENU_QUIT_GAME_FINISHED);
        } break;

        default: {
            showDisconnectMessageBox(ret);
        } break;
    }
}

void MultiPlayerMenu::showDisconnectMessageBox(int cause) {
    switch(cause) {
        case NETWORKDISCONNECT_QUIT: {
            openWindow(MsgBox::create(_("The game host quit the game!")));
        } break;

        case NETWORKDISCONNECT_TIMEOUT: {
            openWindow(MsgBox::create(_("The server timed out!")));
        } break;

        case NETWORKDISCONNECT_PLAYER_EXISTS: {
            openWindow(MsgBox::create(_("A player with the same name as yours already exists!")));
        } break;

        case NETWORKDISCONNECT_GAME_FULL: {
            openWindow(MsgBox::create(_("There is no free player slot in this game left!")));
        } break;

        default: {
            openWindow(MsgBox::create(_("The connection to the game host was lost!")));
        } break;
    }
}
