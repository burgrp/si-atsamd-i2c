namespace applicationEvents {

	class EventHandler;

	EventHandler* firstHandler;

	class EventHandler {
	public:
		int eventId;
		int scheduled;
		EventHandler* nextHandler;
		virtual void onEvent() = 0;

		void handle(int eventId) {
			this->eventId = eventId;
			nextHandler = applicationEvents::firstHandler;
			applicationEvents::firstHandler = this;
		}
	};

	int freeEventId = 1;

	void handleEvents() {
		for (EventHandler* handler = firstHandler; handler; handler = handler->nextHandler) {
			while (handler->scheduled) {
				handler->onEvent();
				handler->scheduled--;
			}
		}
	}

	int createEventId() {
		return freeEventId++;
	}

	void schedule(int eventId) {
		for (EventHandler* handler = firstHandler; handler; handler = handler->nextHandler) {
			if (handler->eventId == eventId) {
				handler->scheduled++;
			}
		}
	}

}

void handleEvent() {
	applicationEvents::handleEvents();
}