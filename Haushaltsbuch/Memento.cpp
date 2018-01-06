#include "pch.hpp"
#include "Memento.hpp"
#include "ScoreLine.hpp"

#define MINIMAL_USE_PROCESSHEAPARRAY
#include "MinimalArray.hpp"

struct MEMENTO_ITEM {
	int cmd;
	SCORELINE_ITEM item;
};

static Minimal::ProcessHeapArrayT<MEMENTO_ITEM> s_undoStack;
static Minimal::ProcessHeapArrayT<MEMENTO_ITEM> s_redoStack;

bool Memento_Redoable()
{
	return s_redoStack.GetSize() != 0;

}

bool Memento_Undoable()
{
	return s_undoStack.GetSize() != 0;
}

bool Memento_Redo()
{
	if (s_redoStack.GetSize() == 0)
		return false;

	ScoreLine_Enter();

	int top = s_redoStack.GetSize() - 1;
	if (s_redoStack[top].cmd == MEMENTO_CMD_APPEND) {
		if (!ScoreLine_Append(&s_redoStack[top].item)) {
			ScoreLine_Leave(true);
			return false;
		}
	} else {
		if (!ScoreLine_Remove(s_redoStack[top].item.timestamp)) {
			ScoreLine_Leave(true);
			return false;
		}
	}

	s_undoStack.Push(s_redoStack[top]);
	s_redoStack.Pop();

	ScoreLine_Leave(false);
	return true;
}

bool Memento_Undo()
{
	if (s_undoStack.GetSize() == 0)
		return false;

	ScoreLine_Enter();

	int top = s_undoStack.GetSize() - 1;
	if (s_undoStack[top].cmd == MEMENTO_CMD_APPEND) {
		if (!ScoreLine_Remove(s_undoStack[top].item.timestamp)) {
			ScoreLine_Leave(true);
			return false;
		}
	} else {
		if (!ScoreLine_Append(&s_undoStack[top].item)) {
			ScoreLine_Leave(true);
			return false;
		}
	}

	s_redoStack.Push(s_undoStack[top]);
	s_undoStack.Pop();

	ScoreLine_Leave(false);
	return true;
}

void Memento_Reset()
{
	s_redoStack.Clear();
	s_undoStack.Clear();
}

void Memento_Append(int cmd, SCORELINE_ITEM *item)
{
	MEMENTO_ITEM mmtItem = { cmd, *item };
	s_redoStack.Clear();
	s_undoStack.Push(mmtItem);
}
