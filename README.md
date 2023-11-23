# Steam Reviews Helper Utility

Download, cache, and summarize (w/ chatGPT default model) Steam reviews for an app identified by a steamappid.

Main features:
* aggregates review data (positive/negative numbers, ratio, news titles) by exact date interval and exports to SteamReviews_appId.csv which can be further used to make pretty graphs in other software.
* summarizes review content with chatGPT default model and exports to SteamReviews_appId_summary.txt.
* acquires Steam News published by the app and by Steam itself.
* caches reviews and news to a local queryable SQLite db (SteamReviews_appId.db).

# Why?

Steam graph widget that allows for filtering reviews by date isn't accurate, I needed this for exact [start, end] date intervals.

The summarization helps with understanding overall sentiment, getting the pain points, and dealing with reviews in different languages.

# Usage

_SteamReviewsHelper.exe -appid <steamAppId> -openaikey <openAI_API_Key> -start <dd/mm/yyyy> -end <dd/mm/yyyy>_

# License

Free to use and modify as you please for all types of projects. No acknowledgment is required.

