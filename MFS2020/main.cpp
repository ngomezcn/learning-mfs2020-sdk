#include <windows.h>
#include <iostream>

#include <SimConnect.h>

int quit = 0;
HANDLE h_sim_connect = nullptr;

using namespace std;

struct data_define {
	int define_id;
	string datum_name; // file:///C:/MSFS%20SDK/Documentation/html/Programming_Tools/SimVars/Simulation_Variables.htm
	string units_name; // file:///C:/MSFS%20SDK/Documentation/html/Programming_Tools/SimVars/Simulation_Variable_Units.htm
	SIMCONNECT_DATATYPE datum_type; // file:///C:/MSFS%20SDK/Documentation/html/Programming_Tools/SimConnect/API_Reference/Structures_And_Enumerations/SIMCONNECT_DATATYPE.htm
	
	float f_epsilon = 0;
	/*
		If data is requested only when it changes(see the flags parameter of SimConnect_RequestDataOnSimObject) a change 
		will only be reported if it is greater than the value of this parameter(not greater than or equal to).
		Even the tiniest change will initiate the transmission of data.
		
		This can be used with integer data, the floating point fEpsilon value is first truncated to its integer component before the comparison is made(for example, an fEpsilon value of 2.9
		truncates to 2, so a data change of 2 will not trigger a transmission, and a change of 3 will do so).

		SimConnect_RequestDataOnSimObject: file:///C:/MSFS%20SDK/Documentation/html/Programming_Tools/SimConnect/API_Reference/Events_And_Data/SimConnect_RequestDataOnSimObject.htm
	*/

	data_define(): define_id(0), datum_type()
	{
	}
};

struct request {
	static int id;
	static int data_define_id;

	request() {
		id++;
	}
};

// To make a data request we will need:
static enum data_define_id {
	definition_1,
};

static enum data_request_id {
	REQUEST_1,
	// ... (we can use different requests to organize our response data)
};

// We group our events to prioritize them (in this case we'll play only with the brake event)
static enum group_id {
	group0
};

static enum event_id {
	key_brakes, // I can call this whatever I want, the important thing is the STRING
};

// THIS IS THE STRUCTURE THAT WILL HOLD THE DATA: (Change it as you need it):
struct sim_response {
	double altitude;
	int32_t heading;
	int32_t speed;
	int32_t vertical_speed;
};

// Event callback
void CALLBACK my_dispatch_proc1(SIMCONNECT_RECV* p_data, DWORD cb_data, void* p_context) {
	switch (p_data->dwID)
	{
		// CASE SIMULATION DATA RECIEVE EVENT
	case SIMCONNECT_RECV_ID_SIMOBJECT_DATA:
	{
		// RETRIEVE SIMULATION DATA and cast it to SIMCONNECT_RECV_SIMOBJECT_DATA POINTER
		auto*  p_obj_data = reinterpret_cast<SIMCONNECT_RECV_SIMOBJECT_DATA*>(p_data);


		// SWITCH TO FIND THE RIGHT REQUEST (IF THERE WAS MORE THAN ONE REQUEST THERE WOULD BE ANOTHER CASE)
		switch (p_obj_data->dwRequestID)
		{
		case REQUEST_1:

			// Cast the request data to a SimResponse (our defined struct)
			const auto* p_s = reinterpret_cast<sim_response*>(&p_obj_data->dwData);

			// DO WHATEVER YOU WANT WITH THE DATA (FOR SIMPLICITY WE WILL JUST PRINT IT TO THE CONSOLE)
			cout
				<< "\rAltitude: " << p_s->altitude
				<< " - Heading: " << p_s->heading
				<< " - Speed (knots): " << p_s->speed
				<< " - Vertical Speed: " << p_s->vertical_speed
				<< flush;
			break;
		}
		break;
	}
	// CASE SIMULATION QUIT EVENT
	case SIMCONNECT_RECV_ID_QUIT:
	{
		quit = 1; // Our application will quit if the simulator is closed
		break;
	}

	// CASE EVENT RECIEVED EVENT
	case SIMCONNECT_RECV_ID_EVENT: {
		const auto evt = reinterpret_cast<SIMCONNECT_RECV_EVENT*>(p_data); // Convert to the appropriate struct

		switch (evt->uEventID) // For each event
		{
		case key_brakes:
			printf("\nEvent brakes: %lu", evt->dwData); // Just print this event's data
			break;
		default: ;
		}
		break;
	}

	default:
		break;
	}
}


void RegisterClientEvents(HRESULT hr) {
	// Registering group and events
	hr = SimConnect_MapClientEventToSimEvent(h_sim_connect, key_brakes, "BRAKES"); // Register brake event

	hr = SimConnect_AddClientEventToNotificationGroup(h_sim_connect, group0, key_brakes); // Link brake event to GROUP0
	hr = SimConnect_SetNotificationGroupPriority(h_sim_connect, group0, SIMCONNECT_GROUP_PRIORITY_HIGHEST); // Set the priority of events in GROUP0 
}

bool initSimEvents() {
	HRESULT hr;

	// If connected to SimConnect displays a success message and return true
	if (SUCCEEDED(SimConnect_Open(&h_sim_connect, "Client Event Demo", NULL, 0, NULL, 0))) {
		std::cout << "\nConnected To Microsoft Flight Simulator 2020!\n";

		// REQUESTING OUR DATA

		// #IMPORTANT: the request order must follow the declaration order of the response struct!!
		// SimConnect_AddToDataDefinition takes:HANDLE, enum DEFINITION_ID, const char* DATA_NAME, const char* UNIT, DATATYPE (DEFAULT IS FLOAT64)
		hr = SimConnect_AddToDataDefinition(h_sim_connect, definition_1, "INDICATED ALTITUDE", "feet");
		hr = SimConnect_AddToDataDefinition(h_sim_connect, definition_1, "HEADING INDICATOR", "degrees", SIMCONNECT_DATATYPE_INT32);
		hr = SimConnect_AddToDataDefinition(h_sim_connect, definition_1, "Airspeed Indicated", "knots", SIMCONNECT_DATATYPE_INT32);
		hr = SimConnect_AddToDataDefinition(h_sim_connect, definition_1, "VERTICAL SPEED", "Feet per second", SIMCONNECT_DATATYPE_INT32);

		// EVERY SECOND REQUEST DATA FOR DEFINITION 1 ON THE CURRENT USER AIRCRAFT (SIMCONNECT_OBJECT_ID_USER)
		hr = SimConnect_RequestDataOnSimObject(h_sim_connect, REQUEST_1, definition_1, SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_VISUAL_FRAME);


		RegisterClientEvents(hr);


		// Process incoming SimConnect Server messages while the app is running
		while (quit == 0) {
			// Call SimConnect_CallDispatch EVERY SECOND until quit - MyDispatchProc1 will handle simulation events
			SimConnect_CallDispatch(h_sim_connect, my_dispatch_proc1, NULL);
			Sleep(1);
		}

		// When we finish running we can close our SimConnect handle
		hr = SimConnect_Close(h_sim_connect);
		return true;
	}
	else { // Or display failed message and return false
		std::cout << "\nFailed to Connect!!!!\n";
		return false;
	}
}

int main() {
	initSimEvents();

	return 0;
}