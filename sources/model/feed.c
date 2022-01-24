#include "feed.h"
#include <string.h>

void initialize_feed(feed_t* feed, char* name, const char* reference, char* unit, const feed_type_t feedType)
{
    strncpy(feed->name, name, MANIFEST_ITEM_NAME_SIZE);
    strncpy(feed->reference, reference, MANIFEST_ITEM_REFERENCE_SIZE);
    strncpy(feed->unit, unit, MANIFEST_ITEM_UNIT_SIZE);
    feed->feedType = feedType;
}
