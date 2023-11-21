#include "stdafx.h"
#include "ReviewsManager.h"
#include "ReviewsDB.h"
#include "ReviewDefs.h"
#include "Logger.h"
#include "cpr/cpr.h"
#include "json.hpp"
#include <format>
#include <sstream>

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

bool cReviewsManager::Init( const std::string& steamAppID )
{
	m_steamAppID	= steamAppID;
	m_reviewsDB		= std::make_unique<cReviewsDB>();

	std::string dbName = REVIEWS_DATABASE_NAME + "_" + m_steamAppID + ".db";
	
	if( !m_reviewsDB->Open( dbName ) )
		return false;

	m_reviewsDB->ResetReviewsTable();

	#if 0	
	tReview rev;
	rev.steamid = 76561198067767552;
	rev.recommended = true;
	rev.review = "free, but I want a refund.";
	rev.timestampCreated = 1668950226;
	m_reviewsDB->AddReviewEntry( rev );
	m_reviewsDB->AddReviewEntry( rev );
	rev.timestampCreated = 1700486226;
	rev.steamid = 76561198067767553;
	m_reviewsDB->AddReviewEntry( rev );
	#endif

	tReview rev2;
	m_reviewsDB->GetOldestEntry( rev2 );
	m_oldestTimestamp = rev2.timestampCreated;

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
		DownloadReviews(startDateUTC);

		if( m_reviewsDB )
			m_reviewsDB->GetReviews( startDateUTC, endDateUTC, reviews );
	}
	else
	{
		//query where review.timestampCreated in [startDateUTC, endDateUTC]
		if( m_reviewsDB )
			m_reviewsDB->GetReviews( startDateUTC, endDateUTC, reviews );
	}

	ExportToCSV( reviews );
}

void cReviewsManager::DownloadReviews(uint64_t untilTimestamp)
{
	LOG("Download reviews");

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
		nlohmann::json	responseJson	= nlohmann::json::parse( response.text.c_str() );

		if( !responseJson.contains("reviews") )
			break;

		nlohmann::json	reviewsJson		= responseJson["reviews"];
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

void cReviewsManager::ExportToCSV(const std::vector<tReview>& reviews)
{
	const int	numReview	= (int) reviews.size();
	int			numPositive	= 0;
	std::map<std::string, std::pair<int,int>> dateToPositiveRatioMap;

	for (const auto& review : reviews)
	{
		numPositive += review.recommended;
		dateToPositiveRatioMap[GetTimeFromTimestamp(review.timestampCreated)].first		+= review.recommended;
		dateToPositiveRatioMap[GetTimeFromTimestamp(review.timestampCreated)].second	+= !review.recommended;
	}

	std::string	fileContent;

	fileContent += "num reviews," + std::to_string(reviews.size()) + ",,\n";
	fileContent += "positive reviews," + std::to_string(numPositive) + ",,\n";
	fileContent += "negative reviews," + std::to_string(reviews.size()-numPositive) + ",,\n";
	fileContent += "positive ratio," + std::to_string((float)numPositive/(float)reviews.size()) + ",,\n";
	fileContent += ",,,\n";
	fileContent += "Dates,Positives,Negatives,\n";

	for (const auto& [key, value] : dateToPositiveRatioMap)
	{
		fileContent += key + "," + std::to_string(value.first) + "," + std::to_string(-value.second) + ",\n";
	}

	std::string dbName = REVIEWS_DATABASE_NAME + "_" + m_steamAppID + ".csv";
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

