#pragma once

// prototype for callback function:
typedef void(*UndoCallbackPtr)(int, void*, BOOL);

class CUndoItem
{
public:
	CUndoItem * prev;
	CUndoItem * next;
	int type;		// used by callback routine
	void * ptr;		// pointer to callback record
	UndoCallbackPtr callback;
};

class CUndoList
{
public:
	CUndoList( int max_items );
	~CUndoList(void);
	void Push( int type, void * ptr, UndoCallbackPtr callback );
	void * Pop();
	void NewEvent();
	void DropFirstEvent();
	void Clear();
	CUndoItem m_start;
	CUndoItem m_end;
	int m_max_items;
	int m_num_items;
	int m_num_events;
};
