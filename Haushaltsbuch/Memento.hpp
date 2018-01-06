#pragma once

#include "scoreline.hpp"

enum MEMENTO_CMD {
	MEMENTO_CMD_APPEND,
	MEMENTO_CMD_REMOVE
};

bool Memento_Redo();
bool Memento_Undo();
bool Memento_Redoable();
bool Memento_Undoable();

void Memento_Append(int cmd, SCORELINE_ITEM *item);

void Memento_Reset();
