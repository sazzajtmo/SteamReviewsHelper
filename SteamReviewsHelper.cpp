#include "stdafx.h"
#include "Logger.h"
#include "cpr/cpr.h"
#include "sqlite3.h"
#include "ReviewsDB.h"
#include "ReviewsManager.h"
#include "ReviewDefs.h"

#pragma comment(lib, "sqlite3.lib")

#ifdef _NDEBUG
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

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	//const std::string apiCall = "http://store.steampowered.com/appreviews/1364390?json=1&filter=recent&day_range=7";

	//cpr::Response r = cpr::Get(cpr::Url{apiCall});
	//LOG( "%s", r.text.c_str() );

	//tReview rev;
	//rev.steamid = 76561198067767552;
	//rev.recommended = true;
	//rev.review = "free, but I want a refund.";

	//cReviewsDB reviews;
	//reviews.Open( "reviews.db" );
	//reviews.ResetReviewsTable();
	//reviews.AddReviewEntry( rev );

	//https://store.steampowered.com/app/1364390
	cReviewsManager::GetInstance()->Init( "1364390" );
	//cReviewsManager::GetInstance()->ExportReviews( "01/10/2022", "30/10/2022" );
	cReviewsManager::GetInstance()->ExportReviews( "01/10/2023", "31/10/2023" );
	//cReviewsManager::GetInstance()->ExportReviews( "01/11/2023", "30/11/2023" );
	cReviewsManager::GetInstance()->Cleanup();
	cReviewsManager::DestroyInstance();
	LOG( "shutdown" );
	return 0;
}