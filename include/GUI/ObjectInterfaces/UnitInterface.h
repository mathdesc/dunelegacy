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

#ifndef DEFAULTUNITINTERFACE_H
#define DEFAULTUNITINTERFACE_H

#include "DefaultObjectInterface.h"

#include <globals.h>

#include <FileClasses/GFXManager.h>
#include <FileClasses/TextManager.h>

#include <GUI/TextButton.h>
#include <GUI/SymbolButton.h>
#include <GUI/HBox.h>
#include <GUI/VBox.h>
#include <GUI/Label.h>

#include <units/UnitBase.h>
#include <units/MCV.h>
#include <units/Harvester.h>
#include <units/Devastator.h>

class UnitInterface : public DefaultObjectInterface {
public:
	static UnitInterface* create(int objectID) {
		UnitInterface* tmp = new UnitInterface(objectID);
		tmp->pAllocated = true;
		return tmp;
	}

protected:
	UnitInterface(int objectID) : DefaultObjectInterface(objectID) {
        int color = houseColor[pLocalHouse->getHouseID()];
        ObjectBase* pObject = currentGame->getObjectManager().getObject(objectID);
        int cmp_button_size = ((int)ceil(settings.video.height/22));
         if (((int)ceil(settings.video.height/22))%2 != 0) {
        	 cmp_button_size = ((int)ceil(settings.video.height/22))+1;
         }
        int buttonVBoxsize = cmp_button_size;

		mainHBox.addWidget(HSpacer::create(5));

		harvesterExtractionLabel.setTextFont(FONT_STD10);
		harvesterExtractionLabel.setTextColor(color+3);
		textVBox.addWidget(&harvesterExtractionLabel, 0.99);
		textVBox.addWidget(HSpacer::create(5), 0.99);
		harvesterProspectionLabel.setTextFont(FONT_STD10);
		harvesterProspectionLabel.setTextColor(color+3);
		textVBox.addWidget(&harvesterProspectionLabel, 0.99);
		textVBox.addWidget(HSpacer::create(5), 0.99);
		harvesterProspectionShortestLabel.setTextFont(FONT_STD10);
		harvesterProspectionShortestLabel.setTextColor(color+3);
		harvesterProspectionShortestLabel.setTooltipText(_("Shortest sample location, extraction speed"));
		harvesterProspectionShortestButton.setSymbol(pGFXManager->getUIGraphic(UI_CursorMove_Zoomlevel0), false);
		harvesterProspectionShortestButton.setToggleButton(false);
		harvesterProspectionShortestButton.setOnClick(std::bind(&UnitInterface::onPreference, this, DISTANCE));
		harvesterProspectionShortestButton.setTooltipText(_("Prefer Shortest distance patch"));
		textVBox.addWidget(&harvesterProspectionShortestLabel, 0.99);
		textVBox.addWidget(&harvesterProspectionShortestButton, 0.99);
		textVBox.addWidget(HSpacer::create(5), 0.99);
		harvesterProspectionDensiestLabel.setTextFont(FONT_STD10);
		harvesterProspectionDensiestLabel.setTextColor(color+3);
		harvesterProspectionDensiestLabel.setTooltipText(_("Densiest sample location, extraction speed"));
		harvesterProspectionDensiestButton.setSymbol(pGFXManager->getUIGraphic(UI_CursorMove_Zoomlevel0), false);
		harvesterProspectionDensiestButton.setToggleButton(false);
		harvesterProspectionDensiestButton.setOnClick(std::bind(&UnitInterface::onPreference, this, DENSITY));
		harvesterProspectionDensiestButton.setTooltipText(_("Prefer Density patch"));
		textVBox.addWidget(&harvesterProspectionDensiestLabel, 0.99);
		textVBox.addWidget(&harvesterProspectionDensiestButton, 0.99);
		textVBox.addWidget(HSpacer::create(5), 0.99);
		buttonVBox.addWidget(&textVBox,0.99);


		buttonVBox.addWidget(VSpacer::create(20));

        moveButton.setSymbol(pGFXManager->getUIGraphic(UI_CursorMove_Zoomlevel0), false);
		moveButton.setTooltipText(_("Move to a position (Hotkey: M)"));
		moveButton.setToggleButton(true);
		moveButton.setOnClick(std::bind(&UnitInterface::onMove, this));
		actionHBox.addWidget(&moveButton);

		actionHBox.addWidget(HSpacer::create(3));

		attackButton.setSymbol(pGFXManager->getUIGraphic(UI_CursorAttack_Zoomlevel0), false);
		attackButton.setTooltipText(_("Attack a unit, structure or position (Hotkey: A)"));
		attackButton.setToggleButton(true);
		attackButton.setOnClick(std::bind(&UnitInterface::onAttack, this));
		actionHBox.addWidget(&attackButton);

        actionHBox.addWidget(HSpacer::create(3));

        captureButton.setSymbol(pGFXManager->getUIGraphic(UI_CursorCapture_Zoomlevel0), false);
        captureButton.setTooltipText(_("Capture a building (Hotkey: C)"));
        captureButton.setVisible(pObject != NULL && pObject->canCapture());
		captureButton.setToggleButton(true);
		captureButton.setOnClick(std::bind(&UnitInterface::onCapture, this));
		actionHBox.addWidget(&captureButton);

		actionHBox.addWidget(HSpacer::create(3));

		SattackButton.setSymbol(pGFXManager->getUIGraphic(UI_CursorSalveAttack_Zoomlevel0), false);
		SattackButton.setTooltipText(_("Salvo attack a unit, structure or position (Hotkey: S)"));
		SattackButton.setVisible(pObject != NULL && pObject->canSalveAttack());
		SattackButton.setToggleButton(true);
		SattackButton.setOnClick(std::bind(&UnitInterface::onSalveAttack, this));
		actionHBox.addWidget(&SattackButton);

		buttonVBox.addWidget(&actionHBox, buttonVBoxsize);

		buttonVBox.addWidget(VSpacer::create(3));

        returnButton.setSymbol(pGFXManager->getUIGraphic(UI_ReturnIcon), false);
        returnButton.setTooltipText(_("Return harvester to refinery (Hotkey: R)"));
        returnButton.setVisible( (itemID == Unit_Harvester) );
		returnButton.setOnClick(std::bind(&UnitInterface::onReturn, this));
		commandHBox.addWidget(&returnButton);

		commandHBox.addWidget(HSpacer::create(3));

		deployButton.setSymbol(pGFXManager->getUIGraphic(UI_DeployIcon), false);
		deployButton.setTooltipText(_("Build a new construction yard"));
        deployButton.setVisible( (itemID == Unit_MCV) );
		deployButton.setOnClick(std::bind(&UnitInterface::onDeploy, this));
		commandHBox.addWidget(&deployButton);

        commandHBox.addWidget(HSpacer::create(3));

        destructButton.setSymbol(pGFXManager->getUIGraphic(UI_DestructIcon), false);
        destructButton.setTooltipText(_("Self-destruct this unit"));
		destructButton.setVisible( (itemID == Unit_Devastator) );
		destructButton.setOnClick(std::bind(&UnitInterface::onDestruct, this));
		commandHBox.addWidget(&destructButton);

		buttonVBox.addWidget(&commandHBox, buttonVBoxsize);

		buttonVBox.addWidget(VSpacer::create(6));

        guardButton.setText(_("Guard"));
        guardButton.setTextColor(color+3);
		guardButton.setTooltipText(_("Unit will not move from location"));
		guardButton.setToggleButton(true);
		guardButton.setOnClick(std::bind(&UnitInterface::onGuard, this));
		buttonVBox.addWidget(&guardButton, buttonVBoxsize);

		buttonVBox.addWidget(VSpacer::create(6));

		areaGuardButton.setText(_("Area Guard"));
        areaGuardButton.setTextColor(color+3);
		areaGuardButton.setTooltipText(_("Unit will engage any unit within guard range"));
		areaGuardButton.setToggleButton(true);
		areaGuardButton.setOnClick(std::bind(&UnitInterface::onAreaGuard, this));
		buttonVBox.addWidget(&areaGuardButton, buttonVBoxsize);

		buttonVBox.addWidget(VSpacer::create(6));

		stopButton.setText(_("Stop"));
        stopButton.setTextColor(color+3);
		stopButton.setTooltipText(_("Unit will not move, nor attack"));
		stopButton.setToggleButton(true);
		stopButton.setOnClick(std::bind(&UnitInterface::onStop, this));
		buttonVBox.addWidget(&stopButton, buttonVBoxsize);

		buttonVBox.addWidget(VSpacer::create(6));

        ambushButton.setText(_("Ambush"));
        ambushButton.setTextColor(color+3);
		ambushButton.setTooltipText(_("Unit will not move until enemy unit spotted"));
		ambushButton.setToggleButton(true);
		ambushButton.setOnClick(std::bind(&UnitInterface::onAmbush, this));
		buttonVBox.addWidget(&ambushButton, buttonVBoxsize);

		buttonVBox.addWidget(VSpacer::create(6));

        huntButton.setText(_("Hunt"));
        huntButton.setTextColor(color+3);
		huntButton.setTooltipText(_("Unit will immediately start to engage an enemy unit"));
		huntButton.setToggleButton(true);
		huntButton.setOnClick(std::bind(&UnitInterface::onHunt, this));
		buttonVBox.addWidget(&huntButton, buttonVBoxsize);

		buttonVBox.addWidget(VSpacer::create(6));
		buttonVBox.addWidget(Spacer::create());
		buttonVBox.addWidget(VSpacer::create(6));

		mainHBox.addWidget(&buttonVBox);
		mainHBox.addWidget(HSpacer::create(5));

		update();
	}

	void onPreference(HarvesterPreference pref) {
		ObjectBase* pObject = currentGame->getObjectManager().getObject(objectID);
		Harvester* pHarvester = dynamic_cast<Harvester*>(pObject);
		if(pHarvester != NULL) {
			fprintf(stderr,"UnitInterface::onPreference %d\n",pref);
			pHarvester->setPreference(pref);
		}
	}
    void onMove() {
        currentGame->currentCursorMode = Game::CursorMode_Move;
	}

	void onAttack() {
        currentGame->currentCursorMode = Game::CursorMode_Attack;
	}

	void onSalveAttack() {
        currentGame->currentCursorMode = Game::CursorMode_SalveAttack;
	}

    void onCapture() {
        currentGame->currentCursorMode = Game::CursorMode_Capture;
	}

	void onReturn() {
		ObjectBase* pObject = currentGame->getObjectManager().getObject(objectID);
		Harvester* pHarvester = dynamic_cast<Harvester*>(pObject);
		if(pHarvester != NULL) {
			pHarvester->handleReturnClick();
		}
	}

	void onDeploy() {
		ObjectBase* pObject = currentGame->getObjectManager().getObject(objectID);
		MCV* pMCV = dynamic_cast<MCV*>(pObject);
		if(pMCV != NULL) {
			pMCV->handleDeployClick();
		}
	}

	void onDestruct() {
		ObjectBase* pObject = currentGame->getObjectManager().getObject(objectID);
		Devastator* pDevastator = dynamic_cast<Devastator*>(pObject);
		if(pDevastator != NULL) {
			pDevastator->handleStartDevastateClick();
		}
	}

    void onGuard() {
		setAttackMode(GUARD);
	}

	void onAreaGuard() {
		setAttackMode(AREAGUARD);
	}

    void onStop() {
		setAttackMode(STOP);
	}

	void onAmbush() {
		setAttackMode(AMBUSH);
	}

    void onHunt() {
		setAttackMode(HUNT);
	}

	void setAttackMode(ATTACKMODE newAttackMode) {
		ObjectBase* pObject = currentGame->getObjectManager().getObject(objectID);
		UnitBase* pUnit = dynamic_cast<UnitBase*>(pObject);

		if(pUnit != NULL) {
			pUnit->handleSetAttackModeClick(newAttackMode);
			pUnit->playConfirmSound();

			update();
		}
	}

	/**
		This method updates the object interface.
		If the object doesn't exists anymore then update returns false.
		\return true = everything ok, false = the object container should be removed
	*/
	virtual bool update() {
	    ObjectBase* pObject = currentGame->getObjectManager().getObject(objectID);
		if(pObject == NULL) {
			return false;
		}
		UnitBase* pUnit = dynamic_cast<UnitBase*>(pObject);

		if (pUnit->getItemID() == Unit_Harvester) {
			Harvester* pHarv = (Harvester*)(pObject);
			Tile * pTile = currentGameMap->getTile(pHarv->getLocation());
			float speed 	= pTile->hasSpice() ? pTile->getSpiceExtractionSpeed(false,true): 0;
			float ref_speed = pTile->hasSpice() ? pTile->getSpiceExtractionSpeed(true,true): 1;
			harvesterExtractionLabel.setText( " " + _("Ext. Speed") + ": " + stringify((int)((speed/ref_speed)*100))+ "%" );
			if (pHarv->getProspectionSamples().size() > 0 ) {
				std::pair<Coord,int> shortest_site = pHarv->getProspectionSampleBestDistance();
				std::pair<Coord,int> densest_site = pHarv->getProspectionSampleBestDensity();
				harvesterProspectionLabel.setText( " " + _("Prospect.") + ": " + stringify( pHarv->getProspectionSamples().size() ) );
				harvesterProspectionShortestLabel.setText( stringify( shortest_site.first.x) + "," + stringify(shortest_site.first.y) + " @ " +
															stringify( shortest_site.second ) + "%");

				harvesterProspectionDensiestLabel.setText( stringify( densest_site.first.x) + "," + stringify(densest_site.first.y) + " @ " +
															stringify( densest_site.second ) + "%");
				harvesterProspectionShortestLabel.setVisible(true);
				harvesterProspectionDensiestLabel.setVisible(true);
			} else {
				harvesterProspectionLabel.setText( " " + _("Prospect.") + ": " +_("None") );
				harvesterProspectionShortestLabel.setVisible(false);
				harvesterProspectionDensiestLabel.setVisible(false);
			}

			harvesterExtractionLabel.setVisible(true);
			harvesterProspectionLabel.setVisible(true);
		} else {
			harvesterExtractionLabel.setVisible(false);
			harvesterProspectionLabel.setVisible(false);
		}

        moveButton.setToggleState(currentGame->currentCursorMode == Game::CursorMode_Move);
		attackButton.setToggleState(currentGame->currentCursorMode == Game::CursorMode_Attack);
		if (pObject->canSalveAttack()) {
			SattackButton.setToggleState(currentGame->currentCursorMode == Game::CursorMode_SalveAttack || ( pUnit != NULL && pUnit->isSalving() ));
		}
		attackButton.setVisible(pObject->canAttack());
		if (pObject->canCapture()) {
			captureButton.setToggleState(currentGame->currentCursorMode == Game::CursorMode_Capture);
			captureButton.setVisible(true);
		}

		if(pUnit != NULL) {
			ATTACKMODE AttackMode = pUnit->getAttackMode();

			guardButton.setToggleState( AttackMode == GUARD );
            areaGuardButton.setToggleState( AttackMode == AREAGUARD );
            stopButton.setToggleState( AttackMode == STOP );
            ambushButton.setToggleState( AttackMode == AMBUSH );
            huntButton.setToggleState( AttackMode == HUNT );
		}

		return true;
	}

	HBox		    buttonHBox;
	VBox		    buttonVBox;
	HBox            actionHBox;
	HBox            commandHBox;

	VBox    		textVBox;
	Label   		harvesterExtractionLabel;
	VBox    		textProspectionVBox;
	Label   		harvesterProspectionLabel;
	Label   		harvesterProspectionDensiestLabel;
	Label 			harvesterProspectionShortestLabel;
	SymbolButton    harvesterProspectionDensiestButton;
	SymbolButton    harvesterProspectionShortestButton;


	SymbolButton    moveButton;
	SymbolButton    attackButton;
	SymbolButton    SattackButton;
    SymbolButton    captureButton;
    SymbolButton    returnButton;
    SymbolButton    deployButton;
    SymbolButton    destructButton;

    TextButton	    guardButton;
	TextButton	    areaGuardButton;
	TextButton      stopButton;
	TextButton	    ambushButton;
	TextButton      huntButton;
};

#endif //DEFAULTUNITINTERFACE_H
