#pragma once

#include "lc_array.h"
#include "lc_math.h"

#define LC_MAX_COLOR_NAME 64
#define LC_COLOR_DIRECT 0x80000000

struct lcColor
{
	quint32 Code;
	bool Translucent;
	lcVector4 Value;
	lcVector4 Edge;
	char Name[LC_MAX_COLOR_NAME];
	char SafeName[LC_MAX_COLOR_NAME];
/*** LPub3D Mod - use 3DViewer colors ***/
	int CValue;
	int EValue;
	int Alpha;
/*** LPub3D Mod end ***/
};

enum
{
	LC_COLORGROUP_SOLID,
	LC_COLORGROUP_TRANSLUCENT,
	LC_COLORGROUP_SPECIAL,
/*** LPub3D Mod - LPub3D color group ***/
	LC_COLORGROUP_LPUB3D,
/*** LPub3D Mod end ***/
	LC_NUM_COLORGROUPS
};

struct lcColorGroup
{
	lcArray<int> Colors;
	QString Name;
};

enum lcInterfaceColor
{
	LC_COLOR_SELECTED,
	LC_COLOR_FOCUSED,
	LC_COLOR_DISABLED,
	LC_COLOR_CAMERA,
	LC_COLOR_LIGHT,
	LC_COLOR_CONTROL_POINT,
	LC_COLOR_CONTROL_POINT_FOCUSED,
	LC_COLOR_HIGHLIGHT,
	LC_NUM_INTERFACECOLORS
};

extern lcVector4 gInterfaceColors[LC_NUM_INTERFACECOLORS];
extern lcArray<lcColor> gColorList;
extern lcColorGroup gColorGroups[LC_NUM_COLORGROUPS];
extern int gNumUserColors;
extern int gEdgeColor;
extern int gDefaultColor;

void lcLoadDefaultColors();
bool lcLoadColorFile(lcFile& File);
/*** LPub3D Mod - load color entry ***/
bool lcLoadColorEntry(const char* ColorEntry);
/*** LPub3D Mod end ***/
int lcGetColorIndex(quint32 ColorCode);
int lcGetBrickLinkColor(int ColorIndex);

inline quint32 lcGetColorCodeFromExtendedColor(int Color)
{
	const int ConverstionTable[] = { 4, 12, 2, 10, 1, 9, 14, 15, 8, 0, 6, 13, 13, 334, 36, 44, 34, 42, 33, 41, 46, 47, 7, 382, 6, 13, 11, 383 };
	return ConverstionTable[Color];
}

inline quint32 lcGetColorCodeFromOriginalColor(int Color)
{
	const int ConverstionTable[] = { 0, 2, 4, 9, 7, 6, 22, 8, 10, 11, 14, 16, 18, 9, 21, 20, 22, 8, 10, 11 };
	return lcGetColorCodeFromExtendedColor(ConverstionTable[Color]);
}

inline quint32 lcGetColorCode(int ColorIndex)
{
	return gColorList[ColorIndex].Code;
}

inline bool lcIsColorTranslucent(int ColorIndex)
{
	return gColorList[ColorIndex].Translucent;
}

