/*
 *  This file is part of Dune Legacy.
 *
 *  Dune Legacy is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Dune Legacy is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Dune Legacy.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GAME_H
#define GAME_H

#include <misc/Random.h>
#include <misc/RobustList.h>
#include <misc/InputStream.h>
#include <misc/OutputStream.h>
#include <ObjectData.h>
#include <ObjectManager.h>
#include <CommandManager.h>
#include <GameInterface.h>
#include <INIMap/INIMapLoader.h>
#include <GameInitSettings.h>
#include <Trigger/TriggerManager.h>
#include <players/Player.h>


#include <DataTypes.h>

#include <SDL.h>
#include <stdarg.h>
#include <string>
#include <map>
#include <utility>
#include <limits.h>

// forward declarations
class ObjectBase;
class InGameMenu;
class MentatHelp;
class WaitingForOtherPlayers;
class ObjectManager;
class House;
class Explosion;


#define END_WAIT_TIME				(6*1000)

#define GAME_NOTHING			-1
#define	GAME_RETURN_TO_MENU		0
#define GAME_NEXTMISSION		1
#define	GAME_LOAD				2
#define GAME_DEBRIEFING_WIN		3
#define	GAME_DEBRIEFING_LOST	4
#define GAME_CUSTOM_GAME_STATS  5


#define NIGHT_MIN_GAMMA (float)0.35f
#define NIGHT_MULT_GAMMA (float)0.70f
#define NIGHT_MAX_GAMMA (float)(NIGHT_MIN_GAMMA+NIGHT_MULT_GAMMA)

class Game
{
public:

    /**
        Default constructor. Call initGame() or initReplay() afterwards.
    */
	Game();

    /**
        Destructor
    */
	~Game();

    /**
        Initializes a game with the specified settings
        \param  newGameInitSettings the game init settings to initialize the game
    */
	void initGame(const GameInitSettings& newGameInitSettings);

	/**
        Initializes a replay from the specified filename
        \param  filename    the file containing the replay
	*/
	void initReplay(const std::string& filename);



	friend class INIMapLoader; // loading INI Maps is done with a INIMapLoader helper object


    /**
        This method processes all objects in the current game. It should be executed exactly once per game tick.
    */
	void processObjects();

    /**
        This method draws a complete frame.
    */
	void drawScreen();

    /**
        This method proccesses all the user input.
    */
	void doInput();

    /**
        Returns the current game cycle number.
        \return the current game cycle
    */
	Uint32 getGameCycleCount() const { return gameCycleCount; };

	/**
        Return the game time in milliseconds.
        \return the current game time in milliseconds
	*/
	Uint32 getGameTime() const { return gameCycleCount * GAMESPEED_DEFAULT; };


	std::string getGameTimeStringFromCycle(Uint32 gamecycle) {
				std::string time;
				int     seconds = (gamecycle* GAMESPEED_DEFAULT) / 1000;
				if (seconds / 3600 >0 )
					time = std::to_string(seconds / 3600)+"h";
				if ((seconds % 3600)/60 >0 )
					time += std::to_string((seconds % 3600)/60)+"min";
				time += std::to_string(seconds % 60)+"sec";
				return time;
	}

	std::string getGameTimeString() {
			std::string time;
			int     seconds = getGameTime() / 1000;
			if (seconds / 3600 >0 )
				time = std::to_string(seconds / 3600)+"h";
			if ((seconds % 3600)/60 >0 )
				time += std::to_string((seconds % 3600)/60)+"min";
			time += std::to_string(seconds % 60)+"sec";
			return time;
	}

	std::string getGameTimeString2() {
			std::string time;
			int     seconds = getGameTime() / 1000;
			if (seconds / 3600 >0 )
				time = std::to_string(seconds / 3600)+":";
			if ((seconds % 3600)/60 >0 )
				time += std::to_string((seconds % 3600)/60)+":";
			time += std::to_string(seconds % 60);
			return time;
	}

    /**
        Get the command manager of this game
        \return the command manager
    */
	CommandManager& getCommandManager() { return cmdManager; };

    /**
        Get the trigger manager of this game
        \return the trigger manager
    */
	TriggerManager& getTriggerManager() { return triggerManager; };

	/**
        Get the explosion list.
        \return the explosion list
	*/
	RobustList<Explosion*>& getExplosionList() { return explosionList; };

	/**
        Returns the house with the id houseID
        \param  houseID the id of the house to return
        \return the house with id houseID
	*/
	House* getHouse(int houseID) {
        return house[houseID];
	}
	/**
        Tell if the game is ended and won or lost
	*/
	bool isGameWon();
	/**
        The current game is finished and the local house has won
	*/
	void setGameWon();

	/**
        The current game is finished and the local house has lost
	*/
	void setGameLost();

    /**
        Draws the cursor.
    */
	void drawCursor();

	/**
        Do the palette animation for the windtrap.
	*/
	void doWindTrapPalatteAnimation();

	/**
        Do the gamma setting for the day/night cycle.
	*/
	Uint32 doDayNightCycle(Uint32 refcycle,float ambiant);


	typedef enum {
		Begin=0,
		End=1,
	} TypeOffset;

	int getDayNightOffset(Phase wanted_phase = Day);
	int getDayNightToOffset(TypeOffset t, Phase wanted_phase, int offset_base);
	std::pair<int,float> getDayNightLight(Uint32 refcycle,int offset ,Phase current_phase = Day);

    /**
        This method sets up the view. The start position is the center point of all owned units/structures
    */
	void setupView();

    /**
        This method loads a previously saved game.
        \param filename the name of the file to load from
        \return true on success, false on failure
    */
	bool loadSaveGame(std::string filename);

    /**
        This method loads a previously saved game.
        \param stream the stream to load from
        \return true on success, false on failure
    */
	bool loadSaveGame(InputStream& stream);

    /**
        This method saves the current running game.
        \param filename the name of the file to save to
        \return true on success, false on failure
    */
	bool saveGame(std::string filename);

    /**
        This method starts the game. Will return when the game is finished or aborted.
    */
	void runMainLoop();

	inline void quitGame() { bQuitGame = true;};

    /**
        This method pauses the current game.
    */
	void pauseGame() {
        if(bPause != true) {
            bPause = true;
            palette.invertPalette();
            palette.applyToSurface(screen,SDL_PHYSPAL,1,palette.getSDLPalette()->ncolors-1);
            //	XXX memcheck reports a source and destination overlap in memcpy
            SDL_SetGamma(1,1,1);
        }
	}

    /**
        This method resumes the current paused game.
    */
	void resumeGame();

    /**
        This method writes out an object to a stream.
        \param stream   the stream to write to
        \param obj      the object to be saved
    */
	void saveObject(OutputStream& stream, ObjectBase* obj);

    /**
        This method loads an object from the stream.
        \param stream   the stream to read from
        \param ObjectID the object id that this unit/structure should get
        \return the read unit/structure
    */
	ObjectBase* loadObject(InputStream& stream, Uint32 objectID);

	inline ObjectManager& getObjectManager() { return objectManager; };
	inline GameInterface& getGameInterface() { return *pInterface; };

	const GameInitSettings& getGameInitSettings() const { return gameInitSettings; };
    void setNextGameInitSettings(const GameInitSettings& nextGameInitSettings) { this->nextGameInitSettings = nextGameInitSettings; };

    /**
        This method should be called if whatNext() returns GAME_NEXTMISSION or GAME_LOAD. You should
        destroy this Game and create a new one. Call Game::initGame() with the GameInitClass
        that was returned previously by getNextGameInitSettings().
        \return a GameInitSettings-Object that describes the next game.
    */
	GameInitSettings getNextGameInitSettings();

    /**
        This method should be called after startGame() has returned. whatNext() will tell the caller
        what should be done after the current game has finished.<br>
        Possible return values are:<br>
        GAME_RETURN_TO_MENU  - the game is finished and you should return to the main menu<br>
        GAME_NEXTMISSION     - the game is finished and you should load the next mission<br>
        GAME_LOAD			 - from inside the game the user requests to load a savegame and you should do this now<br>
        GAME_DEBRIEFING_WIN  - show debriefing (player has won) and call whatNext() again afterwards<br>
        GAME_DEBRIEFING_LOST - show debriefing (player has lost) and call whatNext() again afterwards<br>
        <br>
        \return one of GAME_RETURN_TO_MENU, GAME_NEXTMISSION, GAME_LOAD, GAME_DEBRIEFING_WIN, GAME_DEBRIEFING_LOST
    */
	int whatNext();

    /**
        This method is the callback method for the OPTIONS button at the top of the screen.
        It pauses the game and loads the in game menu.
    */
	void onOptions();

    /**
        This method is the callback method for the MENTAT button at the top of the screen.
        It pauses the game and loads the mentat help screen.
    */
	void onMentat();

    /**
        This method selects all units/structures in the list aList.
        \param aList the list containing all the units/structures to be selected
    */
	void selectAll(std::list<Uint32>& aList);

    /**
        This method unselects all units/structures in the list aList.
        \param aList the list containing all the units/structures to be unselected
    */
	void unselectAll(std::list<Uint32>& aList);

    /**
        Returns a list of all currently selected objects.
        \return list of currently selected units/structures
    */
	std::list<Uint32>& getSelectedList() { return selectedList; };


    /**
        Returns a list of Coord of all currently selected objects.
        \return list of currently selected units/structures
    */
	std::list<std::pair<Uint32,Coord>>& getSelectedListCoord() { return selectedListCoord; };

	/**
        Marks that the selection changed (and must be retransmitted to other players in multiplayer games)
	*/
	inline void selectionChanged() { bSelectionChanged = true; };


	void onReceiveSelectionList(std::string name, std::list<Uint32> newSelectionList, int groupListIndex);

    /**
        Returns a list of all currently by  the other player selected objects (Only in multiplayer with multiple players per house).
        \return list of currently selected units/structures by the other player
    */
	std::list<Uint32>& getSelectedByOtherPlayerList() { return selectedByOtherPlayerList; };

    /**
		Called when a peer disconnects the game.
	*/
    void onPeerDisconnected(std::string name, bool bHost, int cause);

    /**
        Adds a new message to the news ticker.
        \param  text    the text to add
    */
	void addToNewsTicker(const std::string& text) {
		if(pInterface != NULL) {
			pInterface->addToNewsTicker(text);
		}
	}

    /**
        Adds an urgent message to the news ticker.
        \param  text    the text to add
    */
	void addUrgentMessageToNewsTicker(const std::string& text) {
		if(pInterface != NULL) {
			pInterface->addUrgentMessageToNewsTicker(text);
		}
	}

	/**
        This method returns wether the game is currently paused
        \return true, if paused, false otherwise
	*/
	bool isGamePaused() const { return bPause; };

	/**
        This method returns wether the game is finished
        \return true, if paused, false otherwise
	*/
	bool isGameFinished() const { return finished; };

	/**
        Are cheats enabled?
        \return true = cheats enabled, false = cheats disabled
	*/
	bool areCheatsEnabled() const { return bCheatsEnabled; };

    /**
        Register a new player in this game.
        \param  player      the player to register
    */
    void registerPlayer(Player* player) {
        playerName2Player.insert( std::make_pair(player->getPlayername(), player) );
        playerID2Player.insert( std::make_pair(player->getPlayerID(), player) );
    }

    /**
        Unregisters the specified player
        \param  player
    */
    void unregisterPlayer(Player* player) {
        playerID2Player.erase(player->getPlayerID());

        std::multimap<std::string, Player*>::iterator iter;
        for(iter = playerName2Player.begin(); iter != playerName2Player.end(); ++iter) {
                if(iter->second == player) {
                    playerName2Player.erase(iter);
                    break;
                }
        }
    }

    /**
        Returns the first player with the given name.
        \param  playername  the name of the player
        \return the player or NULL if none was found
    */
	Player* getPlayerByName(const std::string& playername) const {
        std::multimap<std::string, Player*>::const_iterator iter = playerName2Player.find(playername);
        if(iter != playerName2Player.end()) {
            return iter->second;
        } else {
            return NULL;
        }
	}

    /**
        Returns the player with the given id.
        \param  playerID  the name of the player
        \return the player or NULL if none was found
    */
	Player* getPlayerByID(Uint8 playerID) const {
        std::map<Uint8, Player*>::const_iterator iter = playerID2Player.find(playerID);
        if(iter != playerID2Player.end()) {
            return iter->second;
        } else {
            return NULL;
        }
	}

    /**
        This function is called when the user left clicks on the radar
        \param  worldPosition       position in world coordinates
        \param  bRightMouseButton   true = right mouse button, false = left mouse button
        \param  bDrag               true = the mouse was moved while being pressed, e.g. dragging
        \return true if dragging should start or continue
    */
    bool onRadarClick(Coord worldPosition, bool bRightMouseButton, bool bDrag);


    inline bool getDrawOverlay() {return drawOverlay;}

	inline ObjectBase* getGroupLeader() { return groupLeader; }
	inline void setGroupLeader(ObjectBase* lead) {  groupLeader = lead; }



    /* Night/Days  */
    inline void setNumberOfDays(int d)		{ nbofdays = (Uint32)d; if (pInterface != NULL) pInterface->getDaysCounter().setCount(d);}
    inline Uint32 getNumberOfDays() 		{return nbofdays;}


    inline Uint8 getDayScale()				{ return dayscale; }
    inline void setDayScale(Uint8 scale)	{
    	if (scale >= GAMEDAYSCALE_MIN && scale <= GAMEDAYSCALE_MAX)
    			dayscale = scale;

    }
    inline Uint32 getDayDuration() 			{return (((1 << (int)dayscale))%INT_MAX);}


#define pow__2 ( (1 << (int)(dayscale+2)))
#define pow__1 ( (1 << (int)(dayscale+1)))
#define pow_0  ( (1 << (int)(dayscale-0)))
#define pow_1  ( (1 << (int)(dayscale-1)))
#define pow_2  ( (1 << (int)(dayscale-2)))
#define pow_3  ( (1 << (int)(dayscale-3)))
#define pow_4  ( (1 << (int)(dayscale-4)))
#define pow_6  ( (1 << (int)(dayscale-6)))
#define pow_7  ( (1 << (int)(dayscale-7)))
#define pow_8  ( (1 << (int)(dayscale-8)))



    inline void setDayPhase(Phase p)		{ dayphase = p; if (pInterface != NULL) pInterface->getDaysCounter().setCounterMode(dayphase);}
    inline Phase getIncDayPhase(Phase p = currentGame->getDayPhase())
    										{ int phase=(int)p;

    											phase = (phase+1)%PHASES_NB;
    											return (Phase)phase;}
    inline Phase getDecDayPhase(Phase p = currentGame->getDayPhase())
    										{ int phase=(int)p;

    											phase = (phase-1)%PHASES_NB;
    											if (phase ==0) phase=PHASES_NB;
    											return (Phase)phase;}
    inline Phase getDayPhase() 				{return dayphase;}

    inline Phase getDayPhaseInverted(Phase p = currentGame->getDayPhase())
    										{ Phase a =	(Phase)((int)(p + (PHASES_NB/2))%PHASES_NB); return (int)a == 0 ? (Phase) PHASES_NB : a ;}

    inline void updateCycleAnim()
    										{// TODO : Remove ?
    										 if (pInterface != NULL) pInterface->getDaysCounter().updateBlitter(screen);}


    inline Uint32 getLight() 				{return light;}
    inline void setLight(float illum) 		{light = illum;}

    inline void setDayPhaseString(std::string phase) {

    	if (phase == "Morning") {
    		setDayPhase (Morning);
    	} else if (phase == "Day") {
    		setDayPhase (Day);
    	}else if (phase == "Eve") {
    		setDayPhase (Eve);
    	}else if (phase == "Night") {
    		setDayPhase (Night);
    	}else if (phase == "None") {
    		setDayPhase (PHASE_NONE);
    	}else {
    		setDayPhase (PHASE_NONE);
    	}
    };
    inline std::string getDayPhaseString(Phase phase = currentGame->getDayPhase()) {
      	switch (phase) {
      		case	Morning:
      			return (_("Morning"));
      			break;
      		case	Day:
      			return (_("Day"));
      			break;
      		case	Eve:
      			return (_("Eve"));
      			break;
      		case	Night:
      			return (_("Night"));
      			break;
      		case	PHASE_NONE:
      		default:
      			return (_("None"));
      			break;

      	}
    };

private:

    /**
        Checks whether the cursor is on the radar view
        \param  mouseX  x-coordinate of cursor
        \param  mouseY  y-coordinate of cursor
        \return true if on radar view
    */
    bool isOnRadarView(int mouseX, int mouseY);

    /**
        Handles the press of one key for chatting
        \param  key the key pressed
    */
    void handleChatInput(SDL_KeyboardEvent& keyboardEvent);

    /**
        Handles the press of one key
        \param  key the key pressed
    */
    void handleKeyInput(SDL_KeyboardEvent& keyboardEvent);

    /**
        Performs a building placement
        \param  xPos    x-coordinate in map coordinates
        \param  yPos    x-coordinate in map coordinates
        \return true if placement was successful
    */
    bool handlePlacementClick(int xPos, int yPos);

    /**
        Performs a attack click for the currently selected units/structures.
        \param  xPos    x-coordinate in map coordinates
        \param  yPos    x-coordinate in map coordinates
        \return true if attack is possible
    */
    bool handleSelectedObjectsAttackClick(int xPos, int yPos);

    /**
        Performs a salve attack click for the currently selected units/structures.
        \param  xPos    x-coordinate in map coordinates
        \param  yPos    x-coordinate in map coordinates
        \return true if attack is possible
    */
    bool handleSelectedObjectsSalveAttackClick(int xPos, int yPos);


    /**
        Performs a move click for the currently selected units/structures.
        \param  xPos    x-coordinate in map coordinates
        \param  yPos    x-coordinate in map coordinates
        \return true if move is possible
    */
    bool handleSelectedObjectsMoveClick(int xPos, int yPos);

    /**
        Performs a capture click for the currently selected units/structures.
        \param  xPos    x-coordinate in map coordinates
        \param  yPos    x-coordinate in map coordinates
        \return true if capture is possible
    */
    bool handleSelectedObjectsCaptureClick(int xPos, int yPos);

    /**
        Performs an action click for the currently selected units/structures.
        \param  xPos    x-coordinate in map coordinates
        \param  yPos    x-coordinate in map coordinates
        \return true if action click is possible
    */
    bool handleSelectedObjectsActionClick(int xPos, int yPos);



    ObjectBase* findGroupLeader();



public:
    enum {
        CursorMode_Normal,
        CursorMode_Attack,
        CursorMode_SalveAttack,
        CursorMode_Move,
        CursorMode_Capture,
        CursorMode_Placing
    };



    int         currentCursorMode;

	GAMETYPE	gameType;
	int			techLevel;
	int			winFlags;
	int         loseFlags;

	int		    gamespeed;

	Random      randomGen;          ///< This is the random number generator for this game
	ObjectData  objectData;         ///< This contains all the unit/structure data

	GAMESTATETYPE gameState;

private:
    bool        chatMode;           ///< chat mode on?
    bool		drawOverlay;		///< overlay drawning of FindTarget
    std::string typingChatMessage;  ///< currently typed chat message

	bool        scrollDownMode;     ///< currently scrolling the map down?
	bool        scrollLeftMode;     ///< currently scrolling the map left?
	bool        scrollRightMode;    ///< currently scrolling the map right?
	bool        scrollUpMode;       ///< currently scrolling the map up?

	bool	    selectionMode;      ///< currently selection multiple units with a selection rectangle?
	SDL_Rect    selectionRect;      ///< the drawn rectangle while selection multiple units

	int		    whatNextParam;

	Uint32      indicatorFrame;
	int         indicatorTime;
	int         indicatorTimer;
	Coord       indicatorPosition;

	float       averageFrameTime;	///< The weighted average of the frame time of all previous frames (smoothed fps = 1000.0f/averageFrameTime)
	float		*FrameTime;
	float		minFrameTime;
	float		maxFrameTime;
	double 		varFrameTime;
	float		devFrameTime;


	Uint32      gameCycleCount;
	Uint32 		nbofdays;		///< Number of days on mission
	Uint8		dayscale;		///< Scaler for the calculus of the day length [8 -14]
	Phase		dayphase;		///< Phase of the current day
	float		light;

	Uint32      skipToGameCycle;    ///< skip to this game cycle

	SDL_Rect	powerIndicatorPos;  ///< position of the power indicator in the right game bar
	SDL_Rect	spiceIndicatorPos;  ///< position of the spice indicator in the right game bar
	SDL_Rect	topBarPos;          ///< position of the top game bar
    SDL_Rect    sideBarPos;         ///< position of the right side bar

	////////////////////

	GameInitSettings	                gameInitSettings;       ///< the init settings this game was started with
	GameInitSettings	                nextGameInitSettings;   ///< the init settings the next game shall be started with (restarting the mission, loading a savegame)
    GameInitSettings::HouseInfoList     houseInfoListSetup;     ///< this saves with which houses and players the game was actually set up. It is a copy of gameInitSettings::houseInfoList but without random houses


	ObjectManager       objectManager;          ///< This manages all the object and maps object ids to the actual objects

	CommandManager      cmdManager;			    ///< This is the manager for all the game commands (e.g. moving a unit)

	TriggerManager      triggerManager;         ///< This is the manager for all the triggers the scenario has (e.g. reinforcements)

	bool	bQuitGame;					///< Should the game be quited after this game tick
	bool	bPause;						///< Is the game currently halted
	bool    bMenu;                      ///< Is there currently a menu shown (options or mentat menu)
	bool	bReplay;					///< Is this game actually a replay

	std::list<Uint32>::iterator electIter;

	bool	bShowFPS;					///< Show the FPS

	bool    bShowTime;                  ///< Show how long this game is running

	bool    bCheatsEnabled;             ///< Cheat codes are enabled?

    bool	finished;                   ///< Is the game finished (won or lost) and we are just waiting for the end message to be shown
	bool	won;                        ///< If the game is finished, is it won or lost
	Uint32  finishedLevelTime;          ///< The time in milliseconds when the level was finished (won or lost)
	bool    finishedLevel;              ///< Set, when the game is really finished and the end message was shown

	GameInterface*	        pInterface;			                ///< This is the whole interface (top bar and side bar)
	InGameMenu*		        pInGameMenu;		                ///< This is the menu that is opened by the option button
	MentatHelp*		        pInGameMentat;		                ///< This is the mentat dialog opened by the mentat button
	WaitingForOtherPlayers* pWaitingForOtherPlayers;            ///< This is the dialog that pops up when we are waiting for other players during network hangs
	Uint32                  startWaitingForOtherPlayersTime;    ///< The time in milliseconds when we started waiting for other players

	bool    bSelectionChanged;                          ///< Has the selected list changed (and must be retransmitted to other plays in multiplayer games)
	std::list<Uint32> selectedList;                      ///< A set of all selected units/structures
	std::list<std::pair<Uint32,Coord>>	 selectedListCoord;				///< A list of all selected units/structures coordinates
	std::list<Uint32> selectedByOtherPlayerList;         ///< This is only used in multiplayer games where two players control one house
    RobustList<Explosion*> explosionList;               ///< A list containing all the explosions that must be drawn

    std::vector<House*> house;                          ///< All the houses of this game, index by their houseID; has the size NUM_HOUSES; unused houses are NULL

    std::multimap<std::string, Player*> playerName2Player;  ///< mapping player names to players (one entry per player)
    std::map<Uint8, Player*> playerID2Player;               ///< mapping player ids to players (one entry per player)



	ObjectBase* groupLeader;   				///< The first selected object is the group leader for formation movement
};

#endif // GAME_H
