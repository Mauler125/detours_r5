#pragma once
#include "public/iconvar.h"
#include "vstdlib/autocompletefilelist.h"

int Host_SSMap_f_CompletionFunc(char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);
int Host_Map_f_CompletionFunc(char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);
int Host_Background_f_CompletionFunc(char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);
int Host_Changelevel_f_CompletionFunc(char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);

int Game_Give_f_CompletionFunc(char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);

int RTech_PakLoad_f_CompletionFunc(char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);
int RTech_PakUnload_f_CompletionFunc(char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);
int RTech_PakSwap_f_CompletionFunc(char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);
int RTech_PakCompress_f_CompletionFunc(char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);
int RTech_PakDecompress_f_CompletionFunc(char const* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);

inline int(*CBaseAutoCompleteFileList__AutoCompletionFunc)
(CBaseAutoCompleteFileList* thisp, const char* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);

///////////////////////////////////////////////////////////////////////////////
class VCompletion : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CBaseAutoCompleteFileList::AutoCompletionFunc", CBaseAutoCompleteFileList__AutoCompletionFunc);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 8B C4 4C 89 40 18 55 41 54").GetPtr(CBaseAutoCompleteFileList__AutoCompletionFunc);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////
