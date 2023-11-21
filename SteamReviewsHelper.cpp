#include "stdafx.h"
#include "Logger.h"
#include "ReviewsDB.h"
#include "ReviewsManager.h"
#include "ReviewDefs.h"

#pragma comment(lib, "sqlite3.lib")

#ifdef NDEBUG
	#pragma comment(lib, "cpr.lib")
	#pragma comment(lib, "libcurl_imp")
	#pragma comment(lib, "curlu")
	#pragma comment(lib, "zlib")
#else
	#pragma comment(lib, "cpr.lib")
	#pragma comment(lib, "libcurl-d_imp")
	#pragma comment(lib, "curlu-d")
	#pragma comment(lib, "zlib")
#endif

std::unordered_map<std::string,std::string> g_helperInputs =
{
	{ "-appid",		"" },
	{ "-openaikey",	"" },
	{ "-start",		"" },
	{ "-end",		"" }
};

//int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
int main(int argc, char* argv[])
{
	LOG( "Steam Reviews Helper Utility - downloads, caches and summarizes (w/ chatGPT 3.5) Steam reviews for an app identified by an steamappid." );
	LOG( " - aggregated data exported to SteamReviews_appId.csv" );
	LOG( " - chatGPT summary exported to SteamReviews_appId_summary.txt" );
	LOG( " - queryable sqlite db exported to SteamReviews_appId.db" );
	LOG( "" );
	LOG( "Usage" );
	LOG( "SteamReviewsHelper.exe -appid <steamAppId> -openaikey <openAI_API_Key> -start <dd/mm/yyyy> -end <dd/mm/yyyy>" );
	LOG( "" );
	LOG( "App log" );
	LOG( "App started" );
	int argIdx = 1;

	while (argIdx < argc)
	{
		auto inputIt = g_helperInputs.find( argv[argIdx] );

		if (inputIt != g_helperInputs.end() && argIdx + 1 < argc)
		{
			inputIt->second = argv[argIdx+1];
			argIdx += 2;
		}
		else
		{
			LOG( "ignoring argument %s", argv[argIdx] );
			argIdx++;
		}
	}

	bool readyToRun = true;
	
	if (g_helperInputs["-appid"] == "")
	{
		LOG( "steam app id missing. aborting" );
		readyToRun = false;
	}

	if (g_helperInputs["-openaikey"] == "")
	{
		LOG( "open AI Key missing - blank summary will be exported." );
	}

	if (g_helperInputs["-start"] == "")
	{
		LOG( "start date missing. aborting" );
		readyToRun = false;
	}

	if (g_helperInputs["-end"] == "")
	{
		LOG( "end date missing. aborting" );
		readyToRun = false;
	}
		
	if (readyToRun)
	{
		cReviewsManager::GetInstance()->Init( g_helperInputs["-appid"], g_helperInputs["-openaikey"] );
		cReviewsManager::GetInstance()->ExportReviews( g_helperInputs["-start"], g_helperInputs["-end"] );
		cReviewsManager::GetInstance()->Cleanup();
		cReviewsManager::DestroyInstance();
	}

	LOG( "App ended" );

	return 0;
}