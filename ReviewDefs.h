#pragma once

#include <string>

struct tReview
{
	std::string		steamid				= "";
	std::string		review				= "";
	bool			recommended			= false;
	uint64_t		playtimeAtReview	= 0u;
	uint64_t		playtimeAllTime		= 0u;
	uint64_t		timestampCreated	= 0u;
	uint64_t		timestampUpdated	= 0u;
	uint64_t		timestampLastPlayed	= 0u;
};

#define PROP_STEAMID				"steamId"
#define PROP_REVIEW					"review"
#define PROP_RECOMMENDED			"recommended"
#define PROP_PLAYTIMEATREVIEW		"playtimeAtReview"
#define PROP_PLAYTIMEALLTIME		"playtimeAllTime"
#define PROP_TIMESTAMPCREATED		"timestampCreated"
#define PROP_TIMESTAMPUPDATED		"timestampUpdated"
#define PROP_TIMESTAMPLASTPLAYED	"timestampLastPlayed"

struct tNews
{
	std::string	steamNewsId				= "";	//news unique id
	std::string	title					= "";
	std::string	source					= "";	//appid (steam news have appid 593110)
	uint64_t	timestampPublished		= 0u;
};

#define STEAM_GENERAL_NEWS_APP_ID	"593110"

#define PROP_NEWSID					"steamNewsId"
#define PROP_NEWSTITLE				"title"
#define PROP_NEWSSOURCE				"appId"
#define PROP_NEWSTIMESTAMPPUBLISHED	"timestampPublished"