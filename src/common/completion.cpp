//=============================================================================//
//
// Purpose: Completion functions for ConCommand callbacks.
//
//=============================================================================//
#include "core/stdafx.h"
#include "public/iconvar.h"
#include "engine/cmodel_bsp.h"
#include "tier1/strtools.h"
#include "completion.h"
#include "vstdlib/autocompletefilelist.h"

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *partial - 
//			context - 
//			longest - 
//			maxcommands - 
//			**commands - 
// Output : int
//-----------------------------------------------------------------------------
int _Host_Map_f_CompletionFunc(char const* cmdname, char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	char* substring = (char*)partial;
	if (strstr(partial, cmdname))
	{
		substring = (char*)partial + strlen(cmdname);
	}

	const int mapcount = g_InstalledMaps.Count();
	const int longest = COMMAND_COMPLETION_ITEM_LENGTH;
	const int count = MIN(mapcount, COMMAND_COMPLETION_MAXITEMS);

	int filtered_count = 0;
	if (count > 0)
	{
		for (int i = 0; i < count; i++)
		{
			if (strstr(g_InstalledMaps[i].String(), substring))
			{
				strncpy(commands[filtered_count], g_InstalledMaps[i].String(), longest);

				char old[COMMAND_COMPLETION_ITEM_LENGTH];
				strncpy(old, commands[filtered_count], sizeof(old));

				snprintf(commands[filtered_count], sizeof(commands[filtered_count]), "%s%s", cmdname, old);
				commands[filtered_count][strlen(commands[filtered_count])] = '\0';

				filtered_count++;
			}
		}
	}

	return filtered_count;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *autocomplete - 
//			*partial - 
//			context - 
//			longest - 
//			maxcommands - 
//			**commands - 
// Output : int
//-----------------------------------------------------------------------------
int _Host_Pak_f_CompletionFunc(CBaseAutoCompleteFileList* autocomplete, char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	int count = autocomplete->AutoCompletionFunc(partial, commands);
	if (count > 0)
	{
		for (size_t i = 0; i < count; i++)
		{
			size_t cmdsize = strlen(commands[i]);
			if (cmdsize < COMMAND_COMPLETION_ITEM_LENGTH - 5)
			{
				snprintf(&commands[i][cmdsize], 5, "%s", "rpak");
			}
			else
			{
				snprintf(commands[i], COMMAND_COMPLETION_ITEM_LENGTH, "%s", "BUFFER_TOO_SMALL!");
			}
		}
	}

	return count;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *partial - 
//			**commands - 
// Output : int
//-----------------------------------------------------------------------------
int Host_SSMap_f_CompletionFunc(char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	char const* cmdname = "ss_map ";
	return _Host_Map_f_CompletionFunc(cmdname, partial, commands);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *partial - 
//			**commands - 
// Output : int
//-----------------------------------------------------------------------------
int Host_Map_f_CompletionFunc(char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	char const* cmdname = "map ";
	return _Host_Map_f_CompletionFunc(cmdname, partial, commands);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *partial - 
//			**commands - 
// Output : int
//-----------------------------------------------------------------------------
int Host_Background_f_CompletionFunc(char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	char const* cmdname = "map_background ";
	return _Host_Map_f_CompletionFunc(cmdname, partial, commands);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *partial - 
//			**commands - 
// Output : int
//-----------------------------------------------------------------------------
int Host_Changelevel_f_CompletionFunc(char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	char const* cmdname = "changelevel ";
	return _Host_Map_f_CompletionFunc(cmdname, partial, commands);
}

static CBaseAutoCompleteFileList s_GiveAutoFileList("give", "scripts/weapons", "txt");
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *partial - 
//			**commands - 
// Output : int
//-----------------------------------------------------------------------------
int Game_Give_f_CompletionFunc(char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	return s_GiveAutoFileList.AutoCompletionFunc(partial, commands);
}

static CBaseAutoCompleteFileList s_PakLoadAutoFileList("pak_requestload", "paks/Win64", "rpak");
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *partial - 
//			**commands - 
// Output : int
//-----------------------------------------------------------------------------
int RTech_PakLoad_f_CompletionFunc(char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	return _Host_Pak_f_CompletionFunc(&s_PakLoadAutoFileList, partial, commands);
}

static CBaseAutoCompleteFileList s_PakUnloadAutoFileList("pak_requestunload", "paks/Win64", "rpak");
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *partial - 
//			**commands - 
// Output : int
//-----------------------------------------------------------------------------
int RTech_PakUnload_f_CompletionFunc(char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	return _Host_Pak_f_CompletionFunc(&s_PakUnloadAutoFileList, partial, commands);
}

static CBaseAutoCompleteFileList s_PakSwapAutoFileList("pak_requestswap", "paks/Win64", "rpak");
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *partial - 
//			**commands - 
// Output : int
//-----------------------------------------------------------------------------
int RTech_PakSwap_f_CompletionFunc(char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	return _Host_Pak_f_CompletionFunc(&s_PakSwapAutoFileList, partial, commands);
}

static CBaseAutoCompleteFileList s_PakCompress("pak_compress", "paks/Win64_override", "rpak");
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *partial - 
//			**commands - 
// Output : int
//-----------------------------------------------------------------------------
int RTech_PakCompress_f_CompletionFunc(char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	return _Host_Pak_f_CompletionFunc(&s_PakCompress, partial, commands);
}

static CBaseAutoCompleteFileList s_PakDecompress("pak_decompress", "paks/Win64", "rpak");
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *partial - 
//			**commands - 
// Output : int
//-----------------------------------------------------------------------------
int RTech_PakDecompress_f_CompletionFunc(char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	return _Host_Pak_f_CompletionFunc(&s_PakDecompress, partial, commands);
}
