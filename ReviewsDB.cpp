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

bool cReviewsDB::ResetReviewsTable()
{
	//also delete please before
	//reviewId INT PRIMARY KEY NOT NULL AUTOINCREMENT,
	//
	std::string sqlQuery = "create table Reviews(reviewId INTEGER PRIMARY KEY AUTOINCREMENT, steamId VARCHAR(255) UNIQUE, review TEXT, recommended BOOL, playtimeAtReview BIGINT, playtimeAllTime BIGINT, timestampCreated BIGINT, timestampUpdated BIGINT, timestampLastPlayed BIGINT)";

	char* errMsg;
	int execRet = sqlite3_exec( m_db, sqlQuery.c_str(), [](void *NotUsed, int argc, char **argv, char **azColName) -> int 
	{
		return 0;
	}, nullptr, &errMsg );

	if (execRet != SQLITE_OK)
	{
		LOG( "error creating table Reviews : %s", errMsg );
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

void cReviewsDB::GetOldestEntry(tReview& review)
{
	if( !m_db )
		return;

	std::string sqlQuery = std::format( R"( 
		select * from Reviews order by timestampCreated limit 1
	)");

	char* errMsg;
	int execRet = sqlite3_exec( m_db, sqlQuery.c_str(), [](void *userData, int argc, char **argv, char **azColName) -> int 
	{
		tReview& reviewRef = *reinterpret_cast<tReview*>( userData );

		for (int i = 0; i < argc; i++)
		{
			if( strcmp( azColName[i], PROP_STEAMID ) == 0 )
				reviewRef.steamid = std::atoll( argv[i] );
			else if( strcmp( azColName[i], PROP_TIMESTAMPCREATED ) == 0 )
				reviewRef.timestampCreated = std::atoll( argv[i] );
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
			if( strcmp( azColName[i], PROP_STEAMID ) == 0 )
				currReview.steamid = argv[i];
			else if( strcmp( azColName[i], PROP_REVIEW ) == 0 )
				currReview.review = argv[i];
			else if( strcmp( azColName[i], PROP_RECOMMENDED ) == 0 )
				currReview.recommended = std::atoi( argv[i] );
			else if( strcmp( azColName[i], PROP_PLAYTIMEATREVIEW ) == 0 )
				currReview.playtimeAtReview = std::atoi( argv[i] );
			else if( strcmp( azColName[i], PROP_PLAYTIMEALLTIME ) == 0 )
				currReview.playtimeAllTime = std::atoi( argv[i] );
			else if( strcmp( azColName[i], PROP_TIMESTAMPCREATED ) == 0 )
				currReview.timestampCreated = std::atoll( argv[i] );
			else if( strcmp( azColName[i], PROP_TIMESTAMPUPDATED ) == 0 )
				currReview.timestampUpdated = std::atoll( argv[i] );
			else if( strcmp( azColName[i], PROP_TIMESTAMPLASTPLAYED ) == 0 )
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
