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

static event_t GetEvent(void)
{
	event_t event =
	{
		.key = KEY_UNKNOWN,
		.type = EVENT_NONE
	};

	if (eventcount > 0)
	{
		event = eventqueue[0];

		for (unsigned int i=0; i<eventcount; i++)
			eventqueue[i] = eventqueue[i + 1];

		eventcount--;
	}

	return(event);
}

static void ProcessEvent(event_t event)
{
	switch (event.type)		// TODO: just for testing remove this
	{
		case EVENT_KEYDOWN:
			Common_Printf("Key down: %d\n", event.key);
			break;

		case EVENT_KEYUP:
			Common_Printf("Key up: %d\n", event.key);
			break;

		default:
			break;
	}
}

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
	event_t event =
	{
		.key = KEY_UNKNOWN,
		.type = EVENT_NONE
	};

	while (1)
	{
		event = GetEvent();

		if (event.type == EVENT_NONE)
			break;

		ProcessEvent(event);
	}
}
