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

	bool							Init( const std::string& steamAppID, const std::string& openAIKey );
	void							Cleanup();
	void							ExportReviews( const std::string& startDate, const std::string& endDate );

private:
	void							DownloadReviews( uint64_t untilTimestamp );
	void							ExportToCSV( const std::vector<tReview>& reviews, const std::vector<tNews>& news );
	std::string						GetTimeFromTimestamp( uint64_t timestamp );
	std::string						SummarizeReviews( const std::vector<tReview>& reviews );
	void							FormatAndExportSummary( std::string& summary );

	void							GetNews( uint64_t startTimestamp, uint64_t endTimestamp, std::vector<tNews>& news );
		
private:	
	static	cReviewsManager*		s_instance;
	std::unique_ptr<cReviewsDB>		m_reviewsDB;
	std::string						m_steamAppID		= "";
	std::string						m_openAIKey			= "";
	uint64_t						m_oldestTimestamp	= 0u;	//UTC
	uint64_t						m_newestTimestamp	= 0u;	//UTC
};

