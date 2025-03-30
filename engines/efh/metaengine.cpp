/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "engines/advancedDetector.h"
#include "common/system.h"
#include "common/savefile.h"
#include "common/textconsole.h"
#include "graphics/thumbnail.h"
#include "graphics/surface.h"
#include "backends/keymapper/action.h"
#include "backends/keymapper/keymapper.h"
#include "backends/keymapper/standard-actions.h"
#include "efh/efh.h"

namespace Efh {

uint32 EfhEngine::getFeatures() const {
	return _gameDescription->flags;
}

const char *EfhEngine::getGameId() const {
	return _gameDescription->gameId;
}

void EfhEngine::initGame(const ADGameDescription *gd) {
	_platform = gd->platform;
}

bool EfhEngine::hasFeature(EngineFeature f) const {
	return (f == kSupportsReturnToLauncher) || (f == kSupportsLoadingDuringRuntime) || (f == kSupportsSavingDuringRuntime);
}

const char *EfhEngine::getCopyrightString() const {
	return "Escape From Hell (C) Electronic Arts, 1990";
}

Common::Platform EfhEngine::getPlatform() const {
	return _platform;
}
} // End of namespace Efh

namespace Efh {

class EfhMetaEngine : public AdvancedMetaEngine<ADGameDescription> {
public:
	const char *getName() const override {
		return "efh";
	}

	Common::Error createInstance(OSystem *syst, Engine **engine, const ADGameDescription *gd) const override;
	bool hasFeature(MetaEngineFeature f) const override;

	int getMaximumSaveSlot() const override;
	SaveStateList listSaves(const char *target) const override;
	SaveStateDescriptor querySaveMetaInfos(const char *target, int slot) const override;
	bool removeSaveState(const char *target, int slot) const override;
};

Common::Error EfhMetaEngine::createInstance(OSystem *syst, Engine **engine, const ADGameDescription *gd) const {
	*engine = new EfhEngine(syst, gd);
	((EfhEngine *)*engine)->initGame(gd);
	return Common::kNoError;
}

bool EfhMetaEngine::hasFeature(MetaEngineFeature f) const {
	return
		(f == kSupportsListSaves) ||
		(f == kSupportsLoadingDuringStartup) ||
		(f == kSupportsDeleteSave) ||
		(f == kSavesSupportMetaInfo) ||
		(f == kSavesSupportThumbnail) ||
		(f == kSavesSupportCreationDate);
}

int EfhMetaEngine::getMaximumSaveSlot() const {
	return 99;
}

SaveStateList EfhMetaEngine::listSaves(const char *target) const {
	Common::SaveFileManager *saveFileMan = g_system->getSavefileManager();
	Common::String pattern = target;
	pattern += ".###";

	Common::StringArray filenames = saveFileMan->listSavefiles(pattern);

	SaveStateList saveList;
	char slot[3];
	for (const auto &filename : filenames) {
		slot[0] = filename.c_str()[filename.size() - 2];
		slot[1] = filename.c_str()[filename.size() - 1];
		slot[2] = '\0';
		// Obtain the last 2 digits of the filename (without extension), since they correspond to the save slot
		int slotNum = atoi(slot);
		if (slotNum >= 0 && slotNum <= getMaximumSaveSlot()) {
			Common::InSaveFile *file = saveFileMan->openForLoading(filename);
			if (file) {
				uint32 sign = file->readUint32LE();
				uint8 saveVersion = file->readByte();

				if (sign != EFH_SAVE_HEADER || saveVersion > kSavegameVersion) {
					warning("Incompatible savegame");
					delete file;
					continue;
				}

				// read name
				uint16 nameSize = file->readUint16LE();
				if (nameSize >= 255) {
					delete file;
					continue;
				}
				char name[256];
				file->read(name, nameSize);
				name[nameSize] = 0;

				saveList.push_back(SaveStateDescriptor(this, slotNum, name));
				delete file;
			}
		}
	}

	Common::sort(saveList.begin(), saveList.end(), SaveStateDescriptorSlotComparator());
	return saveList;
}

SaveStateDescriptor EfhMetaEngine::querySaveMetaInfos(const char *target, int slot) const {
	Common::String fileName = Common::String::format("%s.%03d", target, slot);
	Common::InSaveFile *file = g_system->getSavefileManager()->openForLoading(fileName);

	if (file) {
		uint32 sign = file->readUint32LE();
		uint8 saveVersion = file->readByte();

		if (sign != EFH_SAVE_HEADER || saveVersion > kSavegameVersion) {
			warning("Incompatible savegame");
			delete file;
			return SaveStateDescriptor();
		}

		uint32 saveNameLength = file->readUint16LE();
		Common::String saveName;
		for (uint32 i = 0; i < saveNameLength; ++i) {
			char curChr = file->readByte();
			saveName += curChr;
		}

		SaveStateDescriptor desc(this, slot, saveName);

		Graphics::Surface *thumbnail;
		if (!Graphics::loadThumbnail(*file, thumbnail)) {
			delete file;
			return SaveStateDescriptor();
		}
		desc.setThumbnail(thumbnail);

		// Read in save date/time
		int16 year = file->readSint16LE();
		int16 month = file->readSint16LE();
		int16 day = file->readSint16LE();
		int16 hour = file->readSint16LE();
		int16 minute = file->readSint16LE();
		desc.setSaveDate(year, month, day);
		desc.setSaveTime(hour, minute);

		desc.setDeletableFlag(slot != 0);
		desc.setWriteProtectedFlag(slot == 0);

		delete file;
		return desc;
	}
	return SaveStateDescriptor();
}

bool EfhMetaEngine::removeSaveState(const char *target, int slot) const {
	Common::String fileName = Common::String::format("%s.%03d", target, slot);
	return g_system->getSavefileManager()->removeSavefile(fileName);
}

Common::KeymapArray EfhMetaEngine::initKeymaps(const char *target) const {
	using namespace Common;
    using namespace Efh;

    Keymap *engineKeymap = new Keymap(Keymap::kKeymapTypeGame, "efh-default", "Default keymappings");

    Action *act;
   
    act = new Action("F1", _("Display Character Summary One"));	// This should map in F1 into the game to allow player to click on F1 to check CS(Character Summary) One.
    act->setCustomEngineActionEvent(Efh::kEfhActionCharacterSummaryOne);
    act->addDefaultInputMapping("F1");
    act->addDefaultInputMapping("1");
    engineKeymap->addAction(act);

    act = new Action("F2", _("Display Character Summary Two"));	// Should map in F2 to allow player to click F2 to check CS Two.
    act->setCustomEngineActionEvent(Efh::kEfhActionCharacterSummaryTwo);
    act->addDefaultInputMapping("F2");
    act->addDefaultInputMapping("2");
    engineKeymap->addAction(act);

    act = new Action("F3", _("Display Character Summary Three"));	// Should map in F3 to allow player to click F3 to check CS Three.
    act->setCustomEngineActionEvent(Efh::kEfhActionCharacterSummaryThree);
    act->addDefaultInputMapping("F3");
    act->addDefaultInputMapping("3");
    engineKeymap->addAction(act);

    act = new Action("SAVE", _("Save game"));	// Should map in CTRL-s to allow player to Save the game. I am not sure if this would be the correct inputs to allow the usage of CTRL button For Save, Load, and Buffer.
    act->setCustomEngineActionEvent(Efh::kEfhActionSave);
    act->addDefaultInputMapping("LCTRL+s");
	act->addDefaultInputMapping("RCTRL+s");
    engineKeymap->addAction(act);

    act = new Action("LOAD", _("Load game"));	// Should map in CTRL-l to allow player to Load the game.
    act->setCustomEngineActionEvent(Efh::kEfhActionLoad);
    act->addDefaultInputMapping("LCTRL+l");
	act->addDefaultInputMapping("RCTRL+l");
    engineKeymap->addAction(act);

    act = new Action("Buffer", _("Buffer"));	// Should map in CTRL-b to allow player to turn Buffering on/off. Buffering is off by default. (Base on the online efh manual I found online.)
    act->setCustomEngineActionEvent(Efh::kEfhActionBuffer);
    act->addDefaultInputMapping("LCTRL+b");
	act->addDefaultInputMapping("RCTRL+b");
    engineKeymap->addAction(act);

    act = new Action("Sound", _("Sound"));	// Should map in CTRL-o to allow player turn Sound on/off.
    act->setCustomEngineActionEvent(Efh::kEfhActionSound);
    act->addDefaultInputMapping("LCTRL+o");
	act->addDefaultInputMapping("RCTRL+o");
    engineKeymap->addAction(act);

    act = new Action("Increase", _("Increase delay of combat text"));	// Should map in + to increase combat text delay.
    act->setCustomEngineActionEvent(Efh::kEfhActionIncreaseCombatText);
    act->addDefaultInputMapping("+");
    engineKeymap->addAction(act);

    act = new Action("Decrease", _("Decrease delay of combat text"));	// Should map in - to decrease combat text delay.
    act->setCustomEngineActionEvent(Efh::kEfhActionDecreaseCombatText);
    act->addDefaultInputMapping("-");
    engineKeymap->addAction(act);

    act = new Action("A", _("A"));	// Should map in A into the game.
    act->setCustomEngineActionEvent(Efh::kEfhActionA);
    act->addDefaultInputMapping("a");
    engineKeymap->addAction(act);

    act = new Action("H",_( "H"));	// Should map in H into the game.
    act->setCustomEngineActionEvent(Efh::kEfhActionH);
    act->addDefaultInputMapping("h");
    engineKeymap->addAction(act);

    act = new Action("D", _("D"));	// Should map in D into the game.
    act->setCustomEngineActionEvent(Efh::kEfhActionD);
    act->addDefaultInputMapping("d");
    engineKeymap->addAction(act);

    act = new Action("R", _("R"));	// Should map in R into the game.
    act->setCustomEngineActionEvent(Efh::kEfhActionR);
    act->addDefaultInputMapping("r");
    engineKeymap->addAction(act);

    act = new Action("S", _("S"));	// Should map in S into the game.
    act->setCustomEngineActionEvent(Efh::kEfhActionS);
    act->addDefaultInputMapping("s");
    engineKeymap->addAction(act);

    act = new Action("T", _("T"));	// Should map in T into the game.
    act->setCustomEngineActionEvent(Efh::kEfhActionT);
    act->addDefaultInputMapping("t");
    engineKeymap->addAction(act);

    act = new Action("ESC", _("ESC"));	// Should map in ESC into the game.
    act->setCustomEngineActionEvent(Efh::kEfhActionESC);
    act->addDefaultInputMapping("KEYCODE_ESCAPE");
    engineKeymap->addAction(act);

    act = new Action("C", _("C"));	// Should map in C into the game.
    act->setCustomEngineActionEvent(Efh::kEfhActionC);
    act->addDefaultInputMapping("c");
    engineKeymap->addAction(act);

    act = new Action("Movement down", _("goSouth"));	// Should map in down arrow and kp2 into movement down/goSouth.
    act->setCustomEngineActionEvent(Efh::kEfhActionUp);
    act->addDefaultInputMapping("KEYCODE_DOWN");
    act->addDefaultInputMapping("KEYCODE_KP2");
    engineKeymap->addAction(act);

    act = new Action("Movement up", _("goNorth"));	// Should map in up arrow and kp8 into movement up/goNorth.
    act->setCustomEngineActionEvent(Efh::kEfhActionDown);
    act->addDefaultInputMapping("KEYCODE_UP");
    act->addDefaultInputMapping("KEYCODE_KP8");
    engineKeymap->addAction(act);

    act = new Action("Movement right", _("goEast"));	// Should map in right arrow and kp6 into movement right/goEast.
    act->setCustomEngineActionEvent(Efh::kEfhActionRight);
    act->addDefaultInputMapping("KEYCODE_RIGHT");
    act->addDefaultInputMapping("KEYCODE_KP6");
    engineKeymap->addAction(act);

    act = new Action("Movement left", _("goWest"));	// Should map in left arrow and kp4 into movement left/goWest.
    act->setCustomEngineActionEvent(Efh::kEfhActionLeft);
    act->addDefaultInputMapping("KEYCODE_LEFT");
    act->addDefaultInputMapping("KEYCODE_KP4");
    engineKeymap->addAction(act);

	act = new Action("Movement Down left", _("goSouthEast"));	// Should map in kp1 into movement goSouthEast.
    act->setCustomEngineActionEvent(Efh::kEfhActionDownLeft);
    act->addDefaultInputMapping("KEYCODE_KP1");
    engineKeymap->addAction(act);

	act = new Action("Movement Up left", _("goNorthEast"));	// Should map in kp7 into movement goNorthEast.
    act->setCustomEngineActionEvent(Efh::kEfhActionUpLeft);
    act->addDefaultInputMapping("KEYCODE_KP7");
    engineKeymap->addAction(act);

	act = new Action("Movement Down Right", _("goSouthWest"));	// Should map in kp3 into movement goSouthWest.
    act->setCustomEngineActionEvent(Efh::kEfhActionDownRight);
    act->addDefaultInputMapping("KEYCODE_KP3");
    engineKeymap->addAction(act);

	act = new Action("Movement Up Right", _("goNorthWest"));	// Should map in kp9 into movement goNorthWest.
    act->setCustomEngineActionEvent(Efh::kEfhActionUpRight);
    act->addDefaultInputMapping("KEYCODE_KP9");
    engineKeymap->addAction(act);

    return Keymap::arrayOf(engineKeymap);
}

} // End of namespace Efh

#if PLUGIN_ENABLED_DYNAMIC(EFH)
	REGISTER_PLUGIN_DYNAMIC(EFH, PLUGIN_TYPE_ENGINE, Efh::EfhMetaEngine);
#else
	REGISTER_PLUGIN_STATIC(EFH, PLUGIN_TYPE_ENGINE, Efh::EfhMetaEngine);
#endif
