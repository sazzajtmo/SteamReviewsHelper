#pragma once

struct sqlite3;
struct tReview;

class cReviewsDB
{
public:
	cReviewsDB();
	virtual ~cReviewsDB();

	bool		Open( const std::string& dbName );
	bool		Close();
	bool		ResetReviewsTable();

	bool		AddReviewEntry( const tReview& review );
	void		GetFirstEntry( tReview& review, bool oldest = true ); //oldest == false, then newest entry
	void		GetReviews( uint64_t startTimestamp, uint64_t endTimestamp, std::vector<tReview>& reviews );

private:
	sqlite3* m_db = nullptr;	
};

