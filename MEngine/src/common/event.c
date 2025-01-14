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

/*
* Function: GetEvent
* Gets the next event from the event queue
* 
* Returns: The next event in the queue
*/
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

		for (unsigned int i=0; i<eventcount-1; i++)
			eventqueue[i] = eventqueue[i + 1];

		eventcount--;
	}

	return(event);
}

/*
* Function: ProcessEvent
* Sends the event to the appropriate handler based on the event type
* 
* 	event: The event to process
*/
static void ProcessEvent(event_t event)
{
	if (event.type == EVENT_KEY)
		Input_ProcessKeyInput(event.evar1, event.evar2);
}

/*
* Function: Event_Init
* Initializes the event system
* 
* Returns: A boolean if initialization was successful or not
*/
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

/*
* Function: Event_Shutdown
* Shuts down the event system
*/
void Event_Shutdown(void)
{
	if (!initialized)
		return;

	Log_WriteSeq(LOG_INFO, "Shutting down event system");

	initialized = false;
}

/*
* Function: Event_QueueEvent
* Queues an event to be processed, var1 and var2 store event data
* 
* 	type: The type of event
* 	var1: The first event variable
* 	var2: The second event variable
*/
void Event_QueueEvent(const eventtype_t type, int var1, int var2)
{
	if (eventcount >= MAX_EVENTS)
		return;

	event_t *event = &eventqueue[eventcount++];

	event->type = type;
	event->evar1 = var1;
	event->evar2 = var2;
}

/*
* Function: Event_RunEventLoop
* Processes all the events in the event queue and executes the command buffer during the engine frame
*/
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
		Cmd_ExecuteCommandBuffer();

		event = GetEvent();

		if (event.type == EVENT_NONE)
			break;

		ProcessEvent(event);
	}
}
