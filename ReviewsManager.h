#pragma once

#include <string>
#include <memory>
#include "ReviewDefs.h"

class cReviewsDB;

class cReviewsManager final
{
protected:
	cReviewsManager();
	~cReviewsManager();

public:
	static cReviewsManager*			GetInstance();
	static void						DestroyInstance();

	bool							Init( const std::string& steamAppID );
	void							Cleanup();
	void							ExportReviews( const std::string& startDate, const std::string& endDate );

private:
	void							DownloadReviews( uint64_t untilTimestamp );
	void							ExportToCSV( const std::vector<tReview>& reviews );
	std::string						GetTimeFromTimestamp( uint64_t timestamp );
		
private:	
	static	cReviewsManager*		s_instance;
	std::unique_ptr<cReviewsDB>		m_reviewsDB;
	std::string						m_steamAppID = "";
	uint64_t						m_oldestTimestamp = 0u;	//UTC
};

