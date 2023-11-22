#include "stdafx.h"
#include "ReviewsDB.h"
#include "ReviewDefs.h"
#include "sqlite3.h"
#include "Logger.h"
#include <format>

cReviewsDB::cReviewsDB()
{
}

cReviewsDB::~cReviewsDB()
{
}

bool cReviewsDB::Open(const std::string& dbName)
{
	LOG( "initializing db %s", dbName.c_str() );

	if( sqlite3_initialize() != SQLITE_OK )
	{
		LOG("error initializing sqlite");
		return false;
	}
	
	LOG( "initialized sqlite" );

	if (sqlite3_open(dbName.c_str(), &m_db) != SQLITE_OK)
	{
		LOG("error opening db %s : %s", dbName.c_str(), sqlite3_errmsg(m_db) );
		return false;
	}

	return true;
}

bool cReviewsDB::Close()
{
	if( !m_db )
		return false;

	if (sqlite3_close(m_db) != SQLITE_OK)
	{
		LOG("error closing db: %s", sqlite3_errmsg(m_db) );
		return false;
	}

	return true;
}

bool cReviewsDB::CreateReviewsTable()
{
	//also delete please before
	//reviewId INT PRIMARY KEY NOT NULL AUTOINCREMENT,
	//
	std::string sqlQuery = "create table Reviews(reviewId INTEGER PRIMARY KEY AUTOINCREMENT, steamId VARCHAR(255) UNIQUE, review TEXT, recommended BOOL, playtimeAtReview BIGINT, playtimeAllTime BIGINT, timestampCreated BIGINT, timestampUpdated BIGINT, timestampLastPlayed BIGINT)";

	char* errMsg;
	int execRet = sqlite3_exec( m_db, sqlQuery.c_str(), nullptr, nullptr, &errMsg );

	if (execRet != SQLITE_OK)
	{
		LOG( "error creating table Reviews : %s", errMsg );
		sqlite3_free(errMsg);
		return false;
	}

	return true;
}

bool cReviewsDB::CreateNewsTable()
{
	std::string sqlQuery = "create table News(newsId INTEGER PRIMARY KEY AUTOINCREMENT, steamNewsId VARCHAR(255) UNIQUE, title TEXT, appId VARCHAR(255), timestampPublished BIGINT)";

	char* errMsg;
	int execRet = sqlite3_exec( m_db, sqlQuery.c_str(), nullptr, nullptr, &errMsg );

	if (execRet != SQLITE_OK)
	{
		LOG( "error creating table News : %s", errMsg );
		sqlite3_free(errMsg);
		return false;
	}

	return true;
}

bool cReviewsDB::AddReviewEntry(const tReview& review)
{
	if( !m_db )
		return false;

	std::string sqlQuery = std::format( R"( 
		insert into Reviews( steamId, review, recommended, playtimeAtReview, playtimeAllTime, timestampCreated, timestampUpdated, timestampLastPlayed )
		values ( "{}", "{}", {}, {}, {}, {}, {}, {} )
	)", review.steamid, review.review, review.recommended, review.playtimeAtReview, review.playtimeAllTime, review.timestampCreated, review.timestampUpdated, review.timestampLastPlayed );

	char* errMsg;
	int execRet = sqlite3_exec( m_db, sqlQuery.c_str(), [](void *NotUsed, int argc, char **argv, char **azColName) -> int 
	{
		return 0;
	}, nullptr, &errMsg );

	if (execRet != SQLITE_OK)
	{
		LOG( "error adding entry into Reviews : %s", errMsg );
		sqlite3_free(errMsg);
		return false;
	}

	return true;
}

void cReviewsDB::GetFirstEntry(tReview& review, bool oldest)
{
	if( !m_db )
		return;

	std::string sqlQuery = std::format( R"( 
		select * from Reviews order by timestampCreated {} limit 1
	)", oldest ? "ASC" : "DESC" );

	char* errMsg;
	int execRet = sqlite3_exec( m_db, sqlQuery.c_str(), [](void *userData, int argc, char **argv, char **azColName) -> int 
	{
		tReview& reviewRef = *reinterpret_cast<tReview*>( userData );

		for (int i = 0; i < argc; i++)
		{
			if( strcmp( azColName[i], PROP_STEAMID ) == 0 )
				reviewRef.steamid = argv[i];
			else if( strcmp( azColName[i], PROP_REVIEW ) == 0 )
				reviewRef.review = argv[i];
			else if( strcmp( azColName[i], PROP_RECOMMENDED ) == 0 )
				reviewRef.recommended = std::atoi( argv[i] );
			else if( strcmp( azColName[i], PROP_PLAYTIMEATREVIEW ) == 0 )
				reviewRef.playtimeAtReview = std::atoi( argv[i] );
			else if( strcmp( azColName[i], PROP_PLAYTIMEALLTIME ) == 0 )
				reviewRef.playtimeAllTime = std::atoi( argv[i] );
			else if( strcmp( azColName[i], PROP_TIMESTAMPCREATED ) == 0 )
				reviewRef.timestampCreated = std::atoll( argv[i] );
			else if( strcmp( azColName[i], PROP_TIMESTAMPUPDATED ) == 0 )
				reviewRef.timestampUpdated = std::atoll( argv[i] );
			else if( strcmp( azColName[i], PROP_TIMESTAMPLASTPLAYED ) == 0 )
				reviewRef.timestampLastPlayed = std::atoll( argv[i] );
		}
				
		return 0;
	}, &review, &errMsg );

	if (execRet != SQLITE_OK)
	{
		LOG( "error querying Reviews : %s", errMsg );
		sqlite3_free(errMsg);
		return;
	}
}

void cReviewsDB::GetReviews(uint64_t startTimestamp, uint64_t endTimestamp, std::vector<tReview>& reviews)
{
	if( !m_db )
		return;

	std::string sqlQuery = std::format( R"( 
		select * from Reviews where timestampCreated >= {} and timestampCreated <= {} order by timestampCreated
	)", startTimestamp, endTimestamp );

	char* errMsg;
	int execRet = sqlite3_exec( m_db, sqlQuery.c_str(), [](void *userData, int argc, char **argv, char **azColName) -> int 
	{
		std::vector<tReview>& reviewsRef = *reinterpret_cast<std::vector<tReview>*>( userData );
		tReview currReview;

		for (int i = 0; i < argc; i++)
		{
			const auto& colName = azColName[i];

			if( strcmp( colName, PROP_STEAMID ) == 0 )
				currReview.steamid = argv[i];
			else if( strcmp( colName, PROP_REVIEW ) == 0 )
				currReview.review = argv[i];
			else if( strcmp( colName, PROP_RECOMMENDED ) == 0 )
				currReview.recommended = std::atoi( argv[i] );
			else if( strcmp( colName, PROP_PLAYTIMEATREVIEW ) == 0 )
				currReview.playtimeAtReview = std::atoi( argv[i] );
			else if( strcmp( colName, PROP_PLAYTIMEALLTIME ) == 0 )
				currReview.playtimeAllTime = std::atoi( argv[i] );
			else if( strcmp( colName, PROP_TIMESTAMPCREATED ) == 0 )
				currReview.timestampCreated = std::atoll( argv[i] );
			else if( strcmp( colName, PROP_TIMESTAMPUPDATED ) == 0 )
				currReview.timestampUpdated = std::atoll( argv[i] );
			else if( strcmp( colName, PROP_TIMESTAMPLASTPLAYED ) == 0 )
				currReview.timestampLastPlayed = std::atoll( argv[i] );
		}

		reviewsRef.push_back( currReview );
				
		return 0;
	}, &reviews, &errMsg );

	if (execRet != SQLITE_OK)
	{
		LOG( "error querying Reviews : %s", errMsg );
		sqlite3_free(errMsg);
		return;
	}
}

bool cReviewsDB::AddNewsEntry(const tNews& news)
{
	if( !m_db )
		return false;

	std::string sqlQuery = std::format( R"( 
		insert into News( steamNewsId, title, appId, timestampPublished )
		values ( "{}", "{}", "{}", {} )
	)", news.steamNewsId, news.title, news.source, news.timestampPublished );

	char* errMsg;
	int execRet = sqlite3_exec( m_db, sqlQuery.c_str(), [](void *NotUsed, int argc, char **argv, char **azColName) -> int 
	{
		return 0;
	}, nullptr, &errMsg );

	if (execRet != SQLITE_OK)
	{
		LOG( "error adding entry into News : %s", errMsg );
		sqlite3_free(errMsg);
		return false;
	}

	return true;
}

int cReviewsDB::GetNewsCountByAppId(const std::string& appId)
{
	if( !m_db )
		return 0;

	std::string sqlQuery = std::format( R"( 
		select count(*) from News where appId = '{}'
	)", appId );

	int count = 0;
	char* errMsg;
	int execRet = sqlite3_exec( m_db, sqlQuery.c_str(), [](void *userData, int argc, char **argv, char **azColName) -> int 
	{
		if( argc != 1 )
			return 0;

		int& count = *reinterpret_cast<int*>( userData );
		count = std::atoi( argv[0] );

		return 0;
	}, &count, &errMsg );

	if (execRet != SQLITE_OK)
	{
		LOG( "error adding entry into News : %s", errMsg );
		sqlite3_free(errMsg);
		return 0;
	}

	return count;
}

void cReviewsDB::GetNews(uint64_t startTimestamp, uint64_t endTimestamp, std::vector<tNews>& news)
{
	if( !m_db )
		return;

	std::string sqlQuery = std::format( R"( 
		select * from News where timestampPublished >= {} and timestampPublished <= {} order by timestampPublished
	)", startTimestamp, endTimestamp );

	char* errMsg;
	int execRet = sqlite3_exec( m_db, sqlQuery.c_str(), [](void *userData, int argc, char **argv, char **azColName) -> int 
	{
		std::vector<tNews>& newsRef = *reinterpret_cast<std::vector<tNews>*>( userData );
		tNews newsEntry;

		for (int i = 0; i < argc; i++)
		{
			const auto& colName = azColName[i];

			if( strcmp( colName, PROP_NEWSID ) == 0 )
				newsEntry.steamNewsId = argv[i];
			else if( strcmp( colName, PROP_NEWSSOURCE ) == 0 )
				newsEntry.source = argv[i];
			else if( strcmp( colName, PROP_NEWSTITLE ) == 0 )
				newsEntry.title = argv[i];
			else if( strcmp( colName, PROP_NEWSTIMESTAMPPUBLISHED ) == 0 )
				newsEntry.timestampPublished = std::atoll( argv[i] );
		}

		newsRef.push_back( newsEntry );
				
		return 0;
	}, &news, &errMsg );

	if (execRet != SQLITE_OK)
	{
		LOG( "error querying News : %s", errMsg );
		sqlite3_free(errMsg);
		return;
	}	
}
