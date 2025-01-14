#include <limits.h>
#include "common.h"

#define MAX_EVENTS 256
#define DEF_LOG_FRAME_INTERVAL 300		// log any overflow messages every DEF_LOG_FRAME_INTERVAL num frames

typedef struct
{
	eventtype_t type;
	int evar1;
	int evar2;
} event_t;

static unsigned int eventcount;
static unsigned int queuehead;
static unsigned int queuetail;
static event_t eventqueue[MAX_EVENTS];

static unsigned int lastlogframe;
static unsigned int currentframe;

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
		event = eventqueue[queuehead];
		queuehead = (queuehead + 1) % MAX_EVENTS;
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
	{
		if ((currentframe - lastlogframe + UINT_MAX) % UINT_MAX >= DEF_LOG_FRAME_INTERVAL)
		{
			Log_Write(LOG_WARN, "Event queue overflow, discarding events: type: %d, evars: %d, %d", type, var1, var2);
			lastlogframe = currentframe;
		}

		return;
	}

	eventqueue[queuetail].type = type;
	eventqueue[queuetail].evar1 = var1;
	eventqueue[queuetail].evar2 = var2;
	queuetail = (queuetail + 1) % MAX_EVENTS;
	eventcount++;
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

	currentframe = (currentframe + 1) % UINT_MAX;
}
