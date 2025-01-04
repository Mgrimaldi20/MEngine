#include "common.h"

#define MAX_EVENTS 256

typedef struct
{
	eventtype_t type;
	int evar1;
	int evar2;
} event_t;

static unsigned int eventcount;
static event_t eventqueue[MAX_EVENTS];

static bool initialized;

static event_t GetEvent(void)
{
	event_t event =
	{
		.type = EVENT_NONE,
		.evar1 = 0,
		.evar2 = 0
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
}

bool Event_Init(void)
{
	if (initialized)
		return(true);

	for (unsigned int i=0; i<MAX_EVENTS; i++)
	{
		eventqueue[i].type = EVENT_NONE;
		eventqueue[i].evar1 = 0;
		eventqueue[i].evar2 = 0;
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

void Event_QueueEvent(const eventtype_t type, int var1, int var2)
{
	event_t *event = &eventqueue[eventcount++];

	if (eventcount >= MAX_EVENTS)
	{
		Log_Write(LOG_WARN, "Event queue is full, dropping event");		// TODO: remove this
		return;
	}

	event->type = type;
	event->evar1 = var1;
	event->evar2 = var2;
}

void Event_RunEventLoop(void)
{
	event_t event =
	{
		.type = EVENT_NONE,
		.evar1 = 0,
		.evar2 = 0
	};

	while (1)
	{
		event = GetEvent();

		if (event.type == EVENT_NONE)
			break;

		ProcessEvent(event);
	}
}
