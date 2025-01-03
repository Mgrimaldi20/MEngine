#include "common.h"

#define MAX_EVENTS 256

typedef struct
{
	eventtype_t type;
	keycode_t key;
} event_t;

static unsigned int eventcount;
static event_t eventqueue[MAX_EVENTS];

static bool initialized;

bool Event_Init(void)
{
	if (initialized)
		return(true);

	for (unsigned int i=0; i<MAX_EVENTS; i++)
	{
		eventqueue[i].type = EVENT_NONE;
		eventqueue[i].key = KEY_UNKNOWN;
	}

	initialized = true;

	return(true);
}

void Event_Shutdown(void)
{
	if (!initialized)
		return;

	initialized = false;
}

void Event_QueueEvent(const eventtype_t type, const keycode_t key)
{
	if (eventcount >= MAX_EVENTS)
	{
		Log_Write(LOG_WARN, "Event queue is full, dropping event");		// TODO: remove this
		return;
	}

	event_t *event = &eventqueue[eventcount++];
	event->type = type;
	event->key = key;
}

void Event_RunEventLoop(void)
{
}
