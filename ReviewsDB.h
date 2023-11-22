#pragma once

struct sqlite3;
struct tReview;
struct tNews;

class cReviewsDB
{
public:
	cReviewsDB();
	virtual ~cReviewsDB();

	bool		Open( const std::string& dbName );
	bool		Close();
	bool		CreateReviewsTable();
	bool		CreateNewsTable();

	bool		AddReviewEntry( const tReview& review );
	void		GetFirstEntry( tReview& review, bool oldest = true ); //oldest == false, then newest entry
	void		GetReviews( uint64_t startTimestamp, uint64_t endTimestamp, std::vector<tReview>& reviews );

	bool		AddNewsEntry( const tNews& newsEntry );
	int			GetNewsCountByAppId( const std::string& appId );
	void		GetNews( uint64_t startTimestamp, uint64_t endTimestamp, std::vector<tNews>& news );

private:
	sqlite3* m_db = nullptr;	
};

