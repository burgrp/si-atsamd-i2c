namespace genericTimer {

	static int eventId;

	void initHardware();

	class Timer : public applicationEvents::EventHandler {
	public:

		int counter;

		virtual void onTimer() = 0;

		void onEvent() {
			if (counter) {
				counter--;
				if (!counter) {
					onTimer();
				}
			}
		};

		void start(int timeoutTenMs) {
			
			if (!genericTimer::eventId) {
				genericTimer::eventId = applicationEvents::createEventId();
				genericTimer::initHardware();
			}
			
			if (!eventId) {
				handle(genericTimer::eventId);
			}
			
			counter = timeoutTenMs;
		}

	};

}

