#include "stdafx.h"
#include "ReviewsManager.h"
#include "ReviewsDB.h"
#include "ReviewDefs.h"
#include "Logger.h"
#include "cpr/cpr.h"
#include "json.hpp"
#include <format>
#include <sstream>
#include <tuple>

using json = nlohmann::json;

const std::string REVIEWS_DATABASE_NAME = "SteamReviews";

cReviewsManager* cReviewsManager::s_instance(nullptr);

cReviewsManager::cReviewsManager()
{
}

cReviewsManager::~cReviewsManager()
{
}

cReviewsManager* cReviewsManager::GetInstance()
{
	if( !s_instance )
		s_instance = new cReviewsManager;

	return s_instance;
}

void cReviewsManager::DestroyInstance()
{
	if (s_instance)
	{
		delete s_instance;
		s_instance = nullptr;
	}
}

bool cReviewsManager::Init( const std::string& steamAppID, const std::string& openAIKey )
{
	LOG( "Init for steamapp id %s", steamAppID.c_str() );

	m_steamAppID	= steamAppID;
	m_openAIKey		= openAIKey;
	m_reviewsDB		= std::make_unique<cReviewsDB>();

	std::string dbName = REVIEWS_DATABASE_NAME + "_" + m_steamAppID + ".db";
	
	if( !m_reviewsDB->Open( dbName ) )
		return false;

	m_reviewsDB->CreateReviewsTable();
	m_reviewsDB->CreateNewsTable();

	tReview firstEntry;
	m_reviewsDB->GetFirstEntry( firstEntry, true );		//oldest
	m_oldestTimestamp = firstEntry.timestampCreated;

	m_reviewsDB->GetFirstEntry( firstEntry, false );	//newest
	m_newestTimestamp = firstEntry.timestampCreated;

	return true;
}

void cReviewsManager::Cleanup()
{
	if (m_reviewsDB)
	{
		m_reviewsDB->Close();
		m_reviewsDB.reset();
	}
}

void cReviewsManager::ExportReviews(const std::string& startDate, const std::string& endDate)
{
	LOG( "Query and export reviews between '%s' and '%s'", startDate.c_str(), endDate.c_str() );

	if (!m_reviewsDB)
	{
		LOG("error on ExportReviews > no db was opened");
		return;
	}

	auto getUTCTimestamp = [](const std::string& ymdDate)
	{
		const std::string			date_time_format = "%d/%m/%Y";
		std::chrono::year_month_day	localDate;
		std::istringstream ss{ ymdDate };

		ss >> std::chrono::parse(date_time_format, localDate);
		auto local_time = std::chrono::sys_days(localDate) + std::chrono::hours(0);
		auto utc_time = local_time - std::chrono::hours(0);
		auto utc_timestamp = std::chrono::time_point_cast<std::chrono::seconds>(utc_time);

		return std::chrono::duration_cast<std::chrono::seconds>(utc_timestamp.time_since_epoch()).count();
	};

	uint64_t startDateUTC	= getUTCTimestamp( startDate );
	uint64_t endDateUTC		= getUTCTimestamp( endDate ) + 86400 - 1; //add 24 hours - 1 second because getUTCTimestamp returns date at 00:00

	std::vector<tReview> reviews;

	if (m_oldestTimestamp == 0 || startDateUTC < m_oldestTimestamp)
	{
		LOG( "start date older than oldest timestamp cached (%s vs. %s)", startDate.c_str(), GetTimeFromTimestamp(m_oldestTimestamp).c_str() );
		DownloadReviews(startDateUTC);
	}
	else if ( m_newestTimestamp == 0 || endDateUTC > m_newestTimestamp)
	{
		LOG( "end date newer than newest timestamp cached", endDate.c_str(), GetTimeFromTimestamp(m_newestTimestamp).c_str() );
		DownloadReviews(endDateUTC);
	}

	LOG( "Query reviews from cache" );

	if( m_reviewsDB )
		m_reviewsDB->GetReviews( startDateUTC, endDateUTC, reviews );

	std::vector<tNews> news;
	GetNews( startDateUTC, endDateUTC, news );

	ExportToCSV( reviews, news );

	std::string	reviewsSummary = SummarizeReviews( reviews );

	FormatAndExportSummary( reviewsSummary );
}

void cReviewsManager::DownloadReviews(uint64_t untilTimestamp)
{
	LOG("Download reviews from now to %s", GetTimeFromTimestamp(untilTimestamp).c_str() );

	auto replaceQuotes = [](std::string& str)
	{
		size_t pos = str.find( '"' );

		while (pos != std::string::npos)
		{
			str[pos] = '\'';
			pos = str.find( '"', pos + 1 );
		}
	};

	std::string callCursor = "*"; //reviews are returned in batches of 20, so pass "*" for the first set, then the value of "cursor" that was returned in the response for the next set, etc. Note that cursor values may contain characters that need to be URLEncoded for use in the querystring.

	while (callCursor != "null")
	{
		std::string		apiCall			= std::format("http://store.steampowered.com/appreviews/{}?json=1&num_per_page=20&review_type=all&filter=recent&day_range={}&language=all&purchase_type=all&cursor={}", m_steamAppID, 100, callCursor);
		cpr::Response	response		= cpr::Get(cpr::Url{apiCall});
		json	responseJson	= json::parse( response.text.c_str() );

		if( !responseJson.contains("reviews") )
			break;

		json			reviewsJson		= responseJson["reviews"];
		int				numReviews		= 0;
		bool			reachedGoal		= false;

		for (auto& reviewJson : reviewsJson.items())
		{
			const auto& reviewValue = reviewJson.value();
				
			tReview rev;
			rev.steamid				= reviewValue["author"]["steamid"].dump();
			rev.steamid				= rev.steamid.substr( 1, rev.steamid.length() - 2 );
			rev.recommended			= reviewValue["voted_up"];
			rev.review				= reviewValue["review"];
			replaceQuotes( rev.review );
			rev.timestampCreated	= reviewValue["timestamp_created"];
			rev.timestampUpdated	= reviewValue["timestamp_updated"];
			rev.playtimeAtReview	= reviewValue["author"]["playtime_at_review"];
			rev.playtimeAllTime		= reviewValue["author"]["playtime_forever"];
			rev.timestampLastPlayed = reviewValue["author"]["last_played"];

			LOG( "received %d from %s (%s)", rev.recommended, rev.steamid.c_str(), GetTimeFromTimestamp(rev.timestampCreated).c_str() );

			if (m_reviewsDB)
				m_reviewsDB->AddReviewEntry( rev );

			if( !reachedGoal && untilTimestamp > rev.timestampCreated )
				reachedGoal = true;

			numReviews++;
		}

		LOG("received %d reviews", numReviews );

		callCursor = responseJson["cursor"].dump();//cpr::util::urlEncode( responseJson["cursor"].dump() );
		callCursor = callCursor.substr( 1, callCursor.length() - 2 );
		callCursor = cpr::util::urlEncode( callCursor );

		if( reachedGoal )
			break;
	}
}

void cReviewsManager::ExportToCSV(const std::vector<tReview>& reviews, const std::vector<tNews>& news)
{
	std::string dbName		= REVIEWS_DATABASE_NAME + "_" + m_steamAppID + ".csv";
	
	LOG( "Export aggregated reviews data to %s", dbName.c_str() );

	const int	numReview	= (int) reviews.size();
	int			numPositive	= 0;

	using tDateEntry = std::tuple<int, int, std::string>;	//positives, negatives, news title

	std::map<std::string, tDateEntry> steamDateInfoMap;

	for (const auto& review : reviews)
	{
		numPositive += review.recommended;

		auto& [positives,negatives,title] = steamDateInfoMap[GetTimeFromTimestamp(review.timestampCreated)];

		positives	+= review.recommended;
		negatives	+= !review.recommended;
	}

	for (const auto& newsEntry : news)
	{
		auto& [positives,negatives,title] = steamDateInfoMap[GetTimeFromTimestamp(newsEntry.timestampPublished)];
		title = newsEntry.title;
	}

	std::string	fileContent;

	fileContent += "num reviews," + std::to_string(reviews.size()) + ",,,\n";
	fileContent += "positive reviews," + std::to_string(numPositive) + ",,,\n";
	fileContent += "negative reviews," + std::to_string(reviews.size()-numPositive) + ",,,\n";
	fileContent += "positive ratio," + std::to_string((float)numPositive/(float)reviews.size()) + ",,,\n";
	fileContent += ",,,,\n";
	fileContent += "Dates,Positives,Negatives,News,\n";

	for (const auto& [key, value] : steamDateInfoMap)
	{
		const auto& [positives,negatives,title] = value;
		fileContent += key + "," + std::to_string(positives) + "," + std::to_string(-negatives) + "," + title + ",\n";
	}
		
	std::ofstream csvFile( dbName );
	csvFile.write( fileContent.c_str(), fileContent.size() );
	csvFile.close();
}

std::string cReviewsManager::GetTimeFromTimestamp(uint64_t timestamp)
{
	auto		timePoint		= std::chrono::system_clock::from_time_t(timestamp);
	std::time_t	currentTime_t	= std::chrono::system_clock::to_time_t(timePoint);
	std::tm		currentTime_tm;
			
	gmtime_s(&currentTime_tm,&currentTime_t);

	char buffer[80] = {0}; // This is the size recommended by the C standard

	//std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &currentTime_tm);
	std::strftime(buffer, sizeof(buffer), "%d/%m/%Y", &currentTime_tm);

	return buffer;
}

std::string cReviewsManager::SummarizeReviews(const std::vector<tReview>& reviews)
{
	LOG( "Call OpenAI to summarize reviews" );

	std::string chatSystemMessage	= "Act as a helpful analyst that summarizes and analyses text excerpts from heterogenous sources in different languages. All your responses will be in English even if the review text is in other languages.";
	std::string chatSummaryRequest	= "Review, summarize and compile by merging similar concepts the following texts that are individual reviews for a freemium application. All reviews are separated by ----- string.\n\n";

	for( const auto& review : reviews )
		chatSummaryRequest += review.review + "\n-----\n\n";
	
	json jsonChatBody;
	jsonChatBody["model"] = "gpt-3.5-turbo";

	json jsonSystemMessage;
	jsonSystemMessage["role"] = "system";
	jsonSystemMessage["content"] = chatSystemMessage;
	jsonChatBody["messages"].push_back( jsonSystemMessage );

	json jsonUserMessage;
	jsonUserMessage["role"] = "user";
	jsonUserMessage["content"] = chatSummaryRequest;
	jsonChatBody["messages"].push_back( jsonUserMessage );

	std::string		charAPICall		= "https://api.openai.com/v1/chat/completions";
	cpr::Header		chatHeader		= 
	{
		{ "Content-Type", "application/json" },
		{ "Authorization", std::format("Bearer {}", m_openAIKey ) }
	};
	cpr::Body		chatBody{ jsonChatBody.dump() };

	cpr::Response	response		= cpr::Post(cpr::Url{charAPICall}, chatHeader, chatBody );

	json responseJson = json::parse( response.text );

	LOG( "Call to OpenAI done" );

	if (responseJson.contains("error"))
	{
		LOG( "OpenAI error %s", response.text.c_str() );
		return response.text;
	}

	return responseJson["choices"].at(0)["message"]["content"].dump();
}

void cReviewsManager::FormatAndExportSummary(std::string& summary)
{
	std::string		dbName = REVIEWS_DATABASE_NAME + "_" + m_steamAppID + "_summary.txt";

	LOG( "Export reviews summary to %s", dbName.c_str() );

	auto replaceEscapedNewlines = [](std::string& str)
	{
		size_t pos = str.find( "\\n" );

		while (pos != std::string::npos)
		{
			str.replace( pos, 2, "\n");
			pos = str.find( "\\n" );
		}
	};

	replaceEscapedNewlines( summary );

	std::ofstream	summaryFile( dbName );
	summaryFile.write( summary.c_str(), summary.size() );
	summaryFile.close();
}

void cReviewsManager::GetNews( uint64_t startTimestamp, uint64_t endTimestamp, std::vector<tNews>& news )
{
	LOG( "Download and cache news (steam and app news)" );

	if (!m_reviewsDB)
		return;

	auto replaceQuotes = [](std::string& str)
	{
		size_t pos = str.find( '"' );

		while (pos != std::string::npos)
		{
			str[pos] = '\'';
			pos = str.find( '"', pos + 1 );
		}
	};

	auto getOnlineNewsCount = []( const std::string& appId ) -> int
	{
		int count = 0;

		std::string		apiCall			= std::format( "https://api.steampowered.com/ISteamNews/GetNewsForApp/v2/?appid={}&maxlength=20&count=1", appId );
		cpr::Response	response		= cpr::Get(cpr::Url{apiCall});
		json			responseJson	= json::parse( response.text.c_str() );

		if( !responseJson.contains("appnews") )
			return 0;

		return responseJson["appnews"]["count"];
	};

	auto downloadAndCacheOnlineNews = [this, replaceQuotes]( const std::string& appId, int count )
	{
		std::string		apiCall			= std::format( "https://api.steampowered.com/ISteamNews/GetNewsForApp/v2/?appid={}&maxlength=20&count={}", appId, count );
		cpr::Response	response		= cpr::Get(cpr::Url{apiCall});
		json			responseJson	= json::parse( response.text.c_str() );

		if( !responseJson.contains("appnews") )
			return;

		json			newsItemsJson	= responseJson["appnews"]["newsitems"];

		for (auto& newsItemJson : newsItemsJson.items())
		{
			const auto& newsItem = newsItemJson.value();

			tNews newsEntry;

			newsEntry.steamNewsId			= newsItem["gid"].dump();
			replaceQuotes(newsEntry.steamNewsId);
			newsEntry.title					= newsItem["title"].dump();
			replaceQuotes(newsEntry.title);
			newsEntry.source				= appId;
			newsEntry.timestampPublished	= newsItem["date"];

			m_reviewsDB->AddNewsEntry( newsEntry );

			LOG( "downloaded news %s for app %s on %s", newsEntry.title.c_str(), newsEntry.source.c_str(), GetTimeFromTimestamp(newsEntry.timestampPublished).c_str());
		}
	};

	int newSteamNewsCount		= getOnlineNewsCount( STEAM_GENERAL_NEWS_APP_ID );
	int newAppNewsCount			= getOnlineNewsCount( m_steamAppID );
	int cachedSteamNewsCount	= m_reviewsDB->GetNewsCountByAppId( STEAM_GENERAL_NEWS_APP_ID );
	int cachedAppNewsCount		= m_reviewsDB->GetNewsCountByAppId( m_steamAppID );

	if( newSteamNewsCount - cachedSteamNewsCount > 0 )
		downloadAndCacheOnlineNews( STEAM_GENERAL_NEWS_APP_ID, newSteamNewsCount - cachedSteamNewsCount );

	if( newAppNewsCount - cachedAppNewsCount > 0 )
		downloadAndCacheOnlineNews( m_steamAppID, newSteamNewsCount - cachedSteamNewsCount );

	m_reviewsDB->GetNews( startTimestamp, endTimestamp, news );
}

