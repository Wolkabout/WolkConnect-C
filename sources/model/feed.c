#include "feed.h"
#include <string.h>

void initialize_feed(feed_t* feed, char* name, const char* reference, char* unit, const feed_type_t feedType)
{
    strncpy(feed->name, name, ITEM_NAME_SIZE);
    strncpy(feed->reference, reference, REFERENCE_SIZE);
    strncpy(feed->unit, unit, ITEM_UNIT_SIZE);
    feed->feedType = feedType;
}
