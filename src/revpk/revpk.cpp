//=============================================================================//
//
// Purpose: Standalone VPK tool
//
//=============================================================================//
#include "core/logdef.h"
#include "core/logger.h"
#include "tier0/fasttimer.h"
#include "tier0/cpu.h"
#include "tier1/cmd.h"
#include "tier1/fmtstr.h"
#include "tier1/keyvalues.h"
#include "windows/console.h"
#include "vpklib/packedstore.h"

#include "public/const.h"
#include "localize/ilocalize.h"
#include "vstdlib/keyvaluessystem.h"
#include "filesystem/filesystem_std.h"

#define PACK_COMMAND "pack"
#define UNPACK_COMMAND "unpack"

#define PACK_LOG_DIR "manifest/pack_logs/"
#define UNPACK_LOG_DIR "manifest/unpack_logs/"

#define FRONTEND_ENABLE_FILE "enable.txt"

static CKeyValuesSystem s_KeyValuesSystem;
static CFileSystem_Stdio s_FullFileSystem;
static bool s_bUseAnsiColors = true;

//-----------------------------------------------------------------------------
// Purpose: keyvalues singleton accessor
//-----------------------------------------------------------------------------
IKeyValuesSystem* KeyValuesSystem()
{
    return &s_KeyValuesSystem;
}

//-----------------------------------------------------------------------------
// Purpose: filesystem singleton accessor
//-----------------------------------------------------------------------------
CFileSystem_Stdio* FileSystem()
{
    return &s_FullFileSystem;
}

//-----------------------------------------------------------------------------
// Purpose: init
//-----------------------------------------------------------------------------
static void ReVPK_Init()
{
    CheckSystemCPUForSSE2();

    // Init time.
    Plat_FloatTime();

    g_CoreMsgVCallback = EngineLoggerSink;
    lzham_enable_fail_exceptions(true);

    if (s_bUseAnsiColors)
        Console_ColorInit();

    SpdLog_Init(s_bUseAnsiColors);
}

//-----------------------------------------------------------------------------
// Purpose: shutdown
//-----------------------------------------------------------------------------
static void ReVPK_Shutdown()
{
    // Must be done to flush all buffers.
    SpdLog_Shutdown();
    Console_Shutdown();
}

//-----------------------------------------------------------------------------
// Purpose: logs tool's usage
//-----------------------------------------------------------------------------
static void ReVPK_Usage()
{
    CFmtStr1024 usage;

    usage.Format(
        "ReVPK instructions and options:\n"
        "For packing; run 'revpk %s' with the following parameters:\n"
        "\t<%s>\t- locale prefix for the directory file ( defaults to \"%s\" )\n"
        "\t<%s>\t- context scope for the VPK files [\"%s\", \"%s\"]\n"
        "\t<%s>\t- level name for the VPK files\n"
        "\t<%s>\t- ( optional ) path to the workspace containing the manifest file\n"
        "\t<%s>\t- ( optional ) path in which the VPK files will be built\n"
        "\t<%s>\t- ( optional ) max LZHAM helper threads [\"%d\", \"%d\"] \"%d\" ( default ) for max practical\n"
        "\t<%s>\t- ( optional ) the level of compression [\"%s\", \"%s\", \"%s\", \"%s\", \"%s\"]\n\n"

        "For unpacking; run 'revpk %s' with the following parameters:\n"
        "\t<%s>\t- path and name of the target VPK files\n"
        "\t<%s>\t- ( optional ) path in which the VPK files will be unpacked\n"
        "\t<%s>\t- ( optional ) whether to parse the directory file name from the pack file name\n",

        PACK_COMMAND, // Pack parameters:
        "locale", g_LanguageNames[0],
        "context", g_GameDllTargets[0], g_GameDllTargets[1],
        
        "levelName", "workspacePath", "buildPath",
        
        "numThreads", // Num helper threads.
        -1, LZHAM_MAX_HELPER_THREADS, -1,
        
        "compressLevel", // Compress level.
        "fastest", "faster", "default", "better", "uber",

        UNPACK_COMMAND,// Unpack parameters:
        "fileName", "outPath", "sanitize"
    );

    Warning(eDLL_T::FS, "%s", usage.Get());
}

//-----------------------------------------------------------------------------
// Purpose: writes the VPK front-end enable file
//-----------------------------------------------------------------------------
static void ReVPK_WriteEnableFile(const char* containingPath)
{
    CFmtStr1024 textFileName;
    textFileName.Format("%s%s", containingPath, FRONTEND_ENABLE_FILE);

    if (FileSystem()->FileExists(textFileName.String(), "PLATFORM"))
    {
        // Already exists.
        return;
    }

    FileHandle_t enableTxt = FileSystem()->Open(textFileName.String(), "wb", "PLATFORM");

    if (enableTxt)
    {
        const char* textData = "1 \r\n";
        FileSystem()->Write(textData, strlen(textData), enableTxt);
        FileSystem()->Close(enableTxt);
    }
    else
    {
        Error(eDLL_T::FS, NO_ERROR, "Failed to write front-end enable file \"%s\"; insufficient rights?\n",
            textFileName.String());
    }
}

//-----------------------------------------------------------------------------
// Purpose: packs VPK files into 'SHIP' VPK directory
//-----------------------------------------------------------------------------
static void ReVPK_Pack(const CCommand& args)
{
    const int argCount = args.ArgC();

    if (argCount < 5)
    {
        ReVPK_Usage();
        return;
    }

    CFmtStr1024 workspacePath = argCount > 5 ? args.Arg(5) : "ship/";
    CFmtStr1024 buildPath     = argCount > 6 ? args.Arg(6) : "vpk/";

    // Make sure there's always a trailing slash.
    V_AppendSlash(workspacePath.Access(), workspacePath.GetMaxLength(), '/');
    V_AppendSlash(buildPath.Access(), buildPath.GetMaxLength(), '/');

    if (!FileSystem()->IsDirectory(workspacePath, "PLATFORM"))
    {
        Error(eDLL_T::FS, NO_ERROR, "Workspace path \"%s\" doesn't exist!\n", workspacePath.String());
        return;
    }

    const char* localeName = args.Arg(2);
    const char* contextName = args.Arg(3);
    const char* levelName = args.Arg(4);

    // For clients, we need an enable file which the engine uses to determine
    // whether or not to mount the front-end VPK file.
    if (V_strcmp(contextName, g_GameDllTargets[EPackedStoreTargets::STORE_TARGET_CLIENT]) == NULL)
    {
        ReVPK_WriteEnableFile(buildPath);
    }

    // Write the pack log to a file.
    CFmtStr1024 textFileName("%s%s%s%s_%s.log", buildPath.String(), PACK_LOG_DIR, localeName, contextName, levelName);
    SpdLog_InstallSupplementalLogger("supplemental_logger_mt", textFileName.String());

    VPKPair_t pair(localeName, contextName, levelName, NULL);
    Msg(eDLL_T::FS, "*** Starting VPK build command for: '%s'\n", pair.m_DirName.Get());

    CFastTimer timer;
    timer.Start();

    CPackedStoreBuilder builder;

    builder.InitLzEncoder(
        argCount > 7 ? Min(atoi(args.Arg(7)), LZHAM_MAX_HELPER_THREADS) : -1, // Num threads.
        argCount > 8 ? args.Arg(8) : "default"); // Compress level.

    builder.PackStore(pair, workspacePath.String(), buildPath.String());

    timer.End();
    Msg(eDLL_T::FS, "*** Time elapsed: '%lf' seconds\n", timer.GetDuration().GetSeconds());
    Msg(eDLL_T::FS, "\n");
}

//-----------------------------------------------------------------------------
// Purpose: unpacks VPK files into workspace directory
//-----------------------------------------------------------------------------
static void ReVPK_Unpack(const CCommand& args)
{
    const int argCount = args.ArgC();

    if (argCount < 3)
    {
        ReVPK_Usage();
        return;
    }

    const char* fileName = args.Arg(2);
    const char* outPath = argCount > 3 ? args.Arg(3) : "ship/";
    const bool sanitize = argCount > 4 ? atoi(args.Arg(4)) != NULL : false;

    VPKDir_t vpk(fileName, sanitize);

    // Make sure the VPK directory tree was actually parsed correctly.
    if (vpk.Failed())
    {
        Error(eDLL_T::FS, NO_ERROR, "Failed to parse directory tree file \"%s\"!\n", fileName);
        return;
    }

    CUtlString baseName = PackedStore_GetDirBaseName(vpk.m_DirFilePath);

    // Write the unpack log to a file.
    CFmtStr1024 textFileName("%s%s%s.log", outPath, UNPACK_LOG_DIR, baseName.String());
    SpdLog_InstallSupplementalLogger("supplemental_logger_mt", textFileName.String());

    const char* actualDirFile = vpk.m_DirFilePath.String();
    Msg(eDLL_T::FS, "*** Starting VPK extraction command for: '%s'\n", actualDirFile);

    CFastTimer timer;
    timer.Start();

    CPackedStoreBuilder builder;

    builder.InitLzDecoder();
    builder.UnpackStore(vpk, argCount > 3 ? args.Arg(3) : "ship/");

    timer.End();
    Msg(eDLL_T::FS, "*** Time elapsed: '%lf' seconds\n", timer.GetDuration().GetSeconds());
    Msg(eDLL_T::FS, "\n");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    ReVPK_Init();

    CCommand args;
    CUtlString str;

    for (int i = 0; i < argc; i++)
    {
        str.Append(argv[i]);
        str.Append(' ');
    }

    args.Tokenize(str.Get(), cmd_source_t::kCommandSrcCode);

    if (!args.ArgC()) {
        ReVPK_Usage();
    }
    else
    {
        if (V_strcmp(args.Arg(1), PACK_COMMAND) == NULL) {
            ReVPK_Pack(args);
        }
        else if (V_strcmp(args.Arg(1), UNPACK_COMMAND) == NULL) {
            ReVPK_Unpack(args);
        }
        else {
            ReVPK_Usage();
        }
    }

    ReVPK_Shutdown();
}
