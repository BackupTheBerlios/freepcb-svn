// flags for options, etc.

enum {
	IMPORT_PARTS		= 0x1,
	IMPORT_NETS			= 0x2,
	KEEP_PARTS_AND_CON	= 0x4,
	KEEP_PARTS_NO_CON	= 0x8,
	KEEP_FP				= 0x10,
	KEEP_NETS			= 0x20
};

// netlist return flags
enum {
	FOOTPRINTS_NOT_FOUND = 1
};
// undo record types
enum {
	UNDO_BOARD_MODIFY = 1,	// board outline is about to be modified
	UNDO_BOARD_ADD,			// board outline is about to be added
	UNDO_BOARD_DELETE,		// board outline is about to be deleted
	UNDO_SM_CUTOUT_NONE,	// the first cutout is about to be added 
	UNDO_SM_CUTOUT_LAST,	// flags the last cutout in the undo event
	UNDO_SM_CUTOUT			// cutouts will be modified
};
